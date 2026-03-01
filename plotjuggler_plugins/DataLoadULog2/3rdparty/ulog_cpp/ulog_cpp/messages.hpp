/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "exception.hpp"
#include "raw_messages.hpp"
#include "utils.hpp"

namespace ulog_cpp {

// ULog format: https://docs.px4.io/main/en/dev_log/ulog_file_format.html

/**
 * @brief Definition for a function that writes data to a file
 * Each message has a serialize() method that takes a DataWriteCB as argument.
 * The serialize() method calls the DataWriteCB with the data to be written.
 */
using DataWriteCB = std::function<void(const uint8_t* data, int length)>;

/**
 * @brief ULog file header "message". The file header is always the first element in a ULG file.
 */
class FileHeader {
 public:
  FileHeader(const ulog_file_header_s& header, const ulog_message_flag_bits_s& flag_bits);
  explicit FileHeader(const ulog_file_header_s& header);

  explicit FileHeader(uint64_t timestamp = 0, bool has_default_parameters = false);

  const ulog_file_header_s& header() const { return _header; }
  const ulog_message_flag_bits_s& flagBits() const { return _flag_bits; }

  void serialize(const DataWriteCB& writer) const;

  bool operator==(const FileHeader& h) const
  {
    return memcmp(&_header, &h._header, sizeof(_header)) == 0 &&
           (!_has_flag_bits || memcmp(&_flag_bits, &h._flag_bits, sizeof(_flag_bits)) == 0);
  }

 private:
  ulog_file_header_s _header{};
  ulog_message_flag_bits_s _flag_bits{};
  bool _has_flag_bits{true};
};

class MessageFormat;  // forward declaration

/**
 * @brief Type definition for a Field. A field is a named data element in a message.
 * Fields occur in
 * - Message Formats
 * - Parameter message types (they have exactly one field)
 * - Info Messages (MessageInfo) (they have exactly one field)
 *
 * A field can be of a basic type (int, float, char, ...), an array of a basic type,
 * or reference a MessageFormat (nested message).
 *
 * In case a field is of nested type, the nested Message Format has to be resolved during parsing.
 * Since all Message Formats are in the header, at the end of the header, all nested fields should
 * be resolved.
 *
 * A field can be in "resolved" and "unresolved" state. In resolved state, the following things
 * are defined:
 * - Offset of the field in the Message Format
 * - Size of the field
 *
 * In a correct file, all fields can be resolved after the header is parsed.
 */
class Field {
 public:
  /**
   * Enum for the basic types of a field. Note that they can also appear as arrays, NESTED
   * references a child MessageFormat.
   */
  enum class BasicType {
    INT8,
    UINT8,
    INT16,
    UINT16,
    INT32,
    UINT32,
    INT64,
    UINT64,
    FLOAT,
    DOUBLE,
    CHAR,
    BOOL,
    NESTED,
  };

  /**
   * TypeAttribute type stores basic information about a type. In case of basic types,
   * the attribute field is just copied from the kBaiscTypes map. In case of nested types,
   * the attributes are dynamically computed in nested type resolution.
   */
  struct TypeAttributes {
    std::string name;                   ///< name of the type
    BasicType type{BasicType::NESTED};  ///< type as an enum
    int size{0};                        ///< size in bytes. Recursively computed for nested types
    std::shared_ptr<MessageFormat> nested_message{nullptr};  /// < nested message format

    TypeAttributes() = default;

    TypeAttributes(std::string name, BasicType type, int size)
        : name(std::move(name)), type(type), size(size), nested_message(nullptr)
    {
    }
  };

  /**
   * @brief Map of all basic types. Used to resolve basic types from strings.
   */
  static const std::map<std::string, TypeAttributes> kBasicTypes;

  Field() = default;

  /**
   * @brief Construct a field from input bytes. The string is expected to be in the format
   * '<type>[len] <name>' or '<type> <name>'.
   * This function is used when parsing data.
   * @param str The input string pointer
   * @param len The length of the input string
   */
  Field(const char* str, int len);

