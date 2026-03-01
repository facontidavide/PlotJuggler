/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include "simple_writer.hpp"

#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <fileapi.h>
// clang-format on
#else
#include <unistd.h>
#endif

namespace ulog_cpp {

const std::string SimpleWriter::kFormatNameRegexStr = "[a-zA-Z0-9_\\-/]+";
const std::regex SimpleWriter::kFormatNameRegex = std::regex(std::string(kFormatNameRegexStr));
const std::string SimpleWriter::kFieldNameRegexStr = "[a-z0-9_]+";
const std::regex SimpleWriter::kFieldNameRegex = std::regex(std::string(kFieldNameRegexStr));

SimpleWriter::SimpleWriter(DataWriteCB data_write_cb, uint64_t timestamp_us)
    : _writer(std::make_unique<Writer>(std::move(data_write_cb)))
{
  _writer->fileHeader(FileHeader(timestamp_us));
}

SimpleWriter::SimpleWriter(const std::string& filename, uint64_t timestamp_us)
{
  _file = std::fopen(filename.c_str(), "wb");
  if (!_file) {
    throw ParsingException("Failed to open file");
  }

  _writer = std::make_unique<Writer>(
      [this](const uint8_t* data, int length) { std::fwrite(data, 1, length, _file); });
  _writer->fileHeader(FileHeader(timestamp_us));
}

SimpleWriter::~SimpleWriter()
{
  _writer.reset();
  if (_file) {
    std::fclose(_file);
  }
}

void SimpleWriter::writeMessageFormat(const std::string& name, const std::vector<Field>& fields)
{
  if (_header_complete) {
    throw UsageException("Header already complete");
  }
  // Ensure the first field is the 64 bit timestamp. This is a bit stricter than what ULog requires
  if (fields.empty() || fields[0].name() != "timestamp" ||
      fields[0].type().type != Field::BasicType::UINT64 || fields[0].arrayLength() != -1) {
    throw UsageException("First message field must be 'uint64_t timestamp'");
  }
  if (_formats.find(name) != _formats.end()) {
    throw UsageException("Duplicate format: " + name);
  }

  // Validate naming pattern
  if (!std::regex_match(name, kFormatNameRegex)) {
    throw UsageException("Invalid name: " + name + ", valid regex: " + kFormatNameRegexStr);
  }
  for (const auto& field : fields) {
    if (!std::regex_match(field.name(), kFieldNameRegex)) {
      throw UsageException("Invalid field name: " + field.name() +
                           ", valid regex: " + kFieldNameRegexStr);
    }
  }

  // Check field types and verify padding
  unsigned message_size = 0;
  for (const auto& field : fields) {
    const auto& basic_type_iter = Field::kBasicTypes.find(field.type().name);
    if (basic_type_iter == Field::kBasicTypes.end()) {
      throw UsageException("Invalid field type (nested formats are not supported): " +
                           field.type().name);
    }
    const int array_size = field.arrayLength() <= 0 ? 1 : field.arrayLength();
    if (message_size % basic_type_iter->second.size != 0) {
      throw UsageException(
          "struct requires padding, reorder fields by decreasing type size. Padding before "
          "field: " +
          field.name());
    }
    message_size += array_size * basic_type_iter->second.size;
  }
  _formats[name] = Format{message_size};
  _writer->messageFormat(MessageFormat(name, fields));
}

void SimpleWriter::headerComplete()
{
  if (_header_complete) {
    throw UsageException("Header already complete");
  }
  _writer->headerComplete();
  _header_complete = true;
}

void SimpleWriter::writeTextMessage(Logging::Level level, const std::string& message,
                                    uint64_t timestamp)
{
  if (!_header_complete) {
    throw UsageException("Header not yet complete");
  }
  _writer->logging({level, message, timestamp});
}

void SimpleWriter::fsync()
{
  if (_file) {
    fflush(_file);
#ifdef _WIN32
    FlushFileBuffers(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(_fileno(_file))));
#else
    ::fsync(fileno(_file));
#endif
  }
}
uint16_t SimpleWriter::writeAddLoggedMessage(const std::string& message_format_name,
                                             uint8_t multi_id)
{
  if (!_header_complete) {
    throw UsageException("Header not yet complete");
  }
  const uint16_t msg_id = _subscriptions.size();
  auto format_iter = _formats.find(message_format_name);
  if (format_iter == _formats.end()) {
    throw UsageException("Format not found: " + message_format_name);
  }
  _subscriptions.push_back({format_iter->second.message_size});
  _writer->addLoggedMessage(AddLoggedMessage(multi_id, msg_id, message_format_name));
  return msg_id;
}

void SimpleWriter::writeDataImpl(uint16_t id, const uint8_t* data, unsigned length)
{
  if (!_header_complete) {
    throw UsageException("Header not yet complete");
  }
  if (id >= _subscriptions.size()) {
    throw UsageException("Invalid ID");
  }
  const unsigned expected_size = _subscriptions[id].message_size;
  // Sanity check data size. sizeof(data) can be bigger because of struct padding at the end
  if (length < expected_size) {
    throw UsageException("sizeof(data) is too small");
  }
  std::vector<uint8_t> data_vec;
  data_vec.resize(expected_size);
  memcpy(data_vec.data(), data, expected_size);
  _writer->data(Data(id, std::move(data_vec)));
}

}  // namespace ulog_cpp
