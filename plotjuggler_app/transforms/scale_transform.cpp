#include "scale_transform.h"
#include "ui_scale_transform.h"

ScaleTransform::ScaleTransform() :
  _widget(new QWidget()),
  ui(new Ui::ScaleTransform)
{
  ui->setupUi(_widget);

//  ui->lineEditValue->setValidator( new QDoubleValidator() );

  connect(ui->buttonDegRad, &QPushButton::clicked,
          this, [=](){
            const double deg_rad = 3.14159265359 / 180;
            ui->lineEditValueScale->setText( QString::number(deg_rad, 'g', 5));
            emit parametersChanged();
          } );

  connect(ui->buttonRadDeg, &QPushButton::clicked,
          this, [=](){
            const double rad_deg = 180.0 / 3.14159265359;
            ui->lineEditValueScale->setText( QString::number(rad_deg, 'g', 5));
            emit parametersChanged();
          } );

  connect(ui->lineEditTimeOffset, &QLineEdit::editingFinished,
          this, [=](){ emit parametersChanged();  } );
  connect(ui->lineEditValueOffset, &QLineEdit::editingFinished,
          this, [=](){ emit parametersChanged();  } );
  connect(ui->lineEditValueScale, &QLineEdit::editingFinished,
          this, [=](){ emit parametersChanged();  } );
}

ScaleTransform::~ScaleTransform()
{
  delete ui;
  delete _widget;
}

const char *ScaleTransform::name() const {
  return "Scale/Offset";
}

QWidget *ScaleTransform::optionsWidget()
{
  return _widget;
}

bool ScaleTransform::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
  QDomElement widget_el = doc.createElement("options");
  widget_el.setAttribute("time_offset",  ui->lineEditTimeOffset->text() );
  widget_el.setAttribute("value_offset", ui->lineEditValueOffset->text() );
  widget_el.setAttribute("value_scale",  ui->lineEditValueScale->text() );
  parent_element.appendChild( widget_el );

  return true;
}

bool ScaleTransform::xmlLoadState(const QDomElement &parent_element)
{
  QDomElement widget_el = parent_element.firstChildElement("options");
  ui->lineEditTimeOffset->setText( widget_el.attribute("time_offset") );
  ui->lineEditValueOffset->setText( widget_el.attribute("value_offset") );
  ui->lineEditValueScale->setText( widget_el.attribute("value_scale") );
  return true;
}

std::optional<PlotData::Point> ScaleTransform::calculateNextPoint(size_t index)
{
  double off_x = ui->lineEditTimeOffset->text().toDouble();
  double off_y = ui->lineEditValueOffset->text().toDouble();
  double scale = ui->lineEditValueScale->text().toDouble();

  const auto& p = dataSource()->at(index);
  PlotData::Point out = {p.x + off_x, scale * p.y + off_y};
  return out;
}
