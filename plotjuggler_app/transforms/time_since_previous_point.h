#ifndef TIME_SINCE_LAST_DATA_POINT_TRANSFORM_H
#define TIME_SINCE_LAST_DATA_POINT_TRANSFORM_H

#include "PlotJuggler/transform_function.h"

using namespace PJ;

class TimeSincePreviousPointTranform : public TransformFunction_SISO
{
public:
  TimeSincePreviousPointTranform() = default;

  ~TimeSincePreviousPointTranform() override = default;

  static const char* transformName()
  {
    return "Time Since Previous Point";
  }

  const char* name() const override
  {
    return transformName();
  }

private:
  std::optional<PlotData::Point> calculateNextPoint(size_t index) override;
};

#endif  // TIME_SINCE_LAST_DATA_POINT_TRANSFORM_H
