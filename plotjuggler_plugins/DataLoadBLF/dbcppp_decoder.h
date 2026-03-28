#pragma once

#include <memory>
#include <string>
#include <vector>

#include "dbc_manager.h"

namespace PJ::BLF
{

class DbcpppDecoder : public IDbcDecoder
{
public:
  explicit DbcpppDecoder(const std::string& dbc_file, std::string* error = nullptr);

  bool IsValid() const;

  std::vector<DecodedSignalValue> Decode(uint32_t can_id, bool extended, bool is_fd, uint8_t dlc,
                                         const uint8_t* data, std::size_t size) const override;

private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
  bool valid_ = false;
};

}  // namespace PJ::BLF
