#include "rosx_introspection/serializer.hpp"

#include "rosx_introspection/contrib/nanocdr.hpp"
namespace RosMsgParser {

NanoCDR_Serializer::NanoCDR_Serializer() {
  _storage.reserve(1024);
  _cdr_encoder = std::make_shared<nanocdr::Encoder>(nanocdr::CdrHeader(), _storage);
  // Write the CDR header to the first 4 bytes
}

void NanoCDR_Serializer::serialize(BuiltinType type, const Variant& val) {
  switch (type) {
    case BuiltinType::CHAR:
    case BuiltinType::UINT8:
      _cdr_encoder->encode(val.convert<uint8_t>());
      break;

    case BuiltinType::BOOL:
    case BuiltinType::BYTE:
    case BuiltinType::INT8:
      _cdr_encoder->encode(val.convert<int8_t>());
      break;

    case BuiltinType::UINT16:
      _cdr_encoder->encode(val.convert<uint16_t>());
      break;
    case BuiltinType::INT16:
      _cdr_encoder->encode(val.convert<int16_t>());
      break;
    case BuiltinType::UINT32:
      _cdr_encoder->encode(val.convert<uint32_t>());
      break;
    case BuiltinType::INT32:
      _cdr_encoder->encode(val.convert<int32_t>());
      break;
    case BuiltinType::UINT64:
      _cdr_encoder->encode(val.convert<uint64_t>());
      break;
    case BuiltinType::INT64:
      _cdr_encoder->encode(val.convert<int64_t>());
      break;
    case BuiltinType::FLOAT32:
      _cdr_encoder->encode(val.convert<float>());
      break;
    case BuiltinType::FLOAT64:
      _cdr_encoder->encode(val.convert<double>());
      break;
    default:
      throw std::runtime_error("Unsupported type");
  }
}

void NanoCDR_Serializer::serializeString(const std::string& str) {
  _cdr_encoder->encode(str);
}

void NanoCDR_Serializer::serializeUInt32(uint32_t value) {
  _cdr_encoder->encode(value);
}

void NanoCDR_Serializer::reset() {
  _cdr_encoder = std::make_shared<nanocdr::Encoder>(nanocdr::CdrHeader(), _storage);
}

const char* NanoCDR_Serializer::getBufferData() const {
  return reinterpret_cast<const char*>(_storage.data());
}

size_t NanoCDR_Serializer::getBufferSize() const {
  return _storage.size();
}

}  // namespace RosMsgParser
