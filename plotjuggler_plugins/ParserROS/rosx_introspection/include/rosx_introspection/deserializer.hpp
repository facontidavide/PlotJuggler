#ifndef DESERIALIZER_HPP
#define DESERIALIZER_HPP

// API adapted to FastCDR

#include <exception>

#include "rosx_introspection/builtin_types.hpp"
#include "rosx_introspection/variant.hpp"

namespace nanocdr
{
class Decoder;
}  // namespace nanocdr

namespace RosMsgParser
{

class Deserializer
{
public:
  virtual void init(Span<const uint8_t> buffer)
  {
    _buffer = buffer;
    reset();
  }

  virtual bool isROS2() const = 0;

  virtual ~Deserializer() = default;

  // move the memory pointer
  virtual void jump(size_t bytes) = 0;

  // deserialize the current pointer into a variant (not a string)
  [[nodiscard]] virtual Variant deserialize(BuiltinType type) = 0;

  [[nodiscard]] virtual Span<const uint8_t> deserializeByteSequence() = 0;

  // deserialize the current pointer into a string
  virtual void deserializeString(std::string& out) = 0;

  [[nodiscard]] virtual uint32_t deserializeUInt32() = 0;

  [[nodiscard]] virtual const uint8_t* getCurrentPtr() const = 0;

  [[nodiscard]] virtual size_t bytesLeft() const
  {
    return _buffer.size() - (getCurrentPtr() - _buffer.data());
  }

  // reset the pointer to beginning of buffer
  virtual void reset() = 0;

protected:
  Span<const uint8_t> _buffer;
};

//-----------------------------------------------------------------

// Specialization od deserializer that works with ROS1
class ROS_Deserializer : public Deserializer
{
public:
  Variant deserialize(BuiltinType type) override;

  bool isROS2() const override
  {
    return false;
  }

  void deserializeString(std::string& dst) override;

  uint32_t deserializeUInt32() override;

  Span<const uint8_t> deserializeByteSequence() override;

  const uint8_t* getCurrentPtr() const override;

  void jump(size_t bytes) override;

  void reset() override;

protected:
  const uint8_t* _ptr;
  size_t _bytes_left;

  template <typename T>
  T deserialize()
  {
    T out;
    if (sizeof(T) > _bytes_left)
    {
      throw std::runtime_error("Buffer overrun in Deserializer");
    }
    out = (*(reinterpret_cast<const T*>(_ptr)));
    _bytes_left -= sizeof(T);
    _ptr += sizeof(T);
    return out;
  }
};

//-----------------------------------------------------------------

// Specialization od deserializer that works with ROS2
// wrapping FastCDR
class NanoCDR_Deserializer : public Deserializer
{
public:
  Variant deserialize(BuiltinType type) override;

  void deserializeString(std::string& dst) override;

  uint32_t deserializeUInt32() override;

  Span<const uint8_t> deserializeByteSequence() override;

  const uint8_t* getCurrentPtr() const override;

  void jump(size_t bytes) override;

  virtual void reset() override;

  bool isROS2() const override
  {
    return true;
  }

protected:
  std::shared_ptr<nanocdr::Decoder> _cdr_decoder;
};

using ROS2_Deserializer = NanoCDR_Deserializer;

}  // namespace RosMsgParser

#endif  // DESERIALIZER_HPP
