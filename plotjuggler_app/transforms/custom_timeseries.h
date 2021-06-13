#ifndef CUSTOM_TIMESERIES_H
#define CUSTOM_TIMESERIES_H

#include "timeseries_qwt.h"
#include "custom_function.h"
#include "PlotJuggler/plotdata.h"

class CustomTimeseries : public TransformedTimeseries
{
public:
  CustomTimeseries(const PlotData* source_data,
                   const SnippetData& snippet,
                   PlotDataMapRef& mapped_data);

  bool updateCache(bool reset_old_data) override;

private:
  std::unique_ptr<CustomFunction> _transform;
  const PlotDataMapRef& _mapped_data;
};

#endif  // CUSTOM_TIMESERIES_H
