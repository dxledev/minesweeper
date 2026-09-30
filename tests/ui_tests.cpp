#include "core/storage.h"
#include "core/theme_source.h"
#include "ui/window.h"
#include "ui/modal_backdrop.h"

#include <QApplication>
#include <QFile>
#include <QFrame>
#include <QSignalSpy>
#include <QStyleOptionButton>
#include <QTemporaryDir>
#include <QtTest>
#include <unistd.h>

using namespace minesweeper;

class UiTests : public QObject {
    Q_OBJECT
private:
    void show(Window &window) {
        window.show();
        window.activateWindow();
        window.board()->setFocus();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
    }

    void pointSource(const QString &selector, const QString &destination) {
        const auto temporary = selector + ".next";
        QCOMPARE(::symlink(destination.toLocal8Bit().constData(), temporary.toLocal8Bit().constData()), 0);
        QCOMPARE(::rename(temporary.toLocal8Bit().constData(), selector.toLocal8Bit().constData()), 0);
    }

    qint64 edgeEnergy(const QImage &image, const QRect &logicalArea) {
        const qreal scale = image.devicePixelRatio();
        const QRect area(QPoint(qRound(logicalArea.x() * scale), qRound(logicalArea.y() * scale)),
                         QSize(qRound(logicalArea.width() * scale), qRound(logicalArea.height() * scale)));
        qint64 energy = 0;
        for (int y = area.top(); y <= area.bottom(); ++y)
            for (int x = area.left(); x < area.right(); ++x)
                energy += std::abs(qGray(image.pixel(x, y)) - qGray(image.pixel(x + 1, y)));
        return energy;
    }

private slots:
    void initTestCase() {
        QApplication::setStyle("Fusion");
    }

    void boardInteractionAndLayout() {
        QTemporaryDir directory;
        auto theme = ensureTheme(directory.filePath("theme.json"));
        Window window(directory.path(), theme);
        show(window);
        auto *board = window.board();
        QTest::mouseClick(board, Qt::RightButton, Qt::NoModifier, board->cellRect(80).center().toPoint());
        QCOMPARE(window.game().flags(), 1);
        QTest::mouseClick(board, Qt::LeftButton, Qt::NoModifier, board->cellRect(0).center().toPoint());
        QCOMPARE(window.game().state(), State::Playing);
        QVERIFY(window.game().cell(0).revealed);
        QTest::keyClick(board, Qt::Key_Right);
        QTest::keyClick(board, Qt::Key_Down);
        QVERIFY(window.game().cell(0).revealed);
        window.resize(580, 680);
        QTest::qWait(30);
        const auto first = board->cellRect(0), last = board->cellRect(80);
        QVERIFY(first.left() >= 0 && first.top() >= 0);
        QVERIFY(last.right() <= board->width() && last.bottom() <= board->height());
        QVERIFY(first.width() >= 20);
        window.hide();
        QTest::qWait(40);
        for (auto difficulty : {Difficulty::Intermediate, Difficulty::Expert}) {
            Window other(directory.path(), theme, difficulty);
            other.resize(580, 680);
            show(other);
            const auto lastCell = other.board()->cellRect(int(other.game().cells().size()) - 1);
            QVERIFY(lastCell.right() <= other.board()->width());
            QVERIFY(lastCell.bottom() <= other.board()->height());
            QVERIFY2(lastCell.width() >= 16, qPrintable(QString("Cell width %1 in %2 × %3 window")
                .arg(lastCell.width()).arg(other.width()).arg(other.height())));
        }
    }

    void cliStyleEditsDoNotResetGame() {
        QTemporaryDir directory;
        const auto path = directory.filePath("theme.json");
        auto theme = ensureTheme(path);
        Window window(directory.path(), theme);
        show(window);
        QTest::mouseClick(window.board(), Qt::LeftButton, Qt::NoModifier, window.board()->cellRect(0).center().toPoint());
        const int revealed = window.game().revealedCount();
        const auto next = presetTheme("paper");
        writeJson(path, next.toJson());
        QTRY_COMPARE(window.palette().color(QPalette::Window), next.color("background"));
        QCOMPARE(window.game().revealedCount(), revealed);
        QVERIFY(window.game().cell(0).revealed);
        QFile invalid(path);
        QVERIFY(invalid.open(QIODevice::WriteOnly | QIODevice::Truncate));
        invalid.write("{invalid}");
        invalid.close();
        QTest::qWait(100);
        QCOMPARE(window.palette().color(QPalette::Window), next.color("background"));
        writeJson(path, theme.toJson());
        QTRY_COMPARE(window.palette().color(QPalette::Window), theme.color("background"));
    }

