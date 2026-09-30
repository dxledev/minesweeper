#include "statistics.h"
#include "storage.h"

#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <algorithm>
#include <stdexcept>

namespace minesweeper {
namespace {

qint64 readCount(const QJsonObject &object, const QString &key) {
    const auto value = object.value(key);
    if (value.isUndefined()) return 0;
    const auto count = value.toInteger(-1);
    if (!value.isDouble() || count < 0 || count > 1000000000000LL)
        throw std::invalid_argument(("Invalid statistic: " + key).toStdString());
    return count;
}

DifficultyStats parseStats(const QJsonObject &object) {
    return {readCount(object, "wins"), readCount(object, "losses"), readCount(object, "quits"),
            readCount(object, "fastest_win_ms"), readCount(object, "current_win_streak"),
            readCount(object, "best_win_streak"), readCount(object, "cleared_cells")};
}

QJsonObject serialize(const DifficultyStats &stats) {
    return {{"wins", stats.wins}, {"losses", stats.losses}, {"quits", stats.quits},
            {"fastest_win_ms", stats.fastestWinMilliseconds},
            {"current_win_streak", stats.currentWinStreak}, {"best_win_streak", stats.bestWinStreak},
            {"cleared_cells", stats.clearedCells}};
}

void updateStats(DifficultyStats &stats, State state, qint64 milliseconds, int clearedCells) {
    stats.clearedCells += clearedCells;
    if (state == State::Won) {
        ++stats.wins;
        ++stats.currentWinStreak;
        stats.bestWinStreak = std::max(stats.bestWinStreak, stats.currentWinStreak);
        const auto duration = std::max(qint64(1), milliseconds);
        if (!stats.fastestWinMilliseconds || duration < stats.fastestWinMilliseconds)
            stats.fastestWinMilliseconds = duration;
    } else {
        if (state == State::Lost) ++stats.losses;
        else ++stats.quits;
        stats.currentWinStreak = 0;
    }
}

}

Statistics::Statistics(QString path) : path_(std::move(path)) {}

const DifficultyStats &Statistics::forDifficulty(Difficulty difficulty) const {
    return totals_.at(size_t(difficulty));
}

void Statistics::reload() {
    std::array<DifficultyStats, 3> next{};
    if (QFileInfo::exists(path_)) {
        const auto data = readJson(path_);
        if (data.value("version").toInteger() != 1 || !data.value("difficulties").isObject())
            throw std::invalid_argument("Invalid statistics file");
        const auto difficulties = data.value("difficulties").toObject();
        for (auto difficulty : {Difficulty::Easy, Difficulty::Intermediate, Difficulty::Expert}) {
            const auto key = QString::fromUtf8(difficultyName(difficulty));
            if (!difficulties.value(key).isObject())
                throw std::invalid_argument("Invalid difficulty statistics");
            next.at(size_t(difficulty)) = parseStats(difficulties.value(key).toObject());
        }
    }
    totals_ = next;
}

bool Statistics::record(Difficulty difficulty, State state, qint64 elapsedMilliseconds,
                        bool madeMove, int clearedCells) {
    if (!madeMove || state == State::Ready) return false;
    if (state == State::Playing && elapsedMilliseconds < quitThresholdMilliseconds) return false;
    if (elapsedMilliseconds < 0 || clearedCells < 0)
        throw std::invalid_argument("Invalid game statistics");
    if (!QDir().mkpath(QFileInfo(path_).absolutePath()))
        throw std::runtime_error("Cannot create statistics directory");
    QLockFile lock(path_ + ".lock");
    if (!lock.tryLock(100)) throw std::runtime_error("Statistics are busy; please try again");
    reload();
    auto next = totals_;
    updateStats(next.at(size_t(difficulty)), state, elapsedMilliseconds, clearedCells);
    QJsonObject difficulties;
    for (auto level : {Difficulty::Easy, Difficulty::Intermediate, Difficulty::Expert})
        difficulties.insert(QString::fromUtf8(difficultyName(level)), serialize(next.at(size_t(level))));
    writeJson(path_, {{"version", 1}, {"difficulties", difficulties}});
    totals_ = next;
    return true;
}

}
