#include "dbcppp_decoder.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <utility>

#if defined(PJ_HAS_DBCPPP) && PJ_HAS_DBCPPP
#if __has_include(<dbcppp/Network.h>) && __has_include(<dbcppp/Message.h>) && \
    __has_include(<dbcppp/Signal.h>)
#include <dbcppp/Message.h>
#include <dbcppp/Network.h>
#include <dbcppp/Signal.h>
#define PJ_CAN_USE_DBCPPP 1
#endif
#endif

#if !defined(PJ_CAN_USE_DBCPPP)
#define PJ_CAN_USE_DBCPPP 0
#endif

namespace PJ::BLF
{
namespace
{

constexpr uint64_t kStandardCanIdMask = 0x7FFULL;
constexpr uint64_t kExtendedCanIdMask = 0x1FFFFFFFULL;
constexpr uint64_t kExtendedCanIdFlag = 0x80000000ULL;

void SetError(std::string* error, const std::string& message)
{
  if (error)
  {
    *error = message;
  }
}

uint64_t NormalizeCanId(uint32_t can_id, bool extended)
{
  return static_cast<uint64_t>(can_id) & (extended ? kExtendedCanIdMask : kStandardCanIdMask);
}

template <typename T>
const T* ToPtr(const T& value)
{
  return &value;
}

template <typename T>
const T* ToPtr(const T* value)
{
  return value;
}

#if PJ_CAN_USE_DBCPPP
uint8_t DlcToPayloadSize(uint8_t dlc)
{
  static constexpr uint8_t kDlcToSize[16] = {0U,  1U,  2U,  3U,  4U,  5U,  6U,  7U,
                                              8U,  12U, 16U, 20U, 24U, 32U, 48U, 64U};
  return kDlcToSize[dlc & 0x0FU];
}

const dbcppp::IMessage* SelectMessageByPayload(const std::vector<const dbcppp::IMessage*>& messages,
                                               std::size_t payload_size, bool is_fd)
{
  const dbcppp::IMessage* first_compatible = nullptr;
  const dbcppp::IMessage* best_fit = nullptr;
  uint64_t best_fit_size = 0U;

  for (const auto* message : messages)
  {
    if (!message)
    {
      continue;
    }

    const uint64_t message_size = message->MessageSize();
    if (!is_fd && message_size > 8U)
    {
      continue;
    }

    if (!first_compatible)
    {
      first_compatible = message;
    }

    if (message_size == payload_size)
    {
      return message;
    }

    if (message_size <= payload_size && message_size >= best_fit_size)
    {
      best_fit = message;
      best_fit_size = message_size;
    }
  }

  if (best_fit)
  {
    return best_fit;
  }

  return first_compatible;
}
#endif

}  // namespace

struct DbcpppDecoder::Impl
{
#if PJ_CAN_USE_DBCPPP
  std::unique_ptr<dbcppp::INetwork> network;
  std::unordered_map<uint64_t, std::vector<const dbcppp::IMessage*>> messages_by_id;
#endif
};

DbcpppDecoder::DbcpppDecoder(const std::string& dbc_file, std::string* error)
  : impl_(std::make_shared<Impl>())
{
  if (dbc_file.empty())
  {
    SetError(error, "dbc path is empty");
    return;
  }

#if PJ_CAN_USE_DBCPPP
  std::ifstream dbc_stream(dbc_file);
  if (!dbc_stream.good())
  {
    SetError(error, "failed to open dbc file: " + dbc_file);
    return;
  }

  impl_->network = dbcppp::INetwork::LoadDBCFromIs(dbc_stream);
  if (!impl_->network)
  {
    SetError(error, "dbcppp failed to parse dbc file: " + dbc_file);
    return;
  }

  for (const auto& message : impl_->network->Messages())
  {
    const auto* message_ptr = ToPtr(message);
    if (message_ptr)
    {
      impl_->messages_by_id[message_ptr->Id()].push_back(message_ptr);
    }
  }

  valid_ = true;
  SetError(error, "");
#else
  (void)dbc_file;
  SetError(error, "dbcppp support is not available in this build");
#endif
}

bool DbcpppDecoder::IsValid() const
{
  return valid_;
}

std::vector<DecodedSignalValue> DbcpppDecoder::Decode(uint32_t can_id, bool extended, bool is_fd,
                                                      uint8_t dlc, const uint8_t* data,
                                                      std::size_t size) const
{
#if PJ_CAN_USE_DBCPPP
  if (!valid_ || !impl_)
  {
    return {};
  }

  const uint64_t base_id = NormalizeCanId(can_id, extended);
  const uint64_t lookup_id = extended ? (base_id | kExtendedCanIdFlag) : base_id;
  const auto id_it = impl_->messages_by_id.find(lookup_id);
  if (id_it == impl_->messages_by_id.end())
  {
    return {};
  }

  const std::size_t payload_size =
      std::min<std::size_t>(64U, std::max<std::size_t>(size, DlcToPayloadSize(dlc)));
  const dbcppp::IMessage* message = SelectMessageByPayload(id_it->second, payload_size, is_fd);
  if (!message)
  {
    return {};
  }

  std::array<uint8_t, 64> payload{};
  if (data && size > 0U)
  {
    const std::size_t copy_size = std::min<std::size_t>(size, payload.size());
    std::memcpy(payload.data(), data, copy_size);
  }

  const dbcppp::ISignal* mux_signal = message->MuxSignal();
  const int64_t mux_value = mux_signal ? mux_signal->Decode(payload.data()) : 0;

  std::vector<DecodedSignalValue> decoded_values;

  for (const auto& signal : message->Signals())
  {
    const auto* signal_ptr = ToPtr(signal);
    if (!signal_ptr)
    {
      continue;
    }

    const bool is_mux_value_signal =
        signal_ptr->MultiplexerIndicator() == dbcppp::ISignal::EMultiplexer::MuxValue;
    if (is_mux_value_signal && mux_signal &&
        mux_value != static_cast<int64_t>(signal_ptr->MultiplexerSwitchValue()))
    {
      continue;
    }

    const auto raw = signal_ptr->Decode(payload.data());
    const auto phys = signal_ptr->RawToPhys(raw);

    decoded_values.push_back(
        DecodedSignalValue{message->Name().c_str(), signal_ptr->Name().c_str(), phys});
  }

  return decoded_values;
#else
  (void)can_id;
  (void)extended;
  (void)is_fd;
  (void)dlc;
  (void)data;
  (void)size;
  return {};
#endif
}

}  // namespace PJ::BLF
