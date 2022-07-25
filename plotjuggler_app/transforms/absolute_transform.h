#ifndef ABSOLUTE_TRANSFORM_H
#define ABSOLUTE_TRANSFORM_H

#include "PlotJuggler/transform_function.h"
#include "ui_integral_transform.h"

using namespace PJ;

class AbsoluteTransform : public TransformFunction_SISO
{
public:
  AbsoluteTransform() = default;

  ~AbsoluteTransform() override = default;

  const char* name() const override
  {
    return "Absolute";
  }

private:
  std::optional<PlotData::Point> calculateNextPoint(size_t index) override;
};

#endif  // ABSOLUTE_TRANSFORM_H
