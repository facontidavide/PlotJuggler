#pragma once

#include <QRadioButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include "PlotJuggler/transform_function.h"
#include "ui_moving_variance.h"
#include "PlotJuggler/ring_span.hpp"

using namespace PJ;

namespace Ui
{
class MovingVarianceFilter;
}

class MovingVarianceFilter : public TransformFunction_SISO
{
public:
  explicit MovingVarianceFilter();

  ~MovingVarianceFilter() override;

  void reset() override;

  const char* name() const override
  {
    return "Moving Variance";
  }

  QWidget* optionsWidget() override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

private:
  Ui::MovingVarianceFilter* ui;
  QWidget* _widget;
  std::vector<PlotData::Point> _buffer;
  nonstd::ring_span_lite::ring_span<PlotData::Point> _ring_view;

  std::optional<PlotData::Point> calculateNextPoint(size_t index) override;
};
