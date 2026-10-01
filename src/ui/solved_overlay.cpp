#include "solved_overlay.h"

#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>

namespace minesweeper {

SolvedOverlay::SolvedOverlay(QWidget *parent)
    : QWidget(parent), animation_(this, "progress") {
    setObjectName("solvedOverlay");
    setAccessibleName("Solved");
    setAttribute(Qt::WA_TransparentForMouseEvents);
    animation_.setDuration(650);
    animation_.setEasingCurve(QEasingCurve::OutCubic);
    hide();
}

void SolvedOverlay::setSnapshot(const QPixmap &grid, const Theme &theme) {
    theme_ = theme;
    QGraphicsScene scene;
    auto *item = scene.addPixmap(grid);
    auto *blur = new QGraphicsBlurEffect;
    blur->setBlurRadius(8);
    blur->setBlurHints(QGraphicsBlurEffect::QualityHint);
    item->setGraphicsEffect(blur);
    blurredGrid_ = QPixmap(grid.size());
    blurredGrid_.fill(theme.color("surface"));
    QPainter painter(&blurredGrid_);
    scene.render(&painter, QRectF(blurredGrid_.rect()), QRectF(grid.rect()));
    update();
}

void SolvedOverlay::reveal(const QPixmap &grid, const Theme &theme) {
    animation_.stop();
    setSnapshot(grid, theme);
    setProgress(0);
    show();
    raise();
    animation_.setStartValue(0.0);
    animation_.setEndValue(1.0);
    animation_.start();
}

void SolvedOverlay::clear() {
    animation_.stop();
    hide();
    blurredGrid_ = {};
    progress_ = 0;
}

void SolvedOverlay::setProgress(qreal progress) {
    progress_ = progress;
    update();
}

void SolvedOverlay::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(rect()), 10, 10);
    painter.setClipPath(clip);
    painter.setOpacity(progress_);
    painter.drawPixmap(rect(), blurredGrid_);
    auto veil = theme_.color("surface");
    veil.setAlphaF(.4);
    painter.fillRect(rect(), veil);

    const auto center = QRectF(rect()).center();
    painter.translate(center.x(), center.y() + (1 - progress_) * 12);
    const qreal scale = .92 + progress_ * .08;
    painter.scale(scale, scale);
    auto solvedFont = font();
    solvedFont.setPixelSize(qRound(width() * .16));
    solvedFont.setWeight(QFont::Bold);
    painter.setFont(solvedFont);
    painter.setPen(theme_.color("accent"));
    painter.drawText(QRectF(-width() / 2.0, -height() / 2.0, width(), height()),
                     Qt::AlignCenter, "Solved");
}

}
