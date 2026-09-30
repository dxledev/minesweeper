#include "core/game.h"
#include "core/storage.h"
#include "core/statistics.h"
#include "core/theme.h"
#include "core/theme_source.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

using namespace minesweeper;

class CoreTests : public QObject {
    Q_OBJECT
private slots:
    void statisticsOutcomesAndPersistence() {
        QTemporaryDir directory;
        const auto path = directory.filePath("stats.json");
        Statistics first(path), second(path);
        first.reload();
        for (auto difficulty : {Difficulty::Easy, Difficulty::Intermediate, Difficulty::Expert}) {
            QVERIFY(!first.record(difficulty, State::Ready, 180000, true, 0));
            QVERIFY(!first.record(difficulty, State::Playing, 180000, false, 0));
            QVERIFY(!first.record(difficulty, State::Playing, 179999, true, 20));
            QVERIFY(first.record(difficulty, State::Playing, 180000, true, 20));
            QVERIFY(first.record(difficulty, State::Won, 90000, true, 71));
            QVERIFY(second.record(difficulty, State::Won, 45000, true, 71));
            QVERIFY(first.record(difficulty, State::Lost, 30000, true, 12));
            QVERIFY(second.record(difficulty, State::Won, 60000, true, 71));
            first.reload();
            const auto &stats = first.forDifficulty(difficulty);
            QCOMPARE(stats.wins, 3);
            QCOMPARE(stats.losses, 1);
            QCOMPARE(stats.quits, 1);
            QCOMPARE(stats.fastestWinMilliseconds, 45000);
            QCOMPARE(stats.bestWinStreak, 2);
            QCOMPARE(stats.currentWinStreak, 1);
            QCOMPARE(stats.clearedCells, 245);
        }
        QVERIFY(second.record(Difficulty::Expert, State::Playing, 180001, true, 100));
        Statistics restored(path);
        restored.reload();
        QCOMPARE(restored.forDifficulty(Difficulty::Easy).quits, 1);
        QCOMPARE(restored.forDifficulty(Difficulty::Expert).quits, 2);
        QCOMPARE(restored.forDifficulty(Difficulty::Expert).currentWinStreak, 0);
        QCOMPARE(restored.forDifficulty(Difficulty::Expert).bestWinStreak, 2);
    }

    void statisticsProtectInvalidFiles() {
        QTemporaryDir directory;
        const auto path = directory.filePath("stats.json");
        Statistics statistics(path);
        QVERIFY(!statistics.record(Difficulty::Easy, State::Playing, 100, true, 1));
        QVERIFY(!QFileInfo::exists(path));
        QVERIFY(statistics.record(Difficulty::Easy, State::Won, 1000, true, 71));
        auto invalid = readJson(path);
        auto difficulties = invalid.value("difficulties").toObject();
        auto easy = difficulties.value("easy").toObject();
        easy.insert("wins", -1);
        difficulties.insert("easy", easy);
        invalid.insert("difficulties", difficulties);
        writeJson(path, invalid);
        QVERIFY_EXCEPTION_THROWN(statistics.record(Difficulty::Easy, State::Won, 1000, true, 71), std::invalid_argument);
        QCOMPARE(readJson(path), invalid);
        QCOMPARE(statistics.forDifficulty(Difficulty::Easy).wins, 1);
    }

    void generation() {
        for (auto difficulty : {Difficulty::Easy, Difficulty::Intermediate, Difficulty::Expert}) {
            const auto size = dimensions(difficulty);
            for (std::uint32_t seed = 0; seed < 100; ++seed) {
                Game game(difficulty, seed);
                const int first = seed % game.cells().size();
                QVERIFY(game.reveal(first));
                QCOMPARE(game.cell(first).adjacent, 0);
                QVERIFY(!game.cell(first).mine);
                for (int neighbor : game.neighbors(first)) QVERIFY(!game.cell(neighbor).mine);
                int mines = 0;
                for (int index = 0; index < int(game.cells().size()); ++index) {
                    mines += game.cell(index).mine;
                    const auto adjacent = game.neighbors(index);
                    QCOMPARE(int(game.cell(index).adjacent), int(std::count_if(adjacent.begin(), adjacent.end(),
                        [&](int cell) { return game.cell(cell).mine; })));
                    if (game.cell(index).revealed) QVERIFY(!game.cell(index).mine);
                }
                QCOMPARE(mines, size.mines);
            }
        }
    }

