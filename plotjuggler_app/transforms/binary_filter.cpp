#include "binary_filter.h"
#include <QDoubleValidator>
#include "ui_scale_transform.h"
#include "utils.h"

BinaryFilter::BinaryFilter() : _widget(new QWidget()), ui(new Ui::BinaryFilter)
{
  ui->setupUi(_widget);

  double min = std::numeric_limits<double>::lowest();
  double max = std::numeric_limits<double>::max();

  ui->lineEditA->setValidator(new QDoubleValidator(min, max, 6, ui->lineEditA));
  ui->lineEditB->setValidator(new QDoubleValidator(min, max, 6, ui->lineEditB));

  connect(ui->lineEditA, &QLineEdit::editingFinished, this, [=]() {
    _value_A = ui->lineEditA->text().toDouble();
    emit parametersChanged();
  });

  connect(ui->lineEditB, &QLineEdit::editingFinished, this, [=]() {
    _value_B = ui->lineEditB->text().toDouble();
    swapRanges();
    emit parametersChanged();
  });

  auto emit_signal = [this](bool checked) {
    if (checked)
    {
      emit parametersChanged();
    }
  };

  connect(ui->radioEqual, &QRadioButton::toggled, this, emit_signal);
  connect(ui->radioLess, &QRadioButton::toggled, this, emit_signal);
  connect(ui->radioLessEq, &QRadioButton::toggled, this, emit_signal);
  connect(ui->radioGreater, &QRadioButton::toggled, this, emit_signal);
  connect(ui->radioGreaterEq, &QRadioButton::toggled, this, emit_signal);

  connect(ui->radioRange, &QRadioButton::toggled, this, [=](bool checked) {
    ui->lineEditB->setHidden(!checked);
    swapRanges();
    emit parametersChanged();
  });

  _value_A = ui->lineEditA->text().toDouble();
  _value_B = ui->lineEditB->text().toDouble();
  ui->lineEditB->setHidden(!ui->radioRange->isChecked());
}

BinaryFilter::~BinaryFilter()
{
  delete ui;
  delete _widget;
}

std::optional<PJ::PlotData::Point> BinaryFilter::calculateNextPoint(size_t index)
{
  const auto& p = dataSource()->at(index);
  if (ui->radioEqual->isChecked())
  {
    return PJ::PlotData::Point{ p.x, PJ::isEqual(p.y, _value_A) ? 1.0 : 0.0 };
  }
  if (ui->radioLess->isChecked())
  {
    return PJ::PlotData::Point{ p.x, (p.y < _value_A) ? 1.0 : 0.0 };
  }
  if (ui->radioLessEq->isChecked())
  {
    return PJ::PlotData::Point{ p.x, (p.y <= _value_A) ? 1.0 : 0.0 };
  }
  if (ui->radioGreater->isChecked())
  {
    return PJ::PlotData::Point{ p.x, (p.y > _value_A) ? 1.0 : 0.0 };
  }
  if (ui->radioGreaterEq->isChecked())
  {
    return PJ::PlotData::Point{ p.x, (p.y >= _value_A) ? 1.0 : 0.0 };
  }
  if (ui->radioRange->isChecked())
  {
    return PJ::PlotData::Point{ p.x, (p.y >= _value_A && p.y <= _value_B) ? 1.0 : 0.0 };
  }

  return std::nullopt;
}

void BinaryFilter::swapRanges()
{
  if (ui->radioRange->isChecked() && _value_B < _value_A)
  {
    const QSignalBlocker blockerA(ui->lineEditA);
    const QSignalBlocker blockerB(ui->lineEditB);
    QString tmpB = ui->lineEditB->text();
    ui->lineEditB->setText(ui->lineEditA->text());
    ui->lineEditA->setText(tmpB);
    std::swap(_value_A, _value_B);
  }
}

bool BinaryFilter::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement widget_el = doc.createElement("options");
  if (ui->radioEqual->isChecked())
  {
    widget_el.setAttribute("radioChecked", "Equal");
  }
  else if (ui->radioLess->isChecked())
  {
    widget_el.setAttribute("radioChecked", "Less");
  }
  else if (ui->radioLessEq->isChecked())
  {
    widget_el.setAttribute("radioChecked", "LessEq");
  }
  else if (ui->radioGreater->isChecked())
  {
    widget_el.setAttribute("radioChecked", "Greater");
  }
  else if (ui->radioGreaterEq->isChecked())
  {
    widget_el.setAttribute("radioChecked", "GreaterEq");
  }
  else if (ui->radioRange->isChecked())
  {
    widget_el.setAttribute("radioChecked", "Range");
  }

  widget_el.setAttribute("valueA", ui->lineEditA->text());
  widget_el.setAttribute("valueB", ui->lineEditB->text());
  return true;
}

bool BinaryFilter::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement widget_el = parent_element.firstChildElement("options");
  if (widget_el.isNull())
  {
    return false;
  }

  QString radio = widget_el.attribute("radioChecked");
  if (radio == "Equal")
  {
    ui->radioEqual->setChecked(true);
  }
  else if (radio == "Less")
  {
    ui->radioLess->setChecked(true);
  }
  else if (radio == "LessEq")
  {
    ui->radioLessEq->setChecked(true);
  }
  else if (radio == "Greater")
  {
    ui->radioGreater->setChecked(true);
  }
  else if (radio == "GreaterEq")
  {
    ui->radioGreaterEq->setChecked(true);
  }
  else if (radio == "Range")
  {
    ui->radioRange->setChecked(true);
  }
  ui->lineEditA->setText(widget_el.attribute("valueA"));
  ui->lineEditB->setText(widget_el.attribute("valueB"));
  return true;
}
