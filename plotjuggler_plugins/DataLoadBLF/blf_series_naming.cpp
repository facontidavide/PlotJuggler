#include "blf_series_naming.h"

#include <iomanip>
#include <stdexcept>
#include <sstream>

namespace PJ::BLF
{
namespace
{
constexpr uint32_t kStandardCanIdMask = 0x7FFU;
constexpr uint32_t kExtendedCanIdMask = 0x1FFFFFFFU;
constexpr uint8_t kMaxCanFdPayloadIndex = 63;
constexpr int kStandardCanIdWidth = 3;
constexpr int kExtendedCanIdWidth = 8;
constexpr int kEscapeWidth = 2;

bool IsSafeTokenChar(unsigned char ch)
{
  const bool is_upper = (ch >= 'A' && ch <= 'Z');
  const bool is_lower = (ch >= 'a' && ch <= 'z');
  const bool is_digit = (ch >= '0' && ch <= '9');
  return is_upper || is_lower || is_digit || ch == '_' || ch == '-' || ch == '.';
}

std::string EscapeSeriesToken(const std::string& token)
{
  std::ostringstream out;
  for (unsigned char ch : token)
  {
    if (IsSafeTokenChar(ch))
    {
      out << static_cast<char>(ch);
      continue;
    }
    out << '%' << std::uppercase << std::hex << std::setw(kEscapeWidth)
        << std::setfill('0') << static_cast<unsigned int>(ch) << std::dec;
  }
  return out.str();
}
}  // namespace

std::string ToCanIdHex(uint32_t can_id, bool extended)
{
  const uint32_t masked_id =
      extended ? (can_id & kExtendedCanIdMask) : (can_id & kStandardCanIdMask);
  const int width = extended ? kExtendedCanIdWidth : kStandardCanIdWidth;

  std::ostringstream out;
  out << "0x" << std::uppercase << std::hex << std::setw(width) << std::setfill('0')
      << masked_id;
  return out.str();
}

std::string RawSeriesPrefix(const BlfFrameKey& key)
{
  return "raw/can" + std::to_string(key.channel) + "/" + ToCanIdHex(key.can_id, key.extended);
}

std::string RawByteSeriesName(const BlfFrameKey& key, std::size_t byte_index)
{
  if (byte_index > static_cast<std::size_t>(kMaxCanFdPayloadIndex))
  {
    throw std::out_of_range("byte_index must be <= 63");
  }
  std::ostringstream out;
  out << RawSeriesPrefix(key) << "/data_" << std::setw(2) << std::setfill('0') << std::dec
      << static_cast<unsigned>(byte_index);
  return out.str();
}

std::string DecodedSeriesName(uint32_t channel, const std::string& message,
                              const std::string& signal)
{
  return "dbc/can" + std::to_string(channel) + "/" + EscapeSeriesToken(message) + "/" +
         EscapeSeriesToken(signal);
}

}  // namespace PJ::BLF