  /**
   * @brief Construct a field from a string. The string is expected to be in the format
   * @param type_str The type string
   * @param name_str The name string
   * @param array_length_int The array length. -1 if not an array
   */
  Field(const std::string& type_str, std::string name_str, int array_length_int = -1)
      : _array_length(array_length_int), _name(std::move(name_str))
  {
    auto it = kBasicTypes.find(type_str);
    if (it != kBasicTypes.end()) {
      _type = it->second;
    } else {
      // if not a basic type, set it to recursive
      _type = TypeAttributes(type_str, BasicType::NESTED, 0);
    }
  }

  std::string encode() const;

  /**
   * @brief Equality operator for fields. Two fields are equal if their type, array length and name
   * are equal.
   * @param field The field to compare to
   * @return True if the fields are equal
   */
  bool operator==(const Field& field) const
  {
    return _type.name == field._type.name && _array_length == field._array_length &&
           _name == field._name;
  }

  /**
   * @brief Get the type attributes of the field
   * @return The type attributes
   */
  const TypeAttributes& type() const { return _type; }

  /**
   * @brief Get the array length of the field
   * @return The array length, -1 if not an array
   */
  int arrayLength() const { return _array_length; }

  /**
   * @brief Get the offset of the field in the message. This is only valid if the field is resolved.
   * @return The offset in bytes, -1 if not resolved
   */
  int offsetInMessage() const { return _offset_in_message_bytes; }

  /**
   * @brief Get the name of the field
   * @return The name
   */
  const std::string& name() const { return _name; }

  /**
   * @brief Get the size of the field in bytes. This is only valid if the field is resolved.
   * @return The size in bytes, 0 if nested type is not resolved
   */
  inline int sizeBytes() const;

  /**
   * @brief Returns true if the field is resolved. A field is resolved if the offset in message
   * is defined and the type is not nested or the nested message is resolved.
   * @return True if the field is resolved
   */
  bool definitionResolved() const
  {
    return _offset_in_message_bytes >= 0 &&
           (_type.type != BasicType::NESTED || _type.nested_message != nullptr);
  }

  /**
   * @brief Attempt to resolve the definition of the field.
   * This would be called to resolve the definition of a field in a message format.
   * Fields would have to be resolved in-order, after each field being resolved, the offset
   * of the next field can be computed by reading the size of the last field.
   * @param existing_formats The map of existing message formats. Used to resolve nested types.
   * @param offset The offset of the field in the message
   */
  void resolveDefinition(
      const std::map<std::string, std::shared_ptr<MessageFormat>>& existing_formats, int offset);

  /**
   * Resolve this field's definition, knowing that it will for sure not be of nested type.
   * Resolution will fail if the type is nested.
   * @param offset the offset of the field in the message
   */
  void resolveDefinition(int offset);

  /**
   * For nested fields, this returns the format of the nested field.
   * For not nested fields, this throws.
   * @return The format of the nested field
   */
  std::shared_ptr<MessageFormat> nestedFormat() const;

  /**
   * For nested fields, this will return a field from the nested message format.
   * For not nested fields, this throws
   * @param name The name of the field of the nested message format
   * @return The field from the nested message format
   */
  std::shared_ptr<Field> nestedField(const std::string& name) const;

 private:
  TypeAttributes _type;   ///< type attributes
  int _array_length{-1};  ///< -1 means not-an-array
  int _offset_in_message_bytes{
      -1};            ///< default to begin of message. Gets filled on MessageFormat resolution
  std::string _name;  /// < name of the field
};

/**
 * @brief A value is a wrapper around a variant of all possible value types.
 * This class can extract values from the underlying data representation and cast the values
 * to the user defined types, or return them as a variant.
 *
 * The basic concept of this class is to always(!) extract the value in the format that it is
 * defined in the MessageFormat. For example, if the value is defined as a float, in the
 * MessageFormat, it will be interpreted as float.
 *
 * Since these types are only available at runtime, one can get the value as an std::variant,
 * and then do appropriate handling for each type. In case the user knows what type to expect,
 * it can also be explicitly casted to the desired type. In this case, any type conversion that is
 * allowed by `static_cast` will be performed.
 *
 * Array types can be directly extracted as std::vector<T>, string types as std::string.
 */
class Value {
 public:
  /**
   * NativeTypeVariant is a variant of all possible types that a value can be.
   * Note that it includes all array types as std::vectors, but not nested type.
   * Nested is handled differently.
   */
  using NativeTypeVariant =
      std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float,
                   double, bool, char, std::vector<int8_t>, std::vector<uint8_t>,
                   std::vector<int16_t>, std::vector<uint16_t>, std::vector<int32_t>,
                   std::vector<uint32_t>, std::vector<int64_t>, std::vector<uint64_t>,
                   std::vector<float>, std::vector<double>, std::vector<bool>, std::string>;

