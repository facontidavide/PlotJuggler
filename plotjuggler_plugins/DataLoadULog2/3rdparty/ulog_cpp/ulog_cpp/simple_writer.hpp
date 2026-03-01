/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <cstdio>
#include <memory>
// GCC 14.2 raises an error in <regex> when building with -fsanitize=address
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include <regex>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include <unordered_map>
#include <vector>

#include "writer.hpp"

namespace ulog_cpp {

/**
 * ULog serialization class which checks for integrity and correct calling order.
 * It throws an UsageException() in case of a failed integrity check.
 */
class SimpleWriter {
 public:
  /**
   * Constructor with a callback for writing data.
   * @param data_write_cb callback for serialized ULog data
   * @param timestamp_us start timestamp [us]
   */
  explicit SimpleWriter(DataWriteCB data_write_cb, uint64_t timestamp_us);
  /**
   * Constructor to write to a file.
   * @param filename ULog file to write to (will be overwritten if it exists)
   * @param timestamp_us  start timestamp [us]
   */
  explicit SimpleWriter(const std::string& filename, uint64_t timestamp_us);

  ~SimpleWriter();

  /**
   * Write a key-value info. Typically used for versioning information and written to the header.
   * @tparam T one of std::string, int32_t, float
   * @param key (unique) name, e.g. sys_name
   * @param value
   */
  template <typename T>
  void writeInfo(const std::string& key, const T& value)
  {
    _writer->messageInfo(ulog_cpp::MessageInfo(key, value));
  }

  /**
   * Write a parameter name-value pair to the header
   * @tparam T one of int32_t, float
   * @param key (unique) name, e.g. PARAM_A
   * @param value
   */
  template <typename T>
  void writeParameter(const std::string& key, const T& value)
  {
    if (_header_complete) {
      throw UsageException("Header already complete");
    }
    _writer->parameter(ulog_cpp::Parameter(key, value));
  }

  /**
   * Write a message format definition to the header.
   *
   * Supported field types:
   * "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t", "uint64_t",
   * "float", "double", "bool", "char"
   *
   * The first field must be: {"uint64_t", "timestamp"}.
   *
   * Note that ULog also supports nested format definitions, which is not supported here.
   *
   * When aligning the fields according to a multiple of their size, there must be no padding
   * between fields. The simplest way to achieve this is to order fields by decreasing size of
   * their type. If incorrect, a UsageException() is thrown.
   *
   * @param name format name, must match the regex: "[a-zA-Z0-9_\\-/]+"
   * @param fields message fields, names must match the regex: "[a-z0-9_]+"
   */
  void writeMessageFormat(const std::string& name, const std::vector<Field>& fields);

  /**
   * Call this to complete the header (after calling the above methods).
   */
  void headerComplete();

  /**
   * Write a parameter change (@see writeParameter())
   */
  template <typename T>
  void writeParameterChange(const std::string& key, const T& value)
  {
    if (!_header_complete) {
      throw UsageException("Header not yet complete");
    }
    _writer->parameter(ulog_cpp::Parameter(key, value));
  }

  /**
   * Create a time-series instance based on a message format definition.
   * @param message_format_name Format name from writeMessageFormat()
   * @param multi_id Instance id, if there's multiple
   * @return message id, used for writeData() later on
   */
  uint16_t writeAddLoggedMessage(const std::string& message_format_name, uint8_t multi_id = 0);

  /**
   * Write a text message
   */
  void writeTextMessage(Logging::Level level, const std::string& message, uint64_t timestamp);

  /**
   * Write some data. The timestamp must be monotonically increasing for a given time-series (i.e.
   * same id).
   * @param id ID from writeAddLoggedMessage()
   * @param data data according to the message format definition
   */
  template <typename T>
  void writeData(uint16_t id, const T& data)
  {
    writeDataImpl(id, reinterpret_cast<const uint8_t*>(&data), sizeof(data));
  }

  /**
   * Flush the buffer and call fsync() on the file (only if the file-based constructor is used).
   */
  void fsync();

 private:
  static const std::string kFormatNameRegexStr;
  static const std::regex kFormatNameRegex;
  static const std::string kFieldNameRegexStr;
  static const std::regex kFieldNameRegex;

  struct Format {
    unsigned message_size;
  };
  struct Subscription {
    unsigned message_size;
  };

  void writeDataImpl(uint16_t id, const uint8_t* data, unsigned length);

  std::unique_ptr<Writer> _writer;
  std::FILE* _file{nullptr};

  bool _header_complete{false};
  std::unordered_map<std::string, Format> _formats;
  std::vector<Subscription> _subscriptions;
};

}  // namespace ulog_cpp
