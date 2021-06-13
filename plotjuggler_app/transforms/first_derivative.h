#ifndef FIRST_DERIVATIVE_H
#define FIRST_DERIVATIVE_H

#include <QLineEdit>
#include "PlotJuggler/transform_function.h"
#include "ui_first_derivative.h"

using namespace PJ;

class FirstDerivative: public TimeSeriesTransform
{
public:
  FirstDerivative();

  ~FirstDerivative() override;

  const char* name() const override { return "Derivative"; }

  QWidget* optionsWidget() override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

  void on_buttonCompute_clicked();

private:
  std::optional<PlotData::Point>
  calculateNextPoint(size_t index) override;

  QWidget *_widget;
  Ui::FirstDerivariveForm* ui;
  double _dT;

};

#endif // FIRST_DERIVATIVE_H
