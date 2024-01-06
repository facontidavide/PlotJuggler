#include "moving_variance.h"
#include "ui_moving_variance.h"
#include <QCheckBox>

MovingVarianceFilter::MovingVarianceFilter()
  : ui(new Ui::MovingVarianceFilter)
  , _widget(new QWidget())
  , _buffer(1)
  , _ring_view(_buffer.begin(), _buffer.end())
{
  ui->setupUi(_widget);

  connect(ui->spinBoxSamples, qOverload<int>(&QSpinBox::valueChanged), this,
          [=](int) { emit parametersChanged(); });

  connect(ui->checkBoxStdDev, &QCheckBox::toggled, this,
          [=]() { emit parametersChanged(); });
}

MovingVarianceFilter::~MovingVarianceFilter()
{
  delete ui;
  delete _widget;
}

void MovingVarianceFilter::reset()
{
  _buffer.clear();
  TransformFunction_SISO::reset();
}

std::optional<PlotData::Point> MovingVarianceFilter::calculateNextPoint(size_t index)
{
  size_t buffer_size =
      std::min(size_t(ui->spinBoxSamples->value()), size_t(dataSource()->size()));
  if (buffer_size != _buffer.size())
  {
    _buffer.resize(buffer_size);
    _ring_view = nonstd::ring_span<PlotData::Point>(_buffer.begin(), _buffer.end());
  }

  const auto& p = dataSource()->at(index);
  _ring_view.push_back(p);

  while (_ring_view.size() < buffer_size)
  {
    _ring_view.push_back(p);
  }

  double total = 0;
  for (const auto& point: _ring_view)
  {
    total += point.y;
  }
  const double N = double(_ring_view.size());
  const double avg = total / N;

  double total_sqr = 0;
  for (const auto& point: _ring_view)
  {
    const auto v = point.y - avg;
    total_sqr += v*v;
  }

  if(ui->checkBoxStdDev->isChecked())
  {
    return PlotData::Point{ p.x, std::sqrt(total_sqr / N)};
  }
  return PlotData::Point{ p.x, total_sqr / N};
}

QWidget* MovingVarianceFilter::optionsWidget()
{
  return _widget;
}

bool MovingVarianceFilter::xmlSaveState(QDomDocument& doc,
                                       QDomElement& parent_element) const
{
  QDomElement widget_el = doc.createElement("options");
  widget_el.setAttribute("value", ui->spinBoxSamples->value());
  widget_el.setAttribute("apply_sqrt",
                         ui->checkBoxStdDev->isChecked() ? "true" : "false");
  parent_element.appendChild(widget_el);
  return true;
}

bool MovingVarianceFilter::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement widget_el = parent_element.firstChildElement("options");
  ui->spinBoxSamples->setValue(widget_el.attribute("value").toInt());
  bool checked = widget_el.attribute("apply_sqrt") == "true";
  ui->checkBoxStdDev->setChecked(checked);
  return true;
}
