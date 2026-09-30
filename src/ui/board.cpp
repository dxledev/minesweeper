#include "board.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

namespace minesweeper {
namespace {

void drawFlag(QPainter &painter, const QRectF &rect, const QColor &color) {
    const QPointF pole(rect.center().x() - rect.width() * .14, rect.top() + rect.height() * .25);
    painter.setPen(QPen(color, std::max(1.5, rect.width() * .055), Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(pole, pole + QPointF(0, rect.height() * .5));
    painter.drawLine(pole + QPointF(-rect.width() * .12, rect.height() * .5),
                     pole + QPointF(rect.width() * .18, rect.height() * .5));
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(QPolygonF{pole, pole + QPointF(rect.width() * .35, rect.height() * .15),
                                  pole + QPointF(0, rect.height() * .29)});
}

void drawMine(QPainter &painter, const QRectF &rect, const QColor &color) {
    const QPointF center = rect.center();
    const qreal radius = rect.width() * .17;
    painter.setPen(QPen(color, std::max(1.5, rect.width() * .05), Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(center + QPointF(-radius * 1.5, 0), center + QPointF(radius * 1.5, 0));
    painter.drawLine(center + QPointF(0, -radius * 1.5), center + QPointF(0, radius * 1.5));
    painter.drawLine(center + QPointF(-radius, -radius), center + QPointF(radius, radius));
    painter.drawLine(center + QPointF(radius, -radius), center + QPointF(-radius, radius));
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(center, radius, radius);
}

}

Board::Board(Game &game, QWidget *parent) : QWidget(parent), game_(game), theme_(presetTheme("forest")) {
    setObjectName("board");
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName("Minefield");
    setAccessibleDescription("Arrow keys select a cell. Space reveals, F flags, Enter clears adjacent cells.");
}

void Board::setTheme(const Theme &theme) {
    theme_ = theme;
    update();
}

void Board::resetSelection() {
    selected_ = 0;
    hovered_ = -1;
    update();
}

void Board::setPaused(bool paused) {
    paused_ = paused;
    update();
}

qreal Board::cellSize() const {
    return std::max(1., std::min((width() - 16.) / game_.size().columns,
                               (height() - 16.) / game_.size().rows));
}

QRectF Board::cellRect(int index) const {
    const qreal size = cellSize();
    const auto &dimensions = game_.size();
    const qreal left = (width() - size * dimensions.columns) / 2;
    const qreal top = (height() - size * dimensions.rows) / 2;
    return {left + index % dimensions.columns * size, top + index / dimensions.columns * size, size, size};
}

int Board::cellAt(const QPointF &position) const {
    const auto first = cellRect(0);
    const int column = int(std::floor((position.x() - first.left()) / cellSize()));
    const int row = int(std::floor((position.y() - first.top()) / cellSize()));
    if (column < 0 || column >= game_.size().columns || row < 0 || row >= game_.size().rows) return -1;
    return row * game_.size().columns + column;
}

void Board::paintCell(QPainter &painter, int index) {
    const auto &cell = game_.cell(index);
    const auto rect = cellRect(index).adjusted(1.5, 1.5, -1.5, -1.5);
    const bool lost = game_.state() == State::Lost;
    const bool mineVisible = cell.mine && (cell.revealed || lost);
    const bool wrongFlag = lost && cell.flagged && !cell.mine;
    QColor background = theme_.color(cell.revealed ? "related" : "surface_alt");
    if (cell.flagged) background = theme_.color("matching");
    if (index == hovered_ && !cell.revealed && !game_.finished()) background = theme_.color("selection");
    if (index == game_.detonated()) background = theme_.dangerColor;
    painter.setBrush(background);
    painter.setPen(cell.revealed ? QPen(theme_.color("grid"), .7) : Qt::NoPen);
    painter.drawRoundedRect(rect, std::min(6., rect.width() * .14), std::min(6., rect.width() * .14));
    if (mineVisible && !cell.flagged) {
        drawMine(painter, rect, index == game_.detonated() ? theme_.color("background") : theme_.dangerColor);
    } else if (cell.flagged) {
        drawFlag(painter, rect, wrongFlag ? theme_.dangerColor : theme_.color("accent"));
        if (wrongFlag) {
            painter.setPen(QPen(theme_.dangerColor, 2));
            painter.drawLine(rect.topLeft() + QPointF(4, 4), rect.bottomRight() - QPointF(4, 4));
        }
    } else if (cell.revealed && cell.adjacent) {
        QFont font = painter.font();
        font.setPixelSize(std::max(10, int(rect.height() * .48)));
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(cell.adjacent <= 2 ? theme_.color("accent") : theme_.color("text"));
        painter.drawText(rect, Qt::AlignCenter, QString::number(cell.adjacent));
    }
    if (hasFocus() && selected_ == index && !game_.finished()) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(theme_.color("accent"), 2));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 4, 4);
    }
}

void Board::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (paused_) {
        painter.setPen(theme_.color("muted"));
        QFont font = painter.font();
        font.setPixelSize(24);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, "Paused");
        return;
    }
    for (int index = 0; index < int(game_.cells().size()); ++index) paintCell(painter, index);
}

void Board::act(int index, bool flag, bool chord) {
    if (paused_ || index < 0) return;
    selected_ = index;
    const bool changed = chord ? game_.chord(index) : flag ? game_.toggleFlag(index) : game_.reveal(index);
    update();
    if (changed) emit moved();
}

void Board::mousePressEvent(QMouseEvent *event) {
    setFocus(Qt::MouseFocusReason);
    const int index = cellAt(event->position());
    if (event->button() == Qt::MiddleButton) act(index, false, true);
    else if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
        act(index, event->button() == Qt::RightButton || event->modifiers().testFlag(Qt::ShiftModifier) || flagMode_, false);
}

void Board::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) act(cellAt(event->position()), false, true);
}

void Board::mouseMoveEvent(QMouseEvent *event) {
    const int next = cellAt(event->position());
    if (next != hovered_) {
        hovered_ = next;
        update();
    }
}

void Board::leaveEvent(QEvent *) {
    hovered_ = -1;
    update();
}

void Board::keyPressEvent(QKeyEvent *event) {
    if (paused_) { event->ignore(); return; }
    const int column = selected_ % game_.size().columns;
    const int row = selected_ / game_.size().columns;
    switch (event->key()) {
    case Qt::Key_Left: case Qt::Key_H: if (column > 0) --selected_; break;
    case Qt::Key_Right: case Qt::Key_L: if (column + 1 < game_.size().columns) ++selected_; break;
    case Qt::Key_Up: case Qt::Key_K: if (row > 0) selected_ -= game_.size().columns; break;
    case Qt::Key_Down: case Qt::Key_J: if (row + 1 < game_.size().rows) selected_ += game_.size().columns; break;
    case Qt::Key_F: act(selected_, true, false); break;
    case Qt::Key_Space: act(selected_, flagMode_, false); break;
    case Qt::Key_Return: case Qt::Key_Enter: act(selected_, false, true); break;
    default: QWidget::keyPressEvent(event); return;
    }
    update();
}

}
