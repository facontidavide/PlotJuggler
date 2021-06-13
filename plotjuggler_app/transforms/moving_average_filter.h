#pragma once

#include <QRadioButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include "PlotJuggler/transform_function.h"
#include "ui_moving_average_filter.h"
#include "PlotJuggler/ring_span.hpp"

using namespace PJ;


namespace Ui {
class MovingAverageFilter;
}

class MovingAverageFilter: public TimeSeriesTransform
{
public:
  explicit MovingAverageFilter();

  ~MovingAverageFilter() override;

  void init() override;

  const char* name() const override { return "Moving Average"; }

  QWidget* optionsWidget() override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

private:
  Ui::MovingAverageFilter *ui;
  QWidget *_widget;
  std::vector<PlotData::Point> _buffer;
  nonstd::ring_span_lite::ring_span<PlotData::Point> _ring_view;

  std::optional<PlotData::Point>
  calculateNextPoint(size_t index) override;
};

