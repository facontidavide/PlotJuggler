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

bool IsPlausibleUnixEpochSeconds(double seconds);
qint64 UnixEpochSecondsToMsec(double seconds, bool* ok);

class BlfReader
{
public:
  bool ReadFrames(const QString& path,
                  const std::function<void(const NormalizedCanFrame&)>& on_frame,
                  QString& error,
                  BlfLoadMetadata* metadata = nullptr) const;
};

}  // namespace PJ::BLF
