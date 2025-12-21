#include "outlier_removal.h"
#include "ui_outlier_removal.h"

OutlierRemovalFilter::OutlierRemovalFilter()
  : ui(new Ui::OutlierRemovalFilter)
  , _widget(new QWidget())
  , _buffer(4)
  , _ring_view(_buffer.begin(), _buffer.end())
{
  ui->setupUi(_widget);

  connect(ui->spinBoxFactor, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
          [=](int) { emit parametersChanged(); });
}

OutlierRemovalFilter::~OutlierRemovalFilter()
{
  delete ui;
  delete _widget;
}

QWidget* OutlierRemovalFilter::optionsWidget()
{
  return _widget;
}

bool OutlierRemovalFilter::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement widget_el = doc.createElement("options");
  widget_el.setAttribute("value", ui->spinBoxFactor->value());
  parent_element.appendChild(widget_el);
  return true;
}

bool OutlierRemovalFilter::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement widget_el = parent_element.firstChildElement("options");
  if (widget_el.isNull())
  {
    return false;
  }
  ui->spinBoxFactor->setValue(widget_el.attribute("value", "100.0").toDouble());
  return true;
}

std::optional<PJ::PlotData::Point> OutlierRemovalFilter::calculateNextPoint(size_t index)
{
  const auto& p = dataSource()->at(index);
  _ring_view.push_back(p.y);

  if (index < 3)
  {
    return p;
  }

  double d1 = (_ring_view[1] - _ring_view[2]);
  double d2 = (_ring_view[2] - _ring_view[3]);
  if (d1 * d2 < 0)  // spike
  {
    double d0 = (_ring_view[0] - _ring_view[1]);
    double thresh = ui->spinBoxFactor->value();

    double jump = std::max(std::abs(d1), std::abs(d2));
    if (jump / std::abs(d0) > thresh)
    {
      return {};
    }
  }
  return dataSource()->at(index - 1);
}
