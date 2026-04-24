#pragma once

#include <QColor>

// Status accent colors. Used for badges, error text, and the query-bar
// validity indicator. Intentionally not QPalette roles — they must read the
// same in both light and dark PlotJuggler themes.
inline const QColor kErrorRed{0xd3, 0x2f, 0x2f};      // badge / size overrun
inline const QColor kErrorAccent{0xef, 0x53, 0x50};   // query-bar invalid
inline const QColor kSuccessGreen{0x66, 0xbb, 0x6a};  // query-bar valid
