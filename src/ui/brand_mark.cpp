#include "brand_mark.h"

#include <QPainter>
#include <QPainterPath>

namespace minesweeper {

QPixmap brandMark(const Theme &theme, qreal scale, int size) {
    QPixmap result(qRound(size * scale), qRound(size * scale));
    result.setDevicePixelRatio(scale);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(size / 40., size / 40.);
    painter.setPen(Qt::NoPen);
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            painter.setBrush(theme.color(row == 1 && column == 1 ? "accent" : "selection"));
            painter.drawRoundedRect(QRectF(column * 13 + 1, row * 13 + 1, 10, 10), 3, 3);
        }
    }
    painter.setPen(QPen(theme.color("accent_text"), 1.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    QPainterPath flag;
    flag.moveTo(17, 22);
    flag.lineTo(17, 16);
    flag.lineTo(22, 18);
    flag.lineTo(17, 20);
    flag.moveTo(15.5, 22);
    flag.lineTo(19.5, 22);
    painter.drawPath(flag);
    return result;
}

}
