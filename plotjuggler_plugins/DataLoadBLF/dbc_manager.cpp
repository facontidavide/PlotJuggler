#include "dbc_manager.h"

#include <sstream>
#include <utility>

namespace PJ::BLF
{
namespace
{

class NullDbcDecoder : public IDbcDecoder
{
public:
  std::vector<DecodedSignalValue> Decode(uint32_t can_id, bool extended, bool is_fd, uint8_t dlc,
                                         const uint8_t* data, std::size_t size) const override
  {
    (void)can_id;
    (void)extended;
    (void)is_fd;
    (void)dlc;
    (void)data;
    (void)size;
    return {};
  }
};

void SetError(std::string* error, const std::string& message)
{
  if (error)
  {
    *error = message;
  }
}

}  // namespace

DbcManager::DbcManager(DecoderFactory decoder_factory)
  : decoder_factory_(std::move(decoder_factory))
{
  if (!decoder_factory_)
  {
    decoder_factory_ = [](const std::string&) { return std::make_unique<NullDbcDecoder>(); };
  }
}

bool DbcManager::LoadBindings(const std::vector<ChannelDbcBinding>& bindings, std::string* error)
{
  std::unordered_map<uint32_t, ChannelBindingEntry> next_bindings;
  next_bindings.reserve(bindings.size());

  for (const auto& binding : bindings)
  {
    if (binding.dbc_file.empty())
    {
      std::ostringstream oss;
      oss << "empty dbc file path for channel " << binding.channel;
      SetError(error, oss.str());
      return false;
    }

    if (next_bindings.find(binding.channel) != next_bindings.end())
    {
      std::ostringstream oss;
      oss << "duplicate channel binding: " << binding.channel;
      SetError(error, oss.str());
      return false;
    }

    auto decoder = decoder_factory_(binding.dbc_file);
    if (!decoder)
    {
      std::ostringstream oss;
      oss << "failed to create decoder for channel " << binding.channel;
      SetError(error, oss.str());
      return false;
    }

    next_bindings.emplace(binding.channel,
                          ChannelBindingEntry{binding.dbc_file, std::move(decoder)});
  }

  bindings_ = std::move(next_bindings);
  return true;
}

std::vector<DecodedSignalValue> DbcManager::DecodeForChannel(uint32_t channel, uint32_t can_id,
                                                             bool extended, bool is_fd, uint8_t dlc,
                                                             const uint8_t* data,
                                                             std::size_t size) const
{
  const auto it = bindings_.find(channel);
  if (it == bindings_.end() || !it->second.decoder)
  {
    return {};
  }

  return it->second.decoder->Decode(can_id, extended, is_fd, dlc, data, size);
}

bool DbcManager::HasBinding(uint32_t channel) const
{
  return bindings_.find(channel) != bindings_.end();
}

}  // namespace PJ::BLF
