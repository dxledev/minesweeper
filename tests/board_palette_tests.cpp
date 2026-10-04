#include "core/storage.h"
#include "core/theme_source.h"
#include "ui/board.h"

#include <QMouseEvent>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <cmath>

using namespace minesweeper;

namespace {

double luminance(const QColor &color) {
    const auto linear = [](double component) {
        return component <= .04045 ? component / 12.92 : std::pow((component + .055) / 1.055, 2.4);
    };
    return .2126 * linear(color.redF()) + .7152 * linear(color.greenF()) + .0722 * linear(color.blueF());
}

double contrast(const QColor &first, const QColor &second) {
    const double a = luminance(first), b = luminance(second);
    return (std::max(a, b) + .05) / (std::min(a, b) + .05);
}

QImage renderBoard(Board &board, const Theme &theme) {
    QImage image(board.size(), QImage::Format_RGB32);
    image.fill(theme.color("surface"));
    board.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
    return image;
}

double edgeContrast(const QImage &image, const QRectF &cell, const QColor &surface) {
    double best = 1;
    const int y = qRound(cell.center().y());
    for (int x = qFloor(cell.left()); x <= qCeil(cell.left() + 3); ++x)
        best = std::max(best, contrast(image.pixelColor(x, y), surface));
    return best;
}

}

class BoardPaletteTests : public QObject {
    Q_OBJECT
private slots:
    void visibleGrid_data() {
        QTest::addColumn<Theme>("theme");
        for (const auto &name : presetNames())
            QTest::newRow(qPrintable(name)) << presetTheme(name);
        for (const auto &surface : {"#18141f", "#eeeeee", "#777777"}) {
            auto theme = presetTheme("slate");
            for (const auto &key : {"surface", "surface_alt", "related", "matching", "selection", "grid"})
                theme.colors[key] = QColor(surface);
            QTest::newRow(surface) << theme;
        }
        QTemporaryDir directory;
        const auto path = directory.filePath("dynamic.json");
        writeJson(path, QJsonObject{{"background", "#18141f"}, {"foreground", "#d4c7b2"},
                                   {"primary", "#9aaddf"}, {"surface_container_low", "#18141f"},
                                   {"surface_container", "#18141f"}, {"outline", "#272334"}});
        QTest::newRow("dynamic-colliding-surfaces") << importPalette(path);
        const auto palettes = qEnvironmentVariable("MINESWEEPER_TEST_PALETTES").split(':', Qt::SkipEmptyParts);
        for (const auto &palette : palettes)
            QTest::newRow(qPrintable(palette)) << importPalette(palette);
    }

    void visibleGrid() {
        QFETCH(Theme, theme);
        const auto palette = boardPalette(theme);
        const QList<QColor> borders{palette.coveredBorder, palette.revealedBorder,
                                   palette.flaggedBorder, palette.hoveredBorder};
        const QStringList fills{"surface_alt", "related", "matching", "selection"};
        for (int index = 0; index < borders.size(); ++index) {
            QVERIFY(contrast(borders[index], theme.color("surface")) >= 3);
            QVERIFY(contrast(borders[index], theme.color(fills[index])) >= 3);
        }
        Game game(Difficulty::Easy, 5);
        Board board(game);
        board.resize(466, 466);
        board.setTheme(presetTheme("forest"));
        board.setTheme(theme);
        auto image = renderBoard(board, theme);
        for (int index = 0; index < 81; ++index)
            QVERIFY(edgeContrast(image, board.cellRect(index), theme.color("surface")) >= 2);

        QVERIFY(game.toggleFlag(80));
        QVERIFY(game.reveal(0));
        const auto position = board.cellRect(79).center();
        QMouseEvent move(QEvent::MouseMove, position, position, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&board, &move);
        image = renderBoard(board, theme);
        for (const int index : {0, 79, 80})
            QVERIFY(edgeContrast(image, board.cellRect(index), theme.color("surface")) >= 2);
        const auto preview = qEnvironmentVariable("MINESWEEPER_BOARD_PREVIEW");
        if (!preview.isEmpty() && QByteArray(QTest::currentDataTag()) == "dynamic-colliding-surfaces")
            QVERIFY(image.save(preview));
    }
};

QTEST_MAIN(BoardPaletteTests)
#include "board_palette_tests.moc"
