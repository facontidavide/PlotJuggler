#include "moving_average_filter.h"
#include "ui_moving_average_filter.h"
#include <numeric>
#include <QCheckBox>

MovingAverageFilter::MovingAverageFilter() :
  ui(new Ui::MovingAverageFilter),
  _widget(new QWidget()),
  _buffer(1),
  _ring_view( _buffer.begin(), _buffer.end() )
{
  ui->setupUi(_widget);

  connect(ui->spinBoxSamples, qOverload<int>(&QSpinBox::valueChanged),
          this, [=](int){ emit parametersChanged();  } );

  connect(ui->checkBoxTimeOffset, &QCheckBox::toggled,
          this, [=](){ emit parametersChanged();  } );
}

MovingAverageFilter::~MovingAverageFilter()
{
  delete ui;
  delete _widget;
}

void MovingAverageFilter::init()
{
  _buffer.clear();
  TimeSeriesTransform::init();
}

std::optional<PlotData::Point>
MovingAverageFilter::calculateNextPoint(size_t index)
{
  size_t buffer_size = std::min( int(ui->spinBoxSamples->value()), int(dataSource()->size()));
  if( buffer_size != _buffer.size() ) {
    _buffer.resize( buffer_size );
    _ring_view = nonstd::ring_span<PlotData::Point>(_buffer.begin(), _buffer.end());
  }

  const auto& p = dataSource()->at(index);
  _ring_view.push_back(p);

  while( _ring_view.size() < buffer_size )
  {
    _ring_view.push_back(p);
  }

  double total = 0;

  for(size_t i=0; i < buffer_size; i++)
  {
    total += _ring_view[i].y;
  }

  double time = p.x;
  if( ui->checkBoxTimeOffset->isChecked())
  {
    time = (_ring_view.back().x + _ring_view.front().x) / 2.0;
  }

  PlotData::Point out = {time, total / _ring_view.size() };
  return out;
}

QWidget *MovingAverageFilter::optionsWidget()
{
  return _widget;
}

bool MovingAverageFilter::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
  QDomElement widget_el = doc.createElement("options");
  widget_el.setAttribute("value", ui->spinBoxSamples->value());
  widget_el.setAttribute("compensate_offset", ui->checkBoxTimeOffset->isChecked() ? "true" : "false");
  parent_element.appendChild( widget_el );
  return true;
}

bool MovingAverageFilter::xmlLoadState(const QDomElement &parent_element)
{
  QDomElement widget_el = parent_element.firstChildElement("options");
  ui->spinBoxSamples->setValue( widget_el.attribute("value").toInt() );
  bool checked = widget_el.attribute("compensate_offset") == "true";
  ui->checkBoxTimeOffset->setChecked( checked );
  return true;
}
