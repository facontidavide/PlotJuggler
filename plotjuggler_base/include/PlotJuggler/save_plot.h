/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_SAVE_PLOT_H
#define PJ_SAVE_PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_renderer.h>

#include <QPaintDevice>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>

#include <memory>

const static QSize default_document_dimentions{ 1200, 900 };

void savePlotToFile(QSize dims, QwtPlot* plot, QWidget* parent);

class PlotSaveHelper
{
public:
  PlotSaveHelper(QSize dims, QWidget* parent);
  ~PlotSaveHelper();

  void paint(QwtPlot* plot, QRect paint_at) const;
  void paintTitle(const QString& title, QRectF paint_at, QWidget* parent) const;

private:
  std::unique_ptr<QwtPlotRenderer> _renderer;
  std::unique_ptr<QPaintDevice> _paint_device;
  std::unique_ptr<QPainter> _painter;
  QString _save_filename;
};

#endif  // PJ_SAVE_PLOT_H
