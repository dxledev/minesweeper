#include "board_palette.h"

#include <algorithm>
#include <cmath>

namespace minesweeper {
namespace {

double luminance(const QColor &color) {
    const auto linear = [](double component) {
        return component <= .04045 ? component / 12.92 : std::pow((component + .055) / 1.055, 2.4);
    };
    return .2126 * linear(color.redF()) + .7152 * linear(color.greenF()) + .0722 * linear(color.blueF());
}

double contrast(const QColor &color, const QColor &background) {
    const double foreground = luminance(color), surface = luminance(background);
    return (std::max(foreground, surface) + .05) / (std::min(foreground, surface) + .05);
}

QColor mix(const QColor &color, const QColor &target, double amount) {
    return QColor(qRound(color.red() * (1 - amount) + target.red() * amount),
                  qRound(color.green() * (1 - amount) + target.green() * amount),
                  qRound(color.blue() * (1 - amount) + target.blue() * amount));
}

QColor cellBorder(const Theme &theme, const QString &fillKey) {
    const auto grid = theme.color("grid"), surface = theme.color("surface"), fill = theme.color(fillKey);
    const auto score = [&](const QColor &color) {
        return std::min(contrast(color, surface), contrast(color, fill));
    };
    auto best = grid;
    double bestContrast = score(best);
    if (bestContrast >= 3)
        return best;
    for (int step = 1; step <= 100; ++step) {
        for (const auto &target : {QColor(Qt::white), QColor(Qt::black)}) {
            const auto candidate = mix(grid, target, step / 100.);
            const double candidateContrast = score(candidate);
            if (candidateContrast >= 3)
                return candidate;
            if (candidateContrast > bestContrast) {
                best = candidate;
                bestContrast = candidateContrast;
            }
        }
    }
    return best;
}

}

BoardPalette boardPalette(const Theme &theme) {
    return {cellBorder(theme, "surface_alt"), cellBorder(theme, "related"),
            cellBorder(theme, "matching"), cellBorder(theme, "selection")};
}

}