    void sourcesSymlinksAndWallpaperUpdates() {
        QTemporaryDir directory;
        const auto sourceRoot = directory.filePath("sources");
        QVERIFY(QDir().mkpath(sourceRoot));
        const QStringList sources{"static", "provider-a", "provider-b"};
        const QStringList presets{"rose", "forest", "slate"};
        for (int index = 0; index < sources.size(); ++index) {
            QVERIFY(QDir().mkpath(sourceRoot + "/" + sources[index]));
            writeJson(sourceRoot + "/" + sources[index] + "/theme.json", presetTheme(presets[index]).toJson());
        }
        const auto selector = sourceRoot + "/active";
        pointSource(selector, sourceRoot + "/static");
        const auto path = directory.filePath("theme.json");
        auto initial = followTheme("file", selector + "/theme.json");
        writeJson(path, initial.toJson());
        ThemeWatcher watcher(path, initial);
        QSignalSpy changed(&watcher, &ThemeWatcher::changed);
        // Exercise every ordered mode transition, then an atomic palette update.
        for (int from = 0; from < sources.size(); ++from) {
            for (int to = 0; to < sources.size(); ++to) {
                if (from == to) continue;
                pointSource(selector, sourceRoot + "/" + sources[from]);
                QTRY_COMPARE(watcher.theme().name, presets[from]);
                pointSource(selector, sourceRoot + "/" + sources[to]);
                QTRY_COMPARE(watcher.theme().name, presets[to]);
                QCOMPARE(loadTheme(path).hex("accent"), presetTheme(presets[to]).hex("accent"));
            }
        }
        pointSource(selector, sourceRoot + "/provider-b");
        QTRY_COMPARE(watcher.theme().name, "slate");
        writeJson(sourceRoot + "/provider-b/theme.json", presetTheme("paper").toJson());
        QTRY_COMPARE(watcher.theme().name, "paper");
        QCOMPARE(loadTheme(path).name, "paper");
        QVERIFY(changed.count() >= 7);
        // Renderers also replace the entire directory, not only the file.
        QVERIFY(QDir().mkpath(sourceRoot + "/rendered"));
        writeJson(sourceRoot + "/rendered/theme.json", presetTheme("rose").toJson());
        QVERIFY(QDir().rename(sourceRoot + "/provider-b", sourceRoot + "/old"));
        QVERIFY(QDir().rename(sourceRoot + "/rendered", sourceRoot + "/provider-b"));
        QTRY_COMPARE(watcher.theme().name, "rose");
        writeJson(sourceRoot + "/provider-b/theme.json", presetTheme("forest").toJson());
        QTRY_COMPARE(watcher.theme().name, "forest");
    }

    void followStartupAndInvalidRecovery() {
        QTemporaryDir directory;
        const auto source = directory.filePath("input.json");
        const auto path = directory.filePath("theme.json");
        writeJson(source, presetTheme("forest").toJson());
        auto initial = followTheme("file", source);
        writeJson(path, initial.toJson());
        writeJson(source, presetTheme("paper").toJson());
        ThemeWatcher watcher(path, initial);
        QSignalSpy rejected(&watcher, &ThemeWatcher::rejected);
        QTRY_COMPARE(watcher.theme().name, "paper");
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("{}");
        file.close();
        QTRY_VERIFY(rejected.count() > 0);
        QCOMPARE(watcher.theme().name, "paper");
        QCOMPARE(loadTheme(path).name, "paper");
        writeJson(source, presetTheme("slate").toJson());
        QTRY_COMPARE(watcher.theme().name, "slate");
        writeJson(path, presetTheme("rose").toJson());
        QTRY_COMPARE(watcher.theme().name, "rose");
        writeJson(source, presetTheme("forest").toJson());
        QTest::qWait(100);
        QCOMPARE(watcher.theme().name, "rose");
    }