  /**
   * Construct a "Value" representation from underlying memory and Field reference
   * @param field_ref The field reference to use
   * @param backing_ref_begin Begin iterator of underlying memory
   * @param backing_ref_end End iterator of underlying memory
   * @param array_index Array within an array field to access directly
   */
  Value(const Field& field_ref, const std::vector<uint8_t>::const_iterator& backing_ref_begin,
        const std::vector<uint8_t>::const_iterator& backing_ref_end, int array_index = -1)
      : _field_ref(field_ref),
        _array_index(array_index),
        _backing_ref_begin(backing_ref_begin),
        _backing_ref_end(backing_ref_end)
  {
  }

  Value(const Field& field_ref, const std::vector<uint8_t>& backing_ref, int array_index = -1)
      : Value(field_ref, backing_ref.begin(), backing_ref.end(), array_index)
  {
  }

  /**
   * @brief Get the value as a variant of all possible types.
   * In case the underlying type is an array, it will be extracted as a std::vector<T>.
   * In case the underlying type is a char array, it will be extracted as a std::string.
   * In case the underlying type is an array, but we access a specific element, it will be
   * extracted as the basic type T.
   * @return std::variant of the value, in type defined in the Message Format
   */
  NativeTypeVariant asNativeTypeVariant() const;

  /**
   * @brief Cast the value to a desired, compile time known type. This will perform a static_cast
   * to the desired type, if the type conversion is allowed by static_cast.
   * For example one can extract a value defined as a uint8_t as an int.
   *
   * There are the following rules:
   * - If requested type is vector<T> and native type is also vector<T>, vector<T> is returned
   * - If requested type is vector<T>, but native type is vector<K>, each element gets casted from K
   * to T
   * - If requested type is not a vector, but native type is a vector, the first element is returned
   * - If requested type is a vector, but native type is not a vector, a vector with 1 element is
   * returned
   * - If requested type is T and native type is K, a static_cast is performed on the element
   * - If requested type is T and native type is T as well, the element is returned
   *
   * Specifically, when casting (signed) char, char is first casted to unsigned char, as this
   * is generally the more expected behaviour (e.g. when casting a char to an int, one would
   * expect the ASCII value of the char, not the signed value).
   * @tparam T
   * @return
   */
  template <typename T>
  T as() const
  {
    T res;
    std::visit(
        [&res](auto&& arg) {
          using NativeType = std::decay_t<decltype(arg)>;
          using ReturnType = T;
          if constexpr (is_string<NativeType>::value || is_string<ReturnType>::value) {
            // there is a string involved. Only allowed when both are string
            if constexpr (is_string<NativeType>::value && is_string<ReturnType>::value) {
              // both are string
              res = arg;
            } else {
              // one is string, the other is not
              throw AccessException("Assign strings and non-string types");
            }
          } else if constexpr (is_vector<NativeType>::value) {
            // this is natively a vector
            if constexpr (is_vector<ReturnType>::value) {
              // return type is also vector
              if constexpr (std::is_same_v<typename NativeType::value_type,
                                           typename ReturnType::value_type>) {
                // return type is same as native type
                res = arg;
              } else {
                // return type is different from native type, but a vector
                res.resize(arg.size());
                for (std::size_t i = 0; i < arg.size(); i++) {
                  res[i] = staticCastEnsureUnsignedChar<typename ReturnType::value_type>(arg[i]);
                }
              }
            } else {
              // return type is not a vector, just return first element
              if (arg.size() > 0) {
                res = staticCastEnsureUnsignedChar<ReturnType>(arg[0]);
              } else {
                throw AccessException("Cannot convert empty vector to non-vector type");
              }
            }
          } else {
            // this is natively not a vector
            if constexpr (is_vector<ReturnType>::value) {
              // return type is a vector
              res.resize(1);
              res[0] = staticCastEnsureUnsignedChar<typename ReturnType::value_type>(arg);
            } else {
              // return type is not a vector
              res = staticCastEnsureUnsignedChar<ReturnType>(arg);
            }
          }
        },
        asNativeTypeVariant());
    return res;
  }

