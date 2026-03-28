#pragma once

#include <functional>

#include <QString>

#include "blf_decoder.h"

namespace PJ::BLF
{

class BlfReader
{
public:
  bool ReadFrames(const QString& path,
                  const std::function<void(const NormalizedCanFrame&)>& on_frame,
                  QString& error) const;
};

}  // namespace PJ::BLF
