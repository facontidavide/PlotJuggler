#include "plotlegend.h"
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QMarginsF>
#include "qwt_legend_data.h"
#include "qwt_graphic.h"
#include "qwt_text.h"

PlotLegend::PlotLegend(QwtPlot* parent) : _parent_plot(parent), _collapsed(false)
{
  setRenderHint(QwtPlotItem::RenderAntialiased);

  setMaxColumns(1);
  setAlignmentInCanvas(Qt::Alignment(Qt::AlignTop | Qt::AlignRight));
  setBackgroundMode(QwtPlotLegendItem::BackgroundMode::LegendBackground);;
  setBorderRadius(0);

  setMargin(2);
  setSpacing(1);
  setItemMargin(2);

  QFont font = this->font();
  font.setPointSize(9);
  setFont(font);
  setVisible(true);

  this->attach(parent);
}

QRectF PlotLegend::hideButtonRect() const
{
  const int s = 5;
  auto canvas_rect = _parent_plot->canvas()->rect();
  if (alignmentInCanvas() & Qt::AlignRight)
  {
    return QRectF(geometry(canvas_rect).topRight() + QPoint(-s, -s) , QSize(s*2, s*2));
  }
  return QRectF(geometry(canvas_rect).topLeft()  + QPoint(-s, -s), QSize(s*2, s*2));
}

void PlotLegend::draw(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& rect) const
{
  if (!_collapsed)
  {
    QwtPlotLegendItem::draw(painter, xMap, yMap, rect);
  }

  QRectF iconRect = hideButtonRect();

  if (isVisible() && plotItems().size() > 0)
  {
    painter->save();

    QColor col = _parent_plot->canvas()->palette().foreground().color();
    painter->setPen(col);
    painter->setBrush(QBrush(Qt::white, Qt::SolidPattern));
    painter->drawEllipse(iconRect);

    if( _collapsed ){
      iconRect -= QMarginsF(3,3,3,3);
      painter->setBrush(QBrush(col, Qt::SolidPattern));
      painter->drawEllipse(iconRect);
    }

    painter->restore();
  }
}

void PlotLegend::drawLegendData(QPainter* painter, const QwtPlotItem* plotItem, const QwtLegendData& data,
                                const QRectF& rect) const
{
  Q_UNUSED(plotItem);

  const int m = margin();
  const QRectF r = rect.toRect().adjusted(m, m, -m, -m);

  painter->setClipRect(r, Qt::IntersectClip);

  int titleOff = 0;

  const QwtGraphic graphic = data.icon();
  if (!graphic.isEmpty())
  {
    QRectF iconRect(r.topLeft(), graphic.defaultSize());

    iconRect.moveCenter(QPoint(iconRect.center().x(), rect.center().y()));

    if (plotItem->isVisible())
    {
      graphic.render(painter, iconRect, Qt::KeepAspectRatio);
    }

    titleOff += iconRect.width() + spacing();
  }

  const QwtText text = data.title();
  if (!text.isEmpty())
  {
    auto pen = textPen();
    if (!plotItem->isVisible())
    {
      pen.setColor(QColor(122, 122, 122));
    }
    else
    {
      pen.setColor(_parent_plot->canvas()->palette().foreground().color());
    }
    painter->setPen(pen);
    painter->setFont(font());

    const QRectF textRect = r.adjusted(titleOff, 0, 0, 0);
    text.draw(painter, textRect);
  }
}

void PlotLegend::drawBackground(QPainter* painter, const QRectF& rect) const
{
  painter->save();

  auto pen = textPen();
  pen.setColor(_parent_plot->canvas()->palette().foreground().color());

  painter->setPen(pen);
  painter->setBrush(backgroundBrush());
  const double radius = borderRadius();
  painter->drawRoundedRect(rect, radius, radius);

  painter->restore();
}

const QwtPlotItem* PlotLegend::processMousePressEvent(QMouseEvent* mouse_event)
{
  auto canvas_rect = _parent_plot->canvas()->rect();
  const QPoint press_point = mouse_event->pos();

  if (isVisible() && mouse_event->modifiers() == Qt::NoModifier)
  {
    if ( (hideButtonRect() + QMargins(2,2,2,2) ).contains(press_point))
    {
      _collapsed = !_collapsed;
      _parent_plot->replot();
      return nullptr;
    }

    if (!_collapsed && geometry(canvas_rect).contains(press_point))
    {
      for (auto item : plotItems())
      {
        auto item_rect = legendGeometries(item).first();
        if (item_rect.contains(press_point))
        {
          return item;
        }
      }
    }
  }
  return nullptr;
}
