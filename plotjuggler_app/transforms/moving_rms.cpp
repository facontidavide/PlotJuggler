#include "moving_rms.h"
#include "ui_moving_rms.h"

MovingRMS::MovingRMS()
  : ui(new Ui::MovingRMS)
  , _widget(new QWidget())
  , _buffer(1)
  , _ring_view(_buffer.begin(), _buffer.end())
{
  ui->setupUi(_widget);

  connect(ui->spinBoxSamples, qOverload<int>(&QSpinBox::valueChanged), this,
          [=](int) { emit parametersChanged(); });
}

MovingRMS::~MovingRMS()
{
  delete ui;
  delete _widget;
}

void MovingRMS::reset()
{
  _buffer.clear();
  TransformFunction_SISO::reset();
}

QWidget* MovingRMS::optionsWidget()
{
  return _widget;
}

bool MovingRMS::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement widget_el = doc.createElement("options");
  widget_el.setAttribute("value", ui->spinBoxSamples->value());
  parent_element.appendChild(widget_el);
  return true;
}

bool MovingRMS::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement widget_el = parent_element.firstChildElement("options");
  if (widget_el.isNull())
  {
    return false;
  }
  ui->spinBoxSamples->setValue(widget_el.attribute("value").toInt());
  return true;
}

std::optional<PJ::PlotData::Point> MovingRMS::calculateNextPoint(size_t index)
{
  size_t buffer_size = std::min(size_t(ui->spinBoxSamples->value()), size_t(dataSource()->size()));
  if (buffer_size != _buffer.size())
  {
    _buffer.resize(buffer_size);
    _ring_view = nonstd::ring_span<PJ::PlotData::Point>(_buffer.begin(), _buffer.end());
  }

  const auto& p = dataSource()->at(index);
  _ring_view.push_back(p);

  while (_ring_view.size() < buffer_size)
  {
    _ring_view.push_back(p);
  }

  double total_sqr = 0;

  for (size_t i = 0; i < buffer_size; i++)
  {
    double val = _ring_view[i].y;
    total_sqr += val * val;
  }

  double time = p.x;

  PJ::PlotData::Point out = { time, sqrt(total_sqr / _ring_view.size()) };
  return out;
}
