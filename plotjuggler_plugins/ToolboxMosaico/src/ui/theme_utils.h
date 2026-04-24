#pragma once

#include <QApplication>

[[nodiscard]] inline bool isDarkTheme()
{
  return qApp->styleSheet().contains("style_dark");
}

[[nodiscard]] inline const char* themeName()
{
  return isDarkTheme() ? "dark" : "light";
}
