#include "PlotJuggler/save_plot.h"

#include <qwt_painter.h>
#include <qwt_plot.h>
#include <qwt_plot_renderer.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QPaintDevice>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QString>
#include <QSvgGenerator>
#include <QTextOption>

#include <memory>

auto toExtension(const QString& filter)
{
  return filter.split(" ").front().prepend('.');
}

PlotSaveHelper::PlotSaveHelper(QSize dims, QWidget* parent)
  : _renderer{ std::make_unique<QwtPlotRenderer>(parent) }
{
  QFileDialog save_dialog(parent);
  save_dialog.setAcceptMode(QFileDialog::AcceptSave);

  QStringList filters;
  filters << "png (*.png)"
          << "jpg (*.jpg *.jpeg)"
          << "svg (*.svg)";

  QString selected_filter;
  _save_filename =
      save_dialog.getSaveFileName(parent, "Save plot", "", filters.join(";;"), &selected_filter);
  if (_save_filename.isEmpty())
    return;

  if (QFileInfo(_save_filename).suffix().isEmpty())
  {
    _save_filename.append(toExtension(selected_filter));
  }
  auto is_svg = _save_filename.endsWith(".svg");

  const auto rect = QRect(0, 0, dims.width(), dims.height());
  if (is_svg)
  {
    auto* generator = new QSvgGenerator();
    generator->setFileName(_save_filename);
    generator->setResolution(80);
    generator->setViewBox(rect);

    _paint_device = std::unique_ptr<QPaintDevice>(generator);
  }
  else
  {
    auto* pixmap = new QPixmap(dims.width(), dims.height());
    _paint_device = std::unique_ptr<QPaintDevice>(pixmap);
  }
  _painter = std::make_unique<QPainter>(_paint_device.get());

  _painter->fillRect(rect, parent->palette().window());
}

PlotSaveHelper::~PlotSaveHelper()
{
  if (const auto* pixmap = dynamic_cast<QPixmap*>(_paint_device.get()); pixmap)
  {
    pixmap->save(_save_filename);
  }
}

void PlotSaveHelper::paint(QwtPlot* plot, QRect paint_at) const
{
  static const auto margin = 5;
  if (_save_filename.isEmpty())
    return;

  paint_at.adjust(margin, margin, -margin, -margin);
  _renderer->render(plot, _painter.get(), paint_at);
}

void PlotSaveHelper::paintTitle(const QString& title, QRectF paint_at, QWidget* parent) const
{
  if (_save_filename.isEmpty())
    return;

  QFont font;
  _painter->setFont(parent->font());
  _painter->setPen(parent->palette().windowText().color());

  QTextOption text_options;
  text_options.setAlignment(Qt::AlignCenter);

  _painter->drawText(paint_at, title, text_options);
}

void savePlotToFile(QSize dims, QwtPlot* plot, QWidget* parent)
{
  PlotSaveHelper save_plots_helper(dims, parent);
  const auto rect = QRect(0, 0, dims.width(), dims.height());
  save_plots_helper.paint(plot, rect);
}
