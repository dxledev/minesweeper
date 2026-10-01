#pragma once

#include "core/theme.h"
#include <QPixmap>
#include <QPropertyAnimation>
#include <QWidget>

namespace minesweeper {

class SolvedOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
public:
    explicit SolvedOverlay(QWidget *parent = nullptr);
    void reveal(const QPixmap &grid, const Theme &theme);
    void setSnapshot(const QPixmap &grid, const Theme &theme);
    void clear();
    qreal progress() const { return progress_; }
    void setProgress(qreal progress);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Theme theme_;
    QPixmap blurredGrid_;
    QPropertyAnimation animation_;
    qreal progress_ = 0;
};

}