  /**
   * Access a field in a nested message. This creates a new Value class,
   * with the field definition from the nested Message Format.
   * @param field The field in the nested format to access
   * @return The value of the field in the nested format
   */
  Value operator[](const Field& field) const;

  /**
   * Access a field in a nested message. This creates a new Value class,
   * with the field definition from the nested Message Format.
   * @param field The field in the nested format to access
   * @return The value of the field in the nested format
   */
  Value operator[](const std::shared_ptr<Field>& field) const;

  /**
   * Access a field in a nested message. This creates a new Value class,
   * with the field definition from the nested Message Format.
   * @param field_name the name of the field in the nested format to access
   * @return the value of the field in the nested format
   */
  Value operator[](const std::string& field_name) const;

  /**
   * Access a certain position in the array of this field. This creates a new Value class,
   * with the array index property set. This allows access of individual message elements
   * without converting the entire field into a vector first.
   * @param index the index of the element to access
   * @return a new Value object with the array index property set
   */
  Value operator[](size_t index) const;

 private:
  template <typename T>
  static T deserialize(const std::vector<uint8_t>::const_iterator& backing_start,
                       const std::vector<uint8_t>::const_iterator& backing_end, int offset,
                       int array_offset)
  {
    T v;
    const int total_offset = offset + (array_offset * sizeof(T));
    if (backing_start > backing_end ||
        backing_end - backing_start < static_cast<int64_t>(sizeof(v)) + total_offset) {
      throw AccessException("Unexpected data type size");
    }
    // GCC 14.2 raises an error here when building with -fsanitize=address
#if defined(__GNUC__) && (__GNUC__ >= 14)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    std::copy(backing_start + total_offset, backing_start + total_offset + sizeof(v),
              reinterpret_cast<uint8_t*>(&v));
#if defined(__GNUC__) && (__GNUC__ >= 14)
#pragma GCC diagnostic pop
#endif
    return v;
  }

  template <typename T>
  static std::vector<T> deserializeVector(const std::vector<uint8_t>::const_iterator& backing_start,
                                          const std::vector<uint8_t>::const_iterator& backing_end,
                                          int offset, int size)
  {
    std::vector<T> res;
    res.resize(size);
    for (int i = 0; i < size; i++) {
      res[i] = deserialize<T>(backing_start, backing_end, offset, i);
    }
    return res;
  }

  const Field& _field_ref;      /// < reference to the field definition
  const int _array_index = -1;  /// < index in the array, -1 if accessing head, or not an array
  const std::vector<uint8_t>::const_iterator
      _backing_ref_begin;  /// < begin iterator of the backing vector
  const std::vector<uint8_t>::const_iterator
      _backing_ref_end;  /// < end iterator of the backing vector
};

/**
 * Class that contains an Info Message. An info message can either be standalone, or be comprised
 * of multiple subsequent info messages.
 * An info message has a single field, which can theoretically be of any type. However, in practice
 * it is mostly a basic type.
 */
class MessageInfo {
 public:
  explicit MessageInfo(const uint8_t* msg, bool is_multi = false);

  MessageInfo(Field field, std::vector<uint8_t> value, bool is_multi = false,
              bool continued = false);

  MessageInfo(const std::string& key, const std::string& value);
  MessageInfo(const std::string& key, int32_t value);
  MessageInfo(const std::string& key, float value);

