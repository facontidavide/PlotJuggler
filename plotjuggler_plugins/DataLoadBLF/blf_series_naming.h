#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "blf_types.h"

namespace PJ::BLF
{

std::string ToCanIdHex(uint32_t can_id, bool extended);
std::string RawSeriesPrefix(const BlfFrameKey& key);
// Contract: byte_index must be in [0, 63] for CAN FD payload bytes.
// Throws std::out_of_range if violated.
std::string RawByteSeriesName(const BlfFrameKey& key, std::size_t byte_index);
std::string DecodedSeriesName(uint32_t channel, const std::string& message,
                              const std::string& signal);

}  // namespace PJ::BLF
