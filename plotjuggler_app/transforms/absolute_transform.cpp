#include "absolute_transform.h"
#include <QFormLayout>
#include <QDoubleValidator>

std::optional<PlotData::Point> AbsoluteTransform::calculateNextPoint(size_t index)
{
  const auto& p = dataSource()->at(index);
  PlotData::Point out = { p.x, std::abs(p.y) };
  return out;
}