    void modalPauseResizeAndTheme() {
        QTemporaryDir directory;
        const auto path = directory.filePath("theme.json");
        Window window(directory.path(), ensureTheme(path));
        show(window);
        QTest::mouseClick(window.board(), Qt::LeftButton, Qt::NoModifier, window.board()->cellRect(0).center().toPoint());
        QTest::qWait(40);
        auto *newGame = window.findChild<QPushButton *>("primary");
        QTest::mouseClick(newGame, Qt::LeftButton);
        auto *modal = window.findChild<QWidget *>("modalBackdrop");
        QVERIFY(modal && modal->isVisible());
        QCOMPARE(modal->window(), &window);
        const qint64 elapsed = window.elapsedMilliseconds();
        QTest::qWait(80);
        QCOMPARE(window.elapsedMilliseconds(), elapsed);
        const int before = window.game().revealedCount();
        QTest::keyClick(window.focusWidget(), Qt::Key_Tab);
        QVERIFY(window.focusWidget()->objectName() == "primary");
        QTest::keyClick(window.focusWidget(), Qt::Key_Tab);
        QCOMPARE(window.focusWidget()->objectName(), "keepPlaying");
        writeJson(path, presetTheme("paper").toJson());
        QTRY_COMPARE(window.palette().color(QPalette::Window), presetTheme("paper").color("background"));
        QVERIFY(modal->isVisible());
        window.resize(600, 560);
        QTest::qWait(40);
        QCOMPARE(modal->geometry(), window.centralWidget()->rect());
        QTest::keyClick(window.focusWidget(), Qt::Key_Escape);
        QVERIFY(!modal->isVisible());
        QCOMPARE(window.game().revealedCount(), before);
        QTest::qWait(30);
        QVERIFY(window.elapsedMilliseconds() > elapsed);
        auto *expert = window.findChildren<QPushButton *>("difficulty").last();
        QTest::mouseClick(expert, Qt::LeftButton);
        QVERIFY(modal->isVisible());
        QCOMPARE(window.game().difficulty(), Difficulty::Easy);
        auto *confirm = modal->findChild<QPushButton *>("primary");
        QTest::mouseClick(confirm, Qt::LeftButton);
        QVERIFY(!modal->isVisible());
        QCOMPARE(window.game().difficulty(), Difficulty::Expert);
        QCOMPARE(window.game().state(), State::Ready);
        QCOMPARE(window.elapsedMilliseconds(), 0);
    }

    void modalSnapshotAndButtonSizing() {
        QTemporaryDir directory;
        Window window(directory.path(), ensureTheme(directory.filePath("theme.json")));
        show(window);
        QTest::mouseClick(window.board(), Qt::LeftButton, Qt::NoModifier, window.board()->cellRect(0).center().toPoint());
        const auto original = window.centralWidget()->grab().toImage();
        QTest::mouseClick(window.findChild<QPushButton *>("primary"), Qt::LeftButton);
        auto *modal = window.findChild<QWidget *>("modalBackdrop");
        QVERIFY(modal->isVisible());
        const auto backdrop = modal->grab().toImage();
        const QRect header(24, 22, 260, 75);
        const auto originalEdges = edgeEnergy(original, header);
        const auto blurredEdges = edgeEnergy(backdrop, header);
        QVERIFY(blurredEdges > 0);
        QVERIFY(blurredEdges < originalEdges / 2);
        for (const auto size : {QSize(580, 680), QSize(1920, 1080), QSize(2560, 1440)}) {
            window.resize(size);
            QTest::qWait(40);
            auto *card = modal->findChild<QFrame *>("modalCard");
            QVERIFY(modal->rect().contains(card->geometry()));
            for (auto *action : modal->findChildren<QPushButton *>()) {
                QStyleOptionButton option;
                option.initFrom(action);
                option.rect = action->rect();
                const auto textArea = action->style()->subElementRect(QStyle::SE_PushButtonContents, &option, action);
                QVERIFY2(textArea.height() >= action->fontMetrics().height() + 4,
                         qPrintable(QString("%1 has %2px text space for a %3px font")
                             .arg(action->text()).arg(textArea.height()).arg(action->fontMetrics().height())));
                QVERIFY(textArea.width() >= action->fontMetrics().horizontalAdvance(action->text()));
                QVERIFY(card->rect().contains(QRect(action->mapTo(card, QPoint()), action->size())));
            }
            const auto resized = modal->grab().toImage();
            QVERIFY(edgeEnergy(resized, header) > 0);
        }
        QTest::keyClick(window.focusWidget(), Qt::Key_Escape);
        QVERIFY(!modal->isVisible());
        QVERIFY(window.game().cell(0).revealed);
    }

