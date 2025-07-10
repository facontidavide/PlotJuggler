#include "PlotJuggler/save_plot.h"

#include <qwt_plot.h>
#include <qwt_plot_renderer.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QString>
#include <QSvgGenerator>

auto toExtension(const QString& filter)
{
  return filter.split(" ").front().prepend('.');
}

void savePlotToFile(QSize dims, QwtPlot* plot, QWidget* parent)
{
  QFileDialog save_dialog(parent);
  save_dialog.setAcceptMode(QFileDialog::AcceptSave);

  QStringList filters;
  filters << "png (*.png)"
          << "jpg (*.jpg *.jpeg)"
          << "svg (*.svg)";

  QString selected_filter;
  auto save_filename = save_dialog.getSaveFileName(parent, "Save plot", "",
                                                   filters.join(";;"), &selected_filter);

  if (save_filename.isEmpty())
    return;

  if (QFileInfo(save_filename).suffix().isEmpty())
  {
    save_filename.append(toExtension(selected_filter));
  }
  auto is_svg = save_filename.endsWith(".svg");

  QwtPlotRenderer renderer;

  const auto rect = QRect{0, 0, dims.width(), dims.height()};
  if (is_svg)
  {
    QSvgGenerator generator;
    generator.setFileName(save_filename);
    generator.setResolution(80);
    generator.setViewBox(rect);
    QPainter painter(&generator);
    renderer.render(plot, &painter, rect);
  }
  else
  {
    QPixmap pixmap(dims.width(), dims.height());
    QPainter painter(&pixmap);
    renderer.render(plot, &painter, rect);
    pixmap.save(save_filename);
  }
}
