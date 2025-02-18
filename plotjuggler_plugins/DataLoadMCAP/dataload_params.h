#pragma once

#include <QStringList>

namespace mcap
{

struct LoadParams
{
  QStringList selected_topics;
  unsigned max_array_size;
  bool clamp_large_arrays;
  bool use_timestamp = false;
  bool use_mcap_log_time;
};

}  // namespace mcap
