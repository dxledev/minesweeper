#pragma once

#include "core/theme.h"

namespace minesweeper {

struct BoardPalette {
    QColor coveredBorder;
    QColor revealedBorder;
    QColor flaggedBorder;
    QColor hoveredBorder;
};

BoardPalette boardPalette(const Theme &theme);

}
