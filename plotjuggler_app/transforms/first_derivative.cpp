#include "first_derivative.h"
#include <QFormLayout>
#include <QDoubleValidator>

FirstDerivative::FirstDerivative():
  _widget(new QWidget()),
  ui(new Ui::FirstDerivariveForm),
  _dT(0.0)
{
  ui->setupUi(_widget);
  ui->lineEditCustom->setValidator( new QDoubleValidator(0.0001, 1000, 4, ui->lineEditCustom) );

  connect(ui->buttonCompute, &QPushButton::clicked,
          this, &FirstDerivative::on_buttonCompute_clicked );

  connect(ui->lineEditCustom, &QLineEdit::editingFinished,
          this, [=](){
            _dT = ui->lineEditCustom->text().toDouble();
            emit parametersChanged();
          } );

  connect(ui->radioActual, &QRadioButton::toggled,
          this, [=](bool toggled){
            if( toggled) {
              _dT = 0.0;
              emit parametersChanged();
            }
          } );

  connect(ui->radioCustom, &QRadioButton::toggled,
          this, [=](bool toggled){
            if( toggled) {
              _dT = ui->lineEditCustom->text().toDouble();
              emit parametersChanged();
            }
          } );
}

FirstDerivative::~FirstDerivative()
{
  delete ui;
  delete _widget;
}

std::optional<PlotData::Point>
FirstDerivative::calculateNextPoint(size_t index)
{
  if( index == 0)
  {
    return {};
  }

  const auto& prev = dataSource()->at(index-1);
  const auto& p = dataSource()->at(index);

  double dt = (_dT == 0.0) ? (p.x - prev.x) : _dT;

  if( dt <= 0 )
  {
    return {};
  }

  double der = (p.y - prev.y) / dt;
  PlotData::Point out = {prev.x, der};
  return out;
}

QWidget *FirstDerivative::optionsWidget()
{
  const size_t data_size = dataSource()->size();

  if(!dataSource() || data_size < 2)
  {
    _widget->setEnabled(false);
  }
  return _widget;
}

bool FirstDerivative::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
  QDomElement widget_el = doc.createElement("options");

  if( ui->radioActual->isChecked() )
  {
    widget_el.setAttribute("radioChecked", "radioActual");
  }
  else{
    widget_el.setAttribute("radioChecked", "radioCustom");
  }
  widget_el.setAttribute("lineEdit", ui->lineEditCustom->text() );

  parent_element.appendChild( widget_el );
  return true;
}

bool FirstDerivative::xmlLoadState(const QDomElement &parent_element)
{
  QDomElement widget_el = parent_element.firstChildElement("options");

  ui->lineEditCustom->setText( widget_el.attribute("lineEdit") );

  if( widget_el.attribute("radioChecked") == "radioActual")
  {
    ui->radioActual->setChecked(true);
  }
  else{
    ui->radioCustom->setChecked(true);
  }
  return true;
}

void FirstDerivative::on_buttonCompute_clicked()
{
  if(!dataSource() || dataSource()->size() < 2)
  {
    return;
  }

  const size_t data_size = dataSource()->size();

  // calculate automatic diff
  std::vector<double> diff;
  diff.reserve(data_size-1);
  double prev_t = dataSource()->at(0).x;
  for(size_t i=1; i<data_size; i++)
  {
    double t = dataSource()->at(i).x;
    double delta = t - prev_t;
    prev_t = t;
    diff.push_back(delta);
  }

  size_t first = 0;
  size_t last = diff.size();
  if( data_size > 10 )
  {
    std::sort(diff.begin(), diff.end());
    first = last / 5;
    last = (last*4)/5;
  }
  double total = 0;
  for(size_t i=first; i<last; i++) {
    total += diff[i];
  }
  double estimated_dt = total / static_cast<double>(last-first);
  ui->lineEditCustom->setText(QString::number(estimated_dt, 'g', 4));

  if( ui->radioCustom->isChecked() )
  {
    _dT = estimated_dt;
    emit parametersChanged();
  }
}
