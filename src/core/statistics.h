#pragma once

#include "game.h"
#include <QString>
#include <array>

namespace minesweeper {

struct DifficultyStats {
    qint64 wins = 0;
    qint64 losses = 0;
    qint64 quits = 0;
    qint64 fastestWinMilliseconds = 0;
    qint64 currentWinStreak = 0;
    qint64 bestWinStreak = 0;
    qint64 clearedCells = 0;
};

class Statistics {
public:
    static constexpr qint64 quitThresholdMilliseconds = 180000;
    explicit Statistics(QString path);
    void reload();
    bool record(Difficulty difficulty, State state, qint64 elapsedMilliseconds,
                bool madeMove, int clearedCells);
    const DifficultyStats &forDifficulty(Difficulty difficulty) const;

private:
    QString path_;
    std::array<DifficultyStats, 3> totals_{};
};

}
