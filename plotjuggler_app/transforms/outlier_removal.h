#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include "PlotJuggler/transform_function.h"
#include "ui_outlier_removal.h"
#include "PlotJuggler/ring_span.hpp"

using namespace PJ;


namespace Ui {
class OutlierRemovalFilter;
}

class OutlierRemovalFilter: public TimeSeriesTransform
{
public:
  explicit OutlierRemovalFilter();

  ~OutlierRemovalFilter() override;

  const char* name() const override { return "Outlier Removal"; }

  QWidget* optionsWidget() override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

private:
  Ui::OutlierRemovalFilter *ui;
  QWidget *_widget;
  std::vector<double> _buffer;
  nonstd::ring_span_lite::ring_span<double> _ring_view;

  std::optional<PlotData::Point>
  calculateNextPoint(size_t index) override;
};