  const Field& field() const { return _field; }
  Field& field() { return _field; }
  std::vector<uint8_t>& valueRaw() { return _value; }
  const std::vector<uint8_t>& valueRaw() const { return _value; }
  Value value() const { return Value(_field, _value); }
  bool isContinued() const { return _continued; }
  bool isMulti() const { return _is_multi; }

  void serialize(const DataWriteCB& writer, ULogMessageType type = ULogMessageType::INFO) const;

  bool operator==(const MessageInfo& info) const
  {
    return _field == info._field && _value == info._value && _continued == info._continued &&
           _is_multi == info._is_multi;
  }

 private:
  void initValues(const char* values, int len);
  Field _field;
  std::vector<uint8_t> _value;
  bool _continued{false};
  bool _is_multi{false};
};

/**
 * Class that contains a Message Format. A message format contains all the fields contained in
 * Message Formats have to appear in the header of the log file, and are also parsed there.
 * a message. Message formats are referenced in
 * - Subscriptions: Each subscription is of a specific message format.
 * - Fields: Each field can be of type nested, and have a reference to a message format then.
 *
 * Message Formats can be recursive. A MessageFormat can contain a Field that is of nested type,
 * which refers to another MessgeFormat and so on. Therefore, when MessageFormats are parsed,
 * they are generally not "resolved" as for then nested Fields, MessageDefinitions may not be
 * available yet.
 * MessageFormats are resolved once the header is parsed.
 */
class MessageFormat {
 public:
  /**
   * Construct a messsage format from a raw message. This is used when parsing the header.
   * @param msg the raw message
   */
  explicit MessageFormat(const uint8_t* msg);

  /**
   * Manually construct a message format.
   * @param name name of the message format
   * @param fields list of fields, in-order.
   */
  explicit MessageFormat(std::string name, const std::vector<Field>& fields);

  /**
   * @return the name of the message format
   */
  const std::string& name() const { return _name; }

  /**
   * @return the map of fields, keyed by name
   */
  const std::map<std::string, std::shared_ptr<Field>>& fieldMap() const { return _fields; }

  /**
   * Serialize the message format into a raw message.
   * @param writer
   */
  void serialize(const DataWriteCB& writer) const;

  /**
   * Compare two message formats for equality.
   * Two message formats are equal if they have the same name, and the same fields in the same
   * order.
   * @param format the message format to compare to
   * @return true if the message formats are equal, false otherwise
   */
  bool operator==(const MessageFormat& format) const
  {
    if (_name != format._name) {
      return false;
    }
    if (_fields.size() != format._fields.size()) {
      return false;
    }
    for (size_t i = 0; i < _fields_ordered.size(); i++) {
      if (!(*_fields_ordered[i] == *format._fields_ordered[i])) {
        return false;
      }
    }
    return true;
  }

  /**
   * Recursively tries to resolve the MessageFormat with the help of a map of all known, existing
   * message formats. This is used to resolve nested fields, at the end of parsing the header.
   * @param existing_formats
   */
  void resolveDefinition(
      const std::map<std::string, std::shared_ptr<MessageFormat>>& existing_formats) const;

  /**
   * Returns the total size of this MessageFormat in bytes. This is only valid once the
   * MessageFormat has been resolved. All fields have to be resolved, and the size of the
   * MessageFormat is the sum of the sizes of all fields.
   * @return the size of the MessageFormat in bytes
   */
  int sizeBytes() const;

  /**
   * @return the list of fields, in-order
   */
  const std::vector<std::shared_ptr<Field>>& fields() const { return _fields_ordered; }

  /**
   * @return the list of field names, in-order
   */
  std::vector<std::string> fieldNames() const
  {
    std::vector<std::string> names;
    names.reserve(_fields_ordered.size());
    for (const auto& field_it : _fields_ordered) {
      names.push_back(field_it->name());
    }
    return names;
  }

  /**
   * Get a field by name.
   * @param name the name of the field
   * @return the requested field
   */
  std::shared_ptr<Field> field(const std::string& name) const
  {
    const auto it = _fields.find(name);
    if (it == _fields.end()) {
      throw AccessException("Field not found: " + name);
    }
    return it->second;
  }