    void modalPreservesBackgroundColors() {
        QTemporaryDir directory;
        const auto path = directory.filePath("theme.json");
        Window window(directory.path(), ensureTheme(path));
        window.resize(1920, 1080);
        show(window);
        QTest::mouseClick(window.board(), Qt::LeftButton, Qt::NoModifier, window.board()->cellRect(0).center().toPoint());
        QTest::mouseClick(window.findChild<QPushButton *>("primary"), Qt::LeftButton);
        auto *modal = window.findChild<QWidget *>("modalBackdrop");
        for (const auto &name : {"forest", "slate", "paper"}) {
            const auto theme = presetTheme(name);
            writeJson(path, theme.toJson());
            QTRY_COMPARE(window.palette().color(QPalette::Window), theme.color("background"));
            QTest::qWait(40);
            const auto snapshot = modal->grab().toImage();
            const qreal scale = snapshot.devicePixelRatio();
            for (const auto &point : {QPoint(window.width() / 2, 8), QPoint(window.width() / 2, window.height() - 8)}) {
                const auto actual = snapshot.pixelColor(qRound(point.x() * scale), qRound(point.y() * scale));
                const auto expected = theme.color("background");
                QVERIFY2(std::abs(actual.red() - expected.red()) <= 2
                    && std::abs(actual.green() - expected.green()) <= 2
                    && std::abs(actual.blue() - expected.blue()) <= 2,
                    qPrintable(QString("%1 background changed from %2 to %3")
                        .arg(name, expected.name(), actual.name())));
            }
        }
    }

    void modalBlurKeepsSolidEdges() {
        QWidget parent;
        ModalBackdrop backdrop(&parent);
        backdrop.resize(300, 240);
        for (const auto &color : {QColor("#121318"), QColor("#faf5e4"), QColor("#a73872")}) {
            QPixmap original(600, 480);
            original.setDevicePixelRatio(2);
            original.fill(color);
            backdrop.capture(original);
            backdrop.setTint(color);
            const auto image = backdrop.grab().toImage();
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    const auto pixel = image.pixelColor(x, y);
                    QVERIFY(pixel.alpha() == 255);
                    QVERIFY(std::abs(pixel.red() - color.red()) <= 2);
                    QVERIFY(std::abs(pixel.green() - color.green()) <= 2);
                    QVERIFY(std::abs(pixel.blue() - color.blue()) <= 2);
                }
            }
        }
    }

    void manualPause() {
        QTemporaryDir directory;
        Window window(directory.path(), ensureTheme(directory.filePath("theme.json")));
        show(window);
        QTest::mouseClick(window.board(), Qt::LeftButton, Qt::NoModifier, window.board()->cellRect(0).center().toPoint());
        auto *pause = window.findChild<QPushButton *>("pause");
        QTest::mouseClick(pause, Qt::LeftButton);
        QCOMPARE(pause->text(), "Resume");
        const qint64 elapsed = window.elapsedMilliseconds();
        const int revealed = window.game().revealedCount();
        QTest::qWait(80);
        QTest::mouseClick(window.board(), Qt::LeftButton, Qt::NoModifier, window.board()->cellRect(80).center().toPoint());
        QCOMPARE(window.game().revealedCount(), revealed);
        QCOMPARE(window.elapsedMilliseconds(), elapsed);
        QTest::mouseClick(pause, Qt::LeftButton);
        QCOMPARE(pause->text(), "Pause");
        QTest::qWait(30);
        QVERIFY(window.elapsedMilliseconds() > elapsed);
    }

    void previews() {
        const auto output = qEnvironmentVariable("MINESWEEPER_PREVIEW_DIR");
        if (output.isEmpty()) return;
        QVERIFY(QDir().mkpath(output));
        QTemporaryDir directory;
        const auto path = directory.filePath("theme.json");
        Window window(directory.path(), ensureTheme(path));
        show(window);
        QTest::mouseClick(window.board(), Qt::LeftButton, Qt::NoModifier, window.board()->cellRect(0).center().toPoint());
        QVERIFY(window.grab().save(output + "/easy.png"));
        QTest::mouseClick(window.findChild<QPushButton *>("primary"), Qt::LeftButton);
        QVERIFY(window.grab().save(output + "/modal.png"));
        Window expert(directory.path(), loadTheme(path), Difficulty::Expert);
        show(expert);
        expert.resize(580, 680);
        QTest::qWait(40);
        QVERIFY(expert.grab().save(output + "/expert-compact.png"));
    }
};

QTEST_MAIN(UiTests)
#include "ui_tests.moc"
