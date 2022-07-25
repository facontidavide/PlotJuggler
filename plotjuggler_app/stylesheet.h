/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef STYLESHEET_H
#define STYLESHEET_H

#include <QString>
#include <map>
#include <QColor>
#include <QSettings>
#include <QApplication>
#include <QFile>

#include "QSyntaxStyle"

inline QString SetApplicationStyleSheet(QString style)
{
  QString out;
  QStringList lines = style.split("\n");

  std::map<QString, QString> palette;

  int i = 0;

  while (i < lines.size())
  {
    if (lines[i++].contains("PALETTE START"))
    {
      break;
    }
  }

  while (i < lines.size())
  {
    auto parts = lines[i].split(":");
    if (parts.size() == 2)
    {
      QString value = parts[1].remove(" ");
      value.remove("\r");
      palette.insert({ parts[0].remove(" "), value });
    }

    if (lines[i++].contains("PALETTE END"))
    {
      break;
    }
  }

  while (i < lines.size())
  {
    QString line = lines[i];

    int pos_start = line.indexOf("${", 0);
    if (pos_start != -1)
    {
      int pos_end = line.indexOf("}", pos_start);
      if (pos_end == -1)
      {
        throw std::runtime_error("problem loading stylesheet. Unclosed ${}");
      }
      int mid_length = pos_end - (pos_start + 2);
      QString id = line.mid(pos_start + 2, mid_length);

      if (palette.count(id) == 0)
      {
        std::string msg = "Problem loading stylesheet: can't find palette id: ";
        msg += id.toStdString();
        throw std::runtime_error(msg);
      }
      QString color = palette[id];
      line = line.left(pos_start) + color + line.right(line.size() - pos_end - 1);
    }

    out += line + "\n";
    i++;
  }

  QSettings settings;
  settings.setValue("StyleSheet::theme", palette["theme"]);

  dynamic_cast<QApplication*>(QCoreApplication::instance())->setStyleSheet(out);

  return palette["theme"];
}

inline QSyntaxStyle* GetLuaSyntaxStyle(QString theme)
{
  auto loadStyle = [](const char* path) -> QSyntaxStyle* {
    QFile fl(path);
    QSyntaxStyle* style = nullptr;
    if (fl.open(QIODevice::ReadOnly))
    {
      style = new QSyntaxStyle();
      if (!style->load(fl.readAll()))
      {
        delete style;
      }
    }
    return style;
  };

  static QSyntaxStyle* style[2] = { loadStyle(":/resources/lua_style_light.xml"),
                                    loadStyle(":/resources/lua_style_dark.xml") };

  return theme == "light" ? style[0] : style[1];
}

#endif  // STYLESHEET_H
