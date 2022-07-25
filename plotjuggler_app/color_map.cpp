/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "color_map.h"
#include <QSettings>

sol::protected_function_result ColorMap::setScrip(QString text)
{
  _lua_function = {};
  _lua_engine = {};
  _lua_engine.open_libraries();
  auto func = QString("function ColorMap(v)\n"
                      "%1\n"
                      "end\n")
                  .arg(text);
  auto result = _lua_engine.safe_script(func.toStdString());
  if (result.valid())
  {
    _script = text;
    _lua_function = _lua_engine["ColorMap"];
  }

  return result;
}

QString ColorMap::getError(sol::error err) const
{
  return QString(err.what());
}

QColor ColorMap::mapColor(double value) const
{
  auto res = _lua_function(value);
  if (!res.valid())
  {
    return Qt::transparent;
  }
  if (res.return_count() == 1 && res.get_type(0) == sol::type::string)
  {
    return QColor(res.get<std::string>(0).c_str());
  }
  return Qt::transparent;
}

std::map<QString, ColorMap::Ptr>& ColorMapLibrary()
{
  static std::map<QString, ColorMap::Ptr> colormaps;
  return colormaps;
}

void SaveColorMapToSettings()
{
  QSettings settings;
  QMap<QString, QVariant> colormap_text;
  for (const auto& it : ColorMapLibrary())
  {
    colormap_text.insert(it.first, it.second->script());
  }
  settings.setValue("ColorMapLibrary", colormap_text);
}

void LoadColorMapFromSettings()
{
  QSettings settings;

  ColorMapLibrary().clear();

  QMap<QString, QVariant> colormap_text = settings.value("ColorMapLibrary").toMap();
  for (const auto& key : colormap_text.keys())
  {
    QString script = colormap_text[key].toString();
    auto colormap = std::make_shared<ColorMap>();
    auto res = colormap->setScrip(script);
    if (res.valid())
    {
      ColorMapLibrary().insert({ key, colormap });
    }
  }
}