 private:
  std::string _name;
  std::map<std::string, std::shared_ptr<Field>>
      _fields;  /// < map of fields, keyed by name, same fields as in the list
  std::vector<std::shared_ptr<Field>>
      _fields_ordered;  /// < list of fields, in-order, same fields as in the map
};

using Parameter = MessageInfo;

class ParameterDefault {
 public:
  explicit ParameterDefault(const uint8_t* msg);

  ParameterDefault(Field field, std::vector<uint8_t> value,
                   ulog_parameter_default_type_t default_types);

  const Field& field() const { return _field; }
  Field& field() { return _field; }
  const std::vector<uint8_t>& valueRaw() const { return _value; }
  Value value() const { return Value(_field, _value); }

  void serialize(const DataWriteCB& writer) const;

  ulog_parameter_default_type_t defaultType() const { return _default_types; }

 private:
  void initValues(const char* values, int len);
  Field _field;
  std::vector<uint8_t> _value;
  ulog_parameter_default_type_t _default_types{};
};

class AddLoggedMessage {
 public:
  explicit AddLoggedMessage(const uint8_t* msg);

  AddLoggedMessage(uint8_t multi_id, uint16_t msg_id, std::string message_name);

  const std::string& messageName() const { return _message_name; }
  uint8_t multiId() const { return _multi_id; }
  uint16_t msgId() const { return _msg_id; }

  void serialize(const DataWriteCB& writer) const;

 private:
  uint8_t _multi_id{};
  uint16_t _msg_id{};
  std::string _message_name;
};

class Logging {
 public:
  enum class Level : uint8_t {
    Emergency = '0',
    Alert = '1',
    Critical = '2',
    Error = '3',
    Warning = '4',
    Notice = '5',
    Info = '6',
    Debug = '7'
  };
  explicit Logging(const uint8_t* msg, bool is_tagged = false);

  Logging(Level level, std::string message, uint64_t timestamp);

  Level logLevel() const { return _log_level; }
  std::string logLevelStr() const;
  uint16_t tag() const { return _tag; }
  bool hasTag() const { return _has_tag; }
  uint64_t timestamp() const { return _timestamp; }
  const std::string& message() const { return _message; }

  void serialize(const DataWriteCB& writer) const;

  bool operator==(const Logging& logging) const
  {
    return _log_level == logging._log_level && _tag == logging._tag &&
           _has_tag == logging._has_tag && _timestamp == logging._timestamp &&
           _message == logging._message;
  }

 private:
  Level _log_level{};
  uint16_t _tag{};
  bool _has_tag{false};
  uint64_t _timestamp{};
  std::string _message;
};

/**
 * Data is an element that contains a single element from a subscription. In order
 * to decode it, it will need to get referenced to a MessageFormat. This is done using
 * the TypedDataView class in Subscription.
 */
class Data {
 public:
  explicit Data(const uint8_t* msg);

  Data(uint16_t msg_id, std::vector<uint8_t> data);

  uint16_t msgId() const { return _msg_id; }
  const std::vector<uint8_t>& data() const { return _data; }

  void serialize(const DataWriteCB& writer) const;

  bool operator==(const Data& data) const { return _msg_id == data._msg_id && _data == data._data; }

 private:
  uint16_t _msg_id{};
  std::vector<uint8_t> _data;
};

class Dropout {
 public:
  explicit Dropout(const uint8_t* msg);

  explicit Dropout(uint16_t duration_ms);

  uint16_t durationMs() const { return _duration_ms; }

  void serialize(const DataWriteCB& writer) const;

 private:
  uint16_t _duration_ms{};
};

class Sync {
 public:
  explicit Sync(const uint8_t* msg);

  explicit Sync() = default;

  void serialize(const DataWriteCB& writer) const;

 private:
  static constexpr uint8_t kSyncMagicBytes[] = {0x2F, 0x73, 0x13, 0x20, 0x25, 0x0C, 0xBB, 0x12};
};

}  // namespace ulog_cpp
