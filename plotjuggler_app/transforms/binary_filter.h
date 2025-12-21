#ifndef BINARY_FILTER_H
#define BINARY_FILTER_H

#include <QWidget>
#include "PlotJuggler/transform_function.h"
#include "ui_binary_filter.h"

namespace Ui
{
class BinaryFilter;
}

class BinaryFilter : public PJ::TransformFunction_SISO
{
public:
  BinaryFilter();
  ~BinaryFilter() override;

  static const char* transformName()
  {
    return "Binary Filter";
  }
  const char* name() const override
  {
    return BinaryFilter::transformName();
  }

  QWidget* optionsWidget() override
  {
    return _widget;
  }

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

private:
  QWidget* _widget;
  Ui::BinaryFilter* ui;

  std::optional<PJ::PlotData::Point> calculateNextPoint(size_t index) override;

  double _value_A;
  double _value_B;

  void swapRanges();
};

#endif  // BINARY_FILTER_H
