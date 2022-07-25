/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_SVG_UTIL_H
#define PJ_SVG_UTIL_H

#ifdef QT_NO_SVGRENDERER
#error "QT_NO_SVGRENDERER defined"
#endif

#include <QtSvg>
#include <QFile>
#include <QIcon>
#include <QTextStream>
#include <QByteArray>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

// Useful function to change the color of SVG icons programmatically.
// Useful to switch between dark view and light view.
// To work, the SVG file must use the color #ffffff and #000000 only.
inline const QPixmap& LoadSvg(QString filename, QString style_name = "light")
{
  static std::map<QString, QPixmap> light_images;
  static std::map<QString, QPixmap> dark_images;
  bool light_theme = style_name.contains("light");

  auto* stored_images = light_theme ? &light_images : &dark_images;

  auto it = stored_images->find(filename);
  if (it == stored_images->end())
  {
    QFile file(filename);
    file.open(QFile::ReadOnly | QFile::Text);
    auto svg_data = file.readAll();
    file.close();

    if (light_theme)
    {
      svg_data.replace("#000000", "#111111");
      svg_data.replace("#ffffff", "#dddddd");
    }
    else
    {
      svg_data.replace("#000000", "#dddddd");
      svg_data.replace("#ffffff", "#111111");
    }
    QByteArray content(svg_data);

    QSvgRenderer rr(content);
    QImage image(64, 64, QImage::Format_ARGB32);
    QPainter painter(&image);
    image.fill(Qt::transparent);
    rr.render(&painter);

    it = stored_images->insert({ filename, QPixmap::fromImage(image) }).first;
  }
  return it->second;
}

#endif  // PJ_SVG_UTIL_H
