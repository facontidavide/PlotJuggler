#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace PJ::BLF
{

struct ChannelDbcBinding
{
  uint32_t channel = 0;
  std::string dbc_file;
};

struct DecodedSignalValue
{
  std::string message;
  std::string signal;
  double value = 0.0;
};

class IDbcDecoder
{
public:
  virtual ~IDbcDecoder() = default;

  virtual std::vector<DecodedSignalValue> Decode(uint32_t can_id, bool extended, bool is_fd,
                                                 uint8_t dlc, const uint8_t* data,
                                                 std::size_t size) const = 0;
};

class DbcManager
{
public:
  using DecoderFactory = std::function<std::unique_ptr<IDbcDecoder>(const std::string&)>;

  explicit DbcManager(DecoderFactory decoder_factory = DecoderFactory());

  bool LoadBindings(const std::vector<ChannelDbcBinding>& bindings, std::string* error = nullptr);

  std::vector<DecodedSignalValue> DecodeForChannel(uint32_t channel, uint32_t can_id,
                                                   bool extended, bool is_fd, uint8_t dlc,
                                                   const uint8_t* data, std::size_t size) const;

  bool HasBinding(uint32_t channel) const;

private:
  struct ChannelBindingEntry
  {
    std::string dbc_file;
    std::unique_ptr<IDbcDecoder> decoder;
  };

  DecoderFactory decoder_factory_;
  std::unordered_map<uint32_t, ChannelBindingEntry> bindings_;
};

}  // namespace PJ::BLF
