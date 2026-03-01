/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include "messages.hpp"

#include <cstring>
#include <limits>
#include <memory>
#include <utility>

#define CHECK_MSG_SIZE(size, min_required) \
  if ((size) < (min_required)) throw ParsingException("message too short")

namespace ulog_cpp {

FileHeader::FileHeader(const ulog_file_header_s& header, const ulog_message_flag_bits_s& flag_bits)
    : _header(header), _flag_bits(flag_bits)
{
}
FileHeader::FileHeader(const ulog_file_header_s& header) : _header(header), _has_flag_bits(false)
{
}

FileHeader::FileHeader(uint64_t timestamp, bool has_default_parameters)
{
  // Set flags
  if (has_default_parameters) {
    _flag_bits.compat_flags[0] |= ULOG_COMPAT_FLAG0_DEFAULT_PARAMETERS_MASK;
  }
  _flag_bits.msg_size = sizeof(_flag_bits) - ULOG_MSG_HEADER_LEN;

  memcpy(_header.magic, ulog_file_magic_bytes, sizeof(ulog_file_magic_bytes));
  _header.magic[7] = 1;  // file version 1
  _header.timestamp = timestamp;
}

void FileHeader::serialize(const DataWriteCB& writer) const
{
  writer(reinterpret_cast<const unsigned char*>(&_header), sizeof(_header));
  if (_has_flag_bits) {
    ulog_message_flag_bits_s flag_bits = _flag_bits;
    flag_bits.msg_size = sizeof(flag_bits) - ULOG_MSG_HEADER_LEN;
    writer(reinterpret_cast<const unsigned char*>(&flag_bits), sizeof(flag_bits));
  }
}

MessageInfo::MessageInfo(const uint8_t* msg, bool is_multi) : _is_multi(is_multi)
{
  if (is_multi) {
    const ulog_message_info_multiple_s* info_multi =
        reinterpret_cast<const ulog_message_info_multiple_s*>(msg);
    CHECK_MSG_SIZE(info_multi->msg_size, 3);
    _continued = info_multi->is_continued;
    if (info_multi->key_len > info_multi->msg_size - 2) {
      throw ParsingException("Key too long");  // invalid
    }
    _field = Field(info_multi->key_value_str, info_multi->key_len);
    initValues(info_multi->key_value_str + info_multi->key_len,
               info_multi->msg_size - info_multi->key_len - 2);
  } else {
    const ulog_message_info_s* info = reinterpret_cast<const ulog_message_info_s*>(msg);
    CHECK_MSG_SIZE(info->msg_size, 2);
    if (info->key_len > info->msg_size - 1) {
      throw ParsingException("Key too long");  // invalid
    }
    _field = Field(info->key_value_str, info->key_len);
    initValues(info->key_value_str + info->key_len, info->msg_size - info->key_len - 1);
  }
  if (_field.type().type != Field::BasicType::NESTED) {
    // For basic types we can resolve the definition immediately
    _field.resolveDefinition(0);
  }
}
void MessageInfo::initValues(const char* values, int len)
{
  _value.resize(len);
  memcpy(_value.data(), values, len);
}
Field::Field(const char* str, int len)
{
  // Format: '<type>[len] <name>' or '<type> <name>'
  // Find first space
  const std::string_view key_value{str, static_cast<std::string::size_type>(len)};
  const std::string::size_type first_space = key_value.find(' ');
  if (first_space == std::string::npos) {
    throw ParsingException("Invalid key format");
  }
  const std::string_view key_array = key_value.substr(0, first_space);
  _name = key_value.substr(first_space + 1);
  // Check for arrays
  const std::string::size_type bracket = key_array.find('[');
  std::string type_name;
  if (bracket == std::string::npos) {
    type_name = key_array;
  } else {
    type_name = key_array.substr(0, bracket);
    if (key_array[key_array.length() - 1] != ']') {
      throw ParsingException("Invalid key format (missing ])");
    }
    _array_length = std::stoi(std::string(key_array.substr(bracket + 1)));
  }
  auto it_basic = kBasicTypes.find(type_name);
  if (it_basic != kBasicTypes.end()) {
    _type = it_basic->second;
  } else {
    // Assume this is a recursive type (unresolved at this point)
    _type = {type_name, Field::BasicType::NESTED, 0};
  }
}

void Field::resolveDefinition(
    const std::map<std::string, std::shared_ptr<MessageFormat>>& existing_formats, int offset)
{
  // if this is already resolved, do nothing
  if (definitionResolved()) {
    return;
  }

  _offset_in_message_bytes = offset;
  // if this is a basic type, we are done
  if (_type.type != Field::BasicType::NESTED) {
    return;
  }

  auto it = existing_formats.find(_type.name);
  if (it == existing_formats.end()) {
    throw ParsingException("Message format not found: " + _type.name);
  }
  _type.nested_message = it->second;
  // recursively resolve nested type
  _type.nested_message->resolveDefinition(existing_formats);
  _type.size = _type.nested_message->sizeBytes();
}

void Field::resolveDefinition(int offset)
{
  if (definitionResolved()) {
    return;
  }

  if (_type.type == Field::BasicType::NESTED) {
    throw ParsingException("Nested type not resolved");
  }
  _offset_in_message_bytes = offset;
}

std::shared_ptr<MessageFormat> Field::nestedFormat() const
{
  if (_type.type != Field::BasicType::NESTED) {
    throw AccessException("Not a nested type");
  }
  return _type.nested_message;
}

std::shared_ptr<Field> Field::nestedField(const std::string& name) const
{
  if (_type.type != Field::BasicType::NESTED) {
    throw AccessException("Not a nested type");
  }
  return _type.nested_message->field(name);
}

const std::map<std::string, Field::TypeAttributes> Field::kBasicTypes{
    {"int8_t", {"int8_t", Field::BasicType::INT8, 1}},
    {"uint8_t", {"uint8_t", Field::BasicType::UINT8, 1}},
    {"int16_t", {"int16_t", Field::BasicType::INT16, 2}},
    {"uint16_t", {"uint16_t", Field::BasicType::UINT16, 2}},
    {"int32_t", {"int32_t", Field::BasicType::INT32, 4}},
    {"uint32_t", {"uint32_t", Field::BasicType::UINT32, 4}},
    {"int64_t", {"int64_t", Field::BasicType::INT64, 8}},
    {"uint64_t", {"uint64_t", Field::BasicType::UINT64, 8}},
    {"float", {"float", Field::BasicType::FLOAT, 4}},
    {"double", {"double", Field::BasicType::DOUBLE, 8}},
    {"bool", {"bool", Field::BasicType::BOOL, 1}},
    {"char", {"char", Field::BasicType::CHAR, 1}}};

int Field::sizeBytes() const
{
  if (!definitionResolved()) {
    throw ParsingException("Unresolved type " + _type.name);
  }
  return _type.size * ((_array_length == -1) ? 1 : _array_length);
}

std::string Field::encode() const
{
  if (_array_length >= 0) {
    return _type.name + '[' + std::to_string(_array_length) + ']' + ' ' + _name;
  }
  return _type.name + ' ' + _name;
}
Value::NativeTypeVariant Value::asNativeTypeVariant() const
{
  if (_array_index >= 0 && _field_ref.arrayLength() < 0) {
    throw AccessException("Can not access array element of non-array field");
  }

  if (_field_ref.arrayLength() == -1 || _array_index >= 0) {
    // decode as a single value. Either it is a single value,
    // or the user has explicitly selected an array element
    const int array_offset = _array_index >= 0 ? _array_index : 0;
    switch (_field_ref.type().type) {
      case Field::BasicType::INT8:
        return deserialize<int8_t>(_backing_ref_begin, _backing_ref_end,
                                   _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::UINT8:
        return deserialize<uint8_t>(_backing_ref_begin, _backing_ref_end,
                                    _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::INT16:
        return deserialize<int16_t>(_backing_ref_begin, _backing_ref_end,
                                    _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::UINT16:
        return deserialize<uint16_t>(_backing_ref_begin, _backing_ref_end,
                                     _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::INT32:
        return deserialize<int32_t>(_backing_ref_begin, _backing_ref_end,
                                    _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::UINT32:
        return deserialize<uint32_t>(_backing_ref_begin, _backing_ref_end,
                                     _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::INT64:
        return deserialize<int64_t>(_backing_ref_begin, _backing_ref_end,
                                    _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::UINT64:
        return deserialize<uint64_t>(_backing_ref_begin, _backing_ref_end,
                                     _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::FLOAT:
        return deserialize<float>(_backing_ref_begin, _backing_ref_end,
                                  _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::DOUBLE:
        return deserialize<double>(_backing_ref_begin, _backing_ref_end,
                                   _field_ref.offsetInMessage(), array_offset);
      case Field::BasicType::BOOL:
        return deserialize<bool>(_backing_ref_begin, _backing_ref_end, _field_ref.offsetInMessage(),
                                 array_offset);
      case Field::BasicType::CHAR:
        return deserialize<char>(_backing_ref_begin, _backing_ref_end, _field_ref.offsetInMessage(),
                                 array_offset);
      case Field::BasicType::NESTED:
        throw AccessException("Can't get nested field as basic type. Field " + _field_ref.name());
    }
  } else {
    // decode as an array
    switch (_field_ref.type().type) {
      case Field::BasicType::INT8:
        return deserializeVector<int8_t>(_backing_ref_begin, _backing_ref_end,
                                         _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::UINT8:
        return deserializeVector<uint8_t>(_backing_ref_begin, _backing_ref_end,
                                          _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::INT16:
        return deserializeVector<int16_t>(_backing_ref_begin, _backing_ref_end,
                                          _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::UINT16:
        return deserializeVector<uint16_t>(_backing_ref_begin, _backing_ref_end,
                                           _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::INT32:
        return deserializeVector<int32_t>(_backing_ref_begin, _backing_ref_end,
                                          _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::UINT32:
        return deserializeVector<uint32_t>(_backing_ref_begin, _backing_ref_end,
                                           _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::INT64:
        return deserializeVector<int64_t>(_backing_ref_begin, _backing_ref_end,
                                          _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::UINT64:
        return deserializeVector<uint64_t>(_backing_ref_begin, _backing_ref_end,
                                           _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::FLOAT:
        return deserializeVector<float>(_backing_ref_begin, _backing_ref_end,
                                        _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::DOUBLE:
        return deserializeVector<double>(_backing_ref_begin, _backing_ref_end,
                                         _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::BOOL:
        return deserializeVector<bool>(_backing_ref_begin, _backing_ref_end,
                                       _field_ref.offsetInMessage(), _field_ref.arrayLength());
      case Field::BasicType::CHAR: {
        auto string_start_iterator = _backing_ref_begin + _field_ref.offsetInMessage();
        if (_backing_ref_end - string_start_iterator < _field_ref.arrayLength()) {
          throw AccessException("Decoding fault, memory too short");
        }
        const int string_length = strnlen(&(*string_start_iterator), _field_ref.arrayLength());
        return std::string(string_start_iterator, string_start_iterator + string_length);
      }

      case Field::BasicType::NESTED:
        throw AccessException("Can't get nested field as basic type. Field " + _field_ref.name());
    }
  }
  return deserialize<uint8_t>(_backing_ref_begin, _backing_ref_end, _field_ref.offsetInMessage(),
                              0);
}

Value Value::operator[](const Field& field) const
{
  if (_field_ref.type().type != Field::BasicType::NESTED) {
    throw AccessException("Cannot access field of non-nested type");
  }
  if (!_field_ref.definitionResolved()) {
    throw AccessException("Cannot access field of unresolved type");
  }
  const int submessage_offset = _field_ref.offsetInMessage() +
                                ((_array_index >= 0) ? _field_ref.type().size * _array_index : 0);

  return Value(field, _backing_ref_begin + submessage_offset, _backing_ref_end);
}

Value Value::operator[](const std::shared_ptr<Field>& field) const
{
  return operator[](*field);
}

Value Value::operator[](const std::string& field_name) const
{
  if (_field_ref.type().type != Field::BasicType::NESTED) {
    throw AccessException("Cannot access field of non-nested type");
  }
  if (!_field_ref.definitionResolved()) {
    throw AccessException("Cannot access field of unresolved type");
  }
  const auto& field = _field_ref.type().nested_message->field(field_name);
  return operator[](*field);
}

Value Value::operator[](size_t index) const
{
  if (_field_ref.arrayLength() < 0) {
    throw AccessException("Cannot access field of non-array type");
  }
  if (index >= static_cast<size_t>(_field_ref.arrayLength())) {
    throw AccessException("Index out of bounds");
  }
  return Value(_field_ref, _backing_ref_begin, _backing_ref_end, index);
}

MessageInfo::MessageInfo(Field field, std::vector<uint8_t> value, bool is_multi, bool continued)
    : _field(std::move(field)), _value(std::move(value)), _continued(continued), _is_multi(is_multi)
{
}
MessageInfo::MessageInfo(const std::string& key, int32_t value) : _field({"int32_t", key})
{
  _value.resize(sizeof(value));
  memcpy(_value.data(), &value, sizeof(value));
}
MessageInfo::MessageInfo(const std::string& key, float value) : _field({"float", key})
{
  _value.resize(sizeof(value));
  memcpy(_value.data(), &value, sizeof(value));
}
MessageInfo::MessageInfo(const std::string& key, const std::string& value)
    : _field({"char", key, static_cast<int>(value.length())})
{
  _value.resize(value.length());
  memcpy(_value.data(), value.data(), value.length());
}
void MessageInfo::serialize(const DataWriteCB& writer, ULogMessageType type) const
{
  const std::string field_encoded = _field.encode();
  if (_is_multi) {
    ulog_message_info_multiple_s info_multi{};
    info_multi.is_continued = _continued;
    const int msg_size = field_encoded.length() + _value.size() + 2;
    if (msg_size > std::numeric_limits<uint16_t>::max() ||
        field_encoded.length() > std::numeric_limits<uint8_t>::max()) {
      throw ParsingException("message too long");
    }
    info_multi.key_len = field_encoded.length();
    info_multi.msg_size = msg_size;

    writer(reinterpret_cast<const unsigned char*>(&info_multi), ULOG_MSG_HEADER_LEN + 2);
    writer(reinterpret_cast<const unsigned char*>(field_encoded.data()), field_encoded.size());
    writer(reinterpret_cast<const unsigned char*>(_value.data()), _value.size());
  } else {
    ulog_message_info_s info{};
    const int msg_size = field_encoded.length() + _value.size() + 1;
    if (msg_size > std::numeric_limits<uint16_t>::max() ||
        field_encoded.length() > std::numeric_limits<uint8_t>::max()) {
      throw ParsingException("message too long");
    }
    info.key_len = field_encoded.length();
    info.msg_size = msg_size;
    info.msg_type = static_cast<uint8_t>(type);

    writer(reinterpret_cast<const unsigned char*>(&info), ULOG_MSG_HEADER_LEN + 1);
    writer(reinterpret_cast<const unsigned char*>(field_encoded.data()), field_encoded.size());
    writer(reinterpret_cast<const unsigned char*>(_value.data()), _value.size());
  }
}
MessageFormat::MessageFormat(const uint8_t* msg)
{
  const ulog_message_format_s* format = reinterpret_cast<const ulog_message_format_s*>(msg);
  // Format: <name>:<field0>;<field1>; ...
  auto format_str = std::string_view(format->format, format->msg_size);
  const std::string::size_type colon = format_str.find(':');
  if (colon == std::string::npos) {
    throw ParsingException("Invalid message format (no :)");  // invalid
  }
  _name = format_str.substr(0, colon);
  format_str = format_str.substr(colon + 1);
  while (!format_str.empty()) {
    const std::string::size_type semicolon = format_str.find(';');
    if (semicolon == std::string::npos) {
      throw ParsingException("Invalid message format (no ;)");  // invalid
    }
    auto f = std::make_shared<Field>(format_str.data(), static_cast<int>(semicolon));
    _fields.insert({f->name(), f});
    _fields_ordered.push_back(f);
    format_str = format_str.substr(semicolon + 1);
  }
}
MessageFormat::MessageFormat(std::string name, const std::vector<Field>& fields)
    : _name(std::move(name))
{
  for (const auto& current_field : fields) {
    auto f = std::make_shared<Field>(current_field);
    _fields.insert({f->name(), f});
    _fields_ordered.push_back(f);
  }
}

int MessageFormat::sizeBytes() const
{
  int size = 0;
  for (const auto& it : _fields) {
    size += it.second->sizeBytes();
  }
  return size;
}

void MessageFormat::resolveDefinition(
    const std::map<std::string, std::shared_ptr<MessageFormat>>& existing_formats) const
{
  int offset = 0;
  for (const auto& current_field : _fields_ordered) {
    if (!current_field->definitionResolved()) {
      current_field->resolveDefinition(existing_formats, offset);
    }
    offset += current_field->sizeBytes();
  }
}

void MessageFormat::serialize(const DataWriteCB& writer) const
{
  std::string format_str = _name + ':';

  for (const auto& current_field : _fields_ordered) {
    format_str += current_field->encode() + ';';
  }

  ulog_message_format_s format;
  const int msg_size = format_str.length();
  if (msg_size > std::numeric_limits<uint16_t>::max()) {
    throw ParsingException("message too long");
  }
  format.msg_size = msg_size;
  writer(reinterpret_cast<const unsigned char*>(&format), ULOG_MSG_HEADER_LEN);
  writer(reinterpret_cast<const unsigned char*>(format_str.data()), format_str.size());
}

ParameterDefault::ParameterDefault(const uint8_t* msg)
{
  const ulog_message_parameter_default_s* param_default =
      reinterpret_cast<const ulog_message_parameter_default_s*>(msg);
  CHECK_MSG_SIZE(param_default->msg_size, 3);
  _default_types = param_default->default_types;
  if (param_default->key_len > param_default->msg_size - 2) {
    throw ParsingException("Key too long");  // invalid
  }
  _field = Field(param_default->key_value_str, param_default->key_len);
  initValues(param_default->key_value_str + param_default->key_len,
             param_default->msg_size - param_default->key_len - 2);
  if (_field.type().type != Field::BasicType::NESTED) {
    // For basic types we can resolve the definition immediately
    _field.resolveDefinition(0);
  }
}
void ParameterDefault::initValues(const char* values, int len)
{
  _value.resize(len);
  memcpy(_value.data(), values, len);
}
ParameterDefault::ParameterDefault(Field field, std::vector<uint8_t> value,
                                   ulog_parameter_default_type_t default_types)
    : _field(std::move(field)), _value(std::move(value)), _default_types(default_types)
{
}
void ParameterDefault::serialize(const DataWriteCB& writer) const
{
  const std::string field_encoded = _field.encode();
  ulog_message_parameter_default_s parameter_default{};
  const int msg_size = field_encoded.length() + _value.size() + 2;
  if (msg_size > std::numeric_limits<uint16_t>::max() ||
      field_encoded.length() > std::numeric_limits<uint8_t>::max()) {
    throw ParsingException("message too long");
  }
  parameter_default.key_len = field_encoded.length();
  parameter_default.msg_size = msg_size;
  parameter_default.default_types = _default_types;

  writer(reinterpret_cast<const unsigned char*>(&parameter_default), ULOG_MSG_HEADER_LEN + 2);
  writer(reinterpret_cast<const unsigned char*>(field_encoded.data()), field_encoded.size());
  writer(reinterpret_cast<const unsigned char*>(_value.data()), _value.size());
}

AddLoggedMessage::AddLoggedMessage(const uint8_t* msg)
{
  const ulog_message_add_logged_s* add_logged =
      reinterpret_cast<const ulog_message_add_logged_s*>(msg);
  CHECK_MSG_SIZE(add_logged->msg_size, 4);
  _multi_id = add_logged->multi_id;
  _msg_id = add_logged->msg_id;
  _message_name = std::string(add_logged->message_name, add_logged->msg_size - 3);
}
AddLoggedMessage::AddLoggedMessage(uint8_t multi_id, uint16_t msg_id, std::string message_name)
    : _multi_id(multi_id), _msg_id(msg_id), _message_name(std::move(message_name))
{
}
void AddLoggedMessage::serialize(const DataWriteCB& writer) const
{
  ulog_message_add_logged_s add_logged;
  const int msg_size = _message_name.size() + 3;
  if (msg_size > std::numeric_limits<uint16_t>::max()) {
    throw ParsingException("message too long");
  }
  add_logged.multi_id = _multi_id;
  add_logged.msg_id = _msg_id;
  add_logged.msg_size = msg_size;

  writer(reinterpret_cast<const unsigned char*>(&add_logged), ULOG_MSG_HEADER_LEN + 3);
  writer(reinterpret_cast<const unsigned char*>(_message_name.data()), _message_name.size());
}

Logging::Logging(const uint8_t* msg, bool is_tagged) : _has_tag(is_tagged)
{
  uint8_t log_level{};
  if (is_tagged) {
    const ulog_message_logging_tagged_s* logging =
        reinterpret_cast<const ulog_message_logging_tagged_s*>(msg);
    CHECK_MSG_SIZE(logging->msg_size, 12);
    log_level = logging->log_level;
    _tag = logging->tag;
    _timestamp = logging->timestamp;
    _message = std::string(logging->message, logging->msg_size - 11);
  } else {
    const ulog_message_logging_s* logging = reinterpret_cast<const ulog_message_logging_s*>(msg);
    CHECK_MSG_SIZE(logging->msg_size, 10);
    log_level = logging->log_level;
    _timestamp = logging->timestamp;
    _message = std::string(logging->message, logging->msg_size - 9);
  }
  if (log_level < static_cast<uint8_t>(Level::Emergency) ||
      log_level > static_cast<uint8_t>(Level::Debug)) {
    _log_level = Level::Debug;
  } else {
    _log_level = static_cast<Level>(log_level);
  }
}
Logging::Logging(Level level, std::string message, uint64_t timestamp)
    : _log_level(level), _timestamp(timestamp), _message(std::move(message))
{
}
std::string Logging::logLevelStr() const
{
  switch (_log_level) {
    case Level::Emergency:
      return "Emergency";
    case Level::Alert:
      return "Alert";
    case Level::Critical:
      return "Critical";
    case Level::Error:
      return "Error";
    case Level::Warning:
      return "Warning";
    case Level::Notice:
      return "Notice";
    case Level::Info:
      return "Info";
    case Level::Debug:
      return "Debug";
  }
  return "unknown";
}
void Logging::serialize(const DataWriteCB& writer) const
{
  if (_has_tag) {
    ulog_message_logging_tagged_s logging;
    const int msg_size = _message.size() + 11;
    if (msg_size > std::numeric_limits<uint16_t>::max()) {
      throw ParsingException("message too long");
    }
    logging.log_level = static_cast<uint8_t>(_log_level);
    logging.tag = _tag;
    logging.timestamp = _timestamp;
    logging.msg_size = msg_size;

    writer(reinterpret_cast<const unsigned char*>(&logging), ULOG_MSG_HEADER_LEN + 11);
    writer(reinterpret_cast<const unsigned char*>(_message.data()), _message.size());
  } else {
    ulog_message_logging_s logging;
    const int msg_size = _message.size() + 9;
    if (msg_size > std::numeric_limits<uint16_t>::max()) {
      throw ParsingException("message too long");
    }
    logging.log_level = static_cast<uint8_t>(_log_level);
    logging.timestamp = _timestamp;
    logging.msg_size = msg_size;

    writer(reinterpret_cast<const unsigned char*>(&logging), ULOG_MSG_HEADER_LEN + 9);
    writer(reinterpret_cast<const unsigned char*>(_message.data()), _message.size());
  }
}

Data::Data(const uint8_t* msg)
{
  const ulog_message_data_s* msg_data = reinterpret_cast<const ulog_message_data_s*>(msg);
  CHECK_MSG_SIZE(msg_data->msg_size, 3);
  _msg_id = msg_data->msg_id;
  const int data_len = msg_data->msg_size - 2;
  _data.resize(data_len);
  memcpy(_data.data(), &msg_data->msg_id + 1, data_len);
}
Data::Data(uint16_t msg_id, std::vector<uint8_t> data) : _msg_id(msg_id), _data(std::move(data))
{
}
void Data::serialize(const DataWriteCB& writer) const
{
  ulog_message_data_s data_msg;
  const int msg_size = _data.size() + 2;
  if (msg_size > std::numeric_limits<uint16_t>::max()) {
    throw ParsingException("message too long");
  }
  data_msg.msg_id = _msg_id;
  data_msg.msg_size = msg_size;

  writer(reinterpret_cast<const unsigned char*>(&data_msg), ULOG_MSG_HEADER_LEN + 2);
  writer(reinterpret_cast<const unsigned char*>(_data.data()), _data.size());
}

Dropout::Dropout(const uint8_t* msg)
{
  const ulog_message_dropout_s* dropout = reinterpret_cast<const ulog_message_dropout_s*>(msg);
  CHECK_MSG_SIZE(dropout->msg_size, 2);
  _duration_ms = dropout->duration;
}
Dropout::Dropout(uint16_t duration_ms) : _duration_ms(duration_ms)
{
}
void Dropout::serialize(const DataWriteCB& writer) const
{
  ulog_message_dropout_s dropout;
  const int msg_size = sizeof(dropout) - ULOG_MSG_HEADER_LEN;
  dropout.duration = _duration_ms;
  dropout.msg_size = msg_size;

  writer(reinterpret_cast<const unsigned char*>(&dropout), sizeof(dropout));
}
Sync::Sync(const uint8_t* msg)
{
  const ulog_message_sync_s* sync = reinterpret_cast<const ulog_message_sync_s*>(msg);
  CHECK_MSG_SIZE(sync->msg_size, sizeof(kSyncMagicBytes));
  if (memcmp(sync->sync_magic, kSyncMagicBytes, sizeof(kSyncMagicBytes)) != 0) {
    throw ParsingException("Invalid sync magic bytes");
  }
}
void Sync::serialize(const DataWriteCB& writer) const
{
  ulog_message_sync_s sync;
  const int msg_size = sizeof(sync) - ULOG_MSG_HEADER_LEN;
  static_assert(sizeof(sync.sync_magic) == sizeof(kSyncMagicBytes));
  memcpy(sync.sync_magic, kSyncMagicBytes, sizeof(kSyncMagicBytes));
  sync.msg_size = msg_size;

  writer(reinterpret_cast<const unsigned char*>(&sync), sizeof(sync));
}
}  // namespace ulog_cpp
