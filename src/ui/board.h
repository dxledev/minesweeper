#pragma once

#include "core/game.h"
#include "core/theme.h"
#include <QWidget>

namespace minesweeper {

class Board : public QWidget {
    Q_OBJECT
public:
    Board(Game &game, QWidget *parent = nullptr);
    void setTheme(const Theme &theme);
    void resetSelection();
    void setPaused(bool paused);
    void setFlagMode(bool enabled);
    QRectF cellRect(int index) const;

signals:
    void moved();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void leaveEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    Game &game_;
    Theme theme_;
    bool paused_ = false;
    bool flagMode_ = false;
    int selected_ = 0;
    int hovered_ = -1;
    QPointF pointerPosition_;
    bool pointerInside_ = false;
    qreal cellSize() const;
    int cellAt(const QPointF &position) const;
    bool canClick(int index) const;
    void updateCursor();
    void act(int index, bool flag, bool chord);
    void paintCell(QPainter &painter, int index);
};

}
