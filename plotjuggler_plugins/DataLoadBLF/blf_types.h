#pragma once

#include <cstdint>

namespace PJ::BLF
{

struct BlfFrameKey
{
  uint32_t channel = 0;
  uint32_t can_id = 0;
  bool extended = false;
};

}  // namespace PJ::BLF
