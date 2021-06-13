#include "outlier_removal.h"
#include "ui_outlier_removal.h"

OutlierRemovalFilter::OutlierRemovalFilter():
  ui(new Ui::OutlierRemovalFilter),
  _widget(new QWidget()),
  _buffer(4),
  _ring_view( _buffer.begin(), _buffer.end() )
{
  ui->setupUi(_widget);

  connect(ui->spinBoxFactor, qOverload<double>(&QDoubleSpinBox::valueChanged),
          this, [=](int){ emit parametersChanged();  } );
}

OutlierRemovalFilter::~OutlierRemovalFilter()
{
  delete ui;
  delete _widget;
}


QWidget *OutlierRemovalFilter::optionsWidget()
{
  return _widget;
}

bool OutlierRemovalFilter::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
  QDomElement widget_el = doc.createElement("options");
  widget_el.setAttribute("factor",  ui->spinBoxFactor->value() );
  parent_element.appendChild( widget_el );
  return true;
}

bool OutlierRemovalFilter::xmlLoadState(const QDomElement &parent_element)
{
  QDomElement widget_el = parent_element.firstChildElement("options");
  ui->spinBoxFactor->setValue( widget_el.attribute("value", "100.0").toDouble() );
  return true;
}

std::optional<PJ::PlotData::Point>
OutlierRemovalFilter::calculateNextPoint(size_t index)
{
  const auto& p = dataSource()->at(index);
  _ring_view.push_back(p.y);

  if( index <= 2 )
  {
    return p;
  }

  if( index == 3 ) // skip this
  {
    return {};
  }

  double d1 = (_ring_view[1] -  _ring_view[2]);
  double d2 = (_ring_view[2] -  _ring_view[3]);
  if( d1*d2 < 0 ) // spike
  {
    double min_y = _ring_view[0];
    double max_y = _ring_view[0];

    min_y = std::min(min_y, _ring_view[1]);
    min_y = std::min(min_y, _ring_view[3]);

    max_y = std::max(max_y, _ring_view[1]);
    max_y = std::max(max_y, _ring_view[3]);

    double thresh = (max_y - min_y) * ui->spinBoxFactor->value();

    double jump = std::max(std::abs(d1), std::abs(d2));
    if( jump > thresh )
    {
      return {};
    }
  }
  return dataSource()->at(index-1);
}
