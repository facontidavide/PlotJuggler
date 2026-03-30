#pragma once

#include <functional>
#include <string>
#include <vector>

#include <QString>

#include "blf_decoder.h"

namespace PJ::BLF
{

struct BlfLoadMetadata
{
  bool has_valid_absolute_time = false;
  qint64 first_absolute_time_msec = 0;
  std::vector<std::string> dbc_files;
};

struct BlfReadProgress
{
  uint32_t current_object = 0;
  uint32_t total_objects = 0;
  int percentage = 0;
};

bool IsPlausibleUnixEpochSeconds(double seconds);
qint64 UnixEpochSecondsToMsec(double seconds, bool* ok);
int ComputeBlfReadPercentage(uint32_t current_object, uint32_t total_objects);

class BlfReader
{
public:
  bool ReadFrames(const QString& path,
                  const std::function<void(const NormalizedCanFrame&)>& on_frame,
                  QString& error,
                  BlfLoadMetadata* metadata = nullptr,
                  const std::function<bool(const BlfReadProgress&)>& on_progress = {}) const;
};

}  // namespace PJ::BLF