    void flagsAndLoss() {
        Game game(Difficulty::Easy, 16);
        QVERIFY(game.toggleFlag(0));
        QVERIFY(!game.reveal(0));
        QCOMPARE(game.state(), State::Ready);
        QCOMPARE(game.flags(), 1);
        QVERIFY(game.toggleFlag(0));
        QVERIFY(game.reveal(0));
        QVERIFY(!game.toggleFlag(0));
        int mine = -1;
        for (int index = 0; index < int(game.cells().size()); ++index) {
            if (game.cell(index).mine) { mine = index; break; }
        }
        QVERIFY(game.reveal(mine));
        QCOMPARE(game.state(), State::Lost);
        QCOMPARE(game.detonated(), mine);
        QVERIFY(!game.toggleFlag(mine));
        QVERIFY(!game.reveal(80));
        QVERIFY(!game.reveal(-1));
        QVERIFY(!game.toggleFlag(1000));
    }

    void winAllDifficulties() {
        for (auto difficulty : {Difficulty::Easy, Difficulty::Intermediate, Difficulty::Expert}) {
            Game game(difficulty, 13);
            game.reveal(0);
            for (int index = 0; index < int(game.cells().size()); ++index) {
                if (!game.cell(index).mine) game.reveal(index);
            }
            QCOMPARE(game.state(), State::Won);
            QCOMPARE(game.flags(), game.size().mines);
            QCOMPARE(game.revealedCount(), int(game.cells().size()) - game.size().mines);
            for (const auto &cell : game.cells()) QCOMPARE(cell.flagged, cell.mine);
        }
    }

    void chording() {
        Game game(Difficulty::Intermediate, 5);
        game.reveal(0);
        int target = -1;
        for (int index = 0; index < int(game.cells().size()); ++index) {
            if (!game.cell(index).revealed || !game.cell(index).adjacent) continue;
            for (int neighbor : game.neighbors(index)) {
                if (!game.cell(neighbor).mine && !game.cell(neighbor).revealed) target = index;
            }
            if (target >= 0) break;
        }
        QVERIFY(target >= 0);
        QVERIFY(!game.chord(target));
        const auto adjacent = game.neighbors(target);
        for (int neighbor : adjacent) {
            if (game.cell(neighbor).mine) game.toggleFlag(neighbor);
        }
        const int before = game.revealedCount();
        QVERIFY(game.chord(target));
        QVERIFY(game.revealedCount() > before);
        QVERIFY(game.state() != State::Lost);
        for (int neighbor : adjacent) QVERIFY(game.cell(neighbor).flagged || game.cell(neighbor).revealed);
    }

    void wrongFlagsCanLose() {
        bool tested = false;
        for (std::uint32_t seed = 0; seed < 100 && !tested; ++seed) {
            Game game(Difficulty::Intermediate, seed);
            game.reveal(0);
            for (int index = 0; index < int(game.cells().size()) && !tested; ++index) {
                const auto &cell = game.cell(index);
                if (!cell.revealed || cell.adjacent != 1) continue;
                for (int neighbor : game.neighbors(index)) {
                    if (!game.cell(neighbor).revealed && !game.cell(neighbor).mine) {
                        game.toggleFlag(neighbor);
                        QVERIFY(game.chord(index));
                        QCOMPARE(game.state(), State::Lost);
                        tested = true;
                        break;
                    }
                }
            }
        }
        QVERIFY(tested);
    }

    void palettes() {
        QTemporaryDir directory;
        for (const auto &name : presetNames()) {
            const auto theme = presetTheme(name);
            QCOMPARE(parseTheme(theme.toJson()), theme);
            const auto path = directory.filePath(name + ".json");
            writeJson(path, theme.toJson());
            QCOMPARE(loadTheme(path), theme);
        }
        QVERIFY_EXCEPTION_THROWN(setColors(presetTheme("forest"), {"accent=bad"}), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(setColors(presetTheme("forest"), {"unknown=#112233"}), std::invalid_argument);
        const auto path = directory.filePath("palette.json");
        writeJson(path, {{"base", "1a1a1a"}, {"text", "fefefe"}, {"primary", "aabbcc"}});
        const auto imported = importPalette(path);
        QCOMPARE(imported.hex("background"), "#1a1a1a");
        QCOMPARE(imported.hex("accent"), "#aabbcc");
        QVERIFY(followTheme("file", directory.path()).sourcePath.endsWith("/palette.json"));
        QFile qml(directory.filePath("Colors.qml"));
        QVERIFY(qml.open(QIODevice::WriteOnly));
        qml.write("readonly property color background: Qt.rgba(16/255, 24/255, 25/255, 1)\n"
                  "readonly property color foreground: \"#ffffff\"\n"
                  "readonly property color primary: \"#aabbcc\"\n");
        qml.close();
        QCOMPARE(importPalette(qml.fileName()).hex("background"), "#101819");
        const auto customized = setColors(followTheme("file", path), {"accent=#112233"});
        QVERIFY(customized.sourcePath.isEmpty());
        QVERIFY(customized.sourceKind.isEmpty());
    }
};

QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
