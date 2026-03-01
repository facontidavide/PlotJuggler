/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include "reader.hpp"

#include <cstring>

#include "raw_messages.hpp"

#if 0  // NOLINT (readability-avoid-unconditional-preprocessor-if)
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)
#endif

namespace ulog_cpp {

const std::set<ULogMessageType> Reader::kKnownMessageTypes{{
    ULogMessageType::FORMAT,
    ULogMessageType::DATA,
    ULogMessageType::INFO,
    ULogMessageType::INFO_MULTIPLE,
    ULogMessageType::PARAMETER,
    ULogMessageType::PARAMETER_DEFAULT,
    ULogMessageType::ADD_LOGGED_MSG,
    ULogMessageType::REMOVE_LOGGED_MSG,
    ULogMessageType::SYNC,
    ULogMessageType::DROPOUT,
    ULogMessageType::LOGGING,
    ULogMessageType::LOGGING_TAGGED,
    ULogMessageType::FLAG_BITS,
}};

// cppcheck-suppress [uninitMemberVar,unmatchedSuppression]
Reader::Reader(std::shared_ptr<DataHandlerInterface> data_handler_interface)
    : _data_handler_interface(std::move(data_handler_interface))
{
  // Reader assumes to run on little endian
  // TODO: use std::endian from C++20
  int num = 1;
  // cppcheck-suppress [knownConditionTrueFalse,unmatchedSuppression]
  if (*reinterpret_cast<char*>(&num) != 1) {
    _data_handler_interface->error("Reader requires little endian", false);
    _state = State::InvalidData;
  }
  _partial_message_buffer = static_cast<uint8_t*>(malloc(kBufferSizeInit));
  _partial_message_buffer_length_capacity = kBufferSizeInit;
}

Reader::~Reader()
{
  if (_partial_message_buffer) {
    free(_partial_message_buffer);
  }
}

void Reader::readChunk(const uint8_t* data, int length)
{
  if (_state == State::InvalidData) {
    return;
  }

  if (_state == State::ReadMagic) {
    const int num_read = readMagic(data, length);
    data += num_read;
    length -= num_read;
    _total_num_read += num_read;
  }

  if (_state == State::ReadFlagBits && length > 0) {
    const int num_read = readFlagBits(data, length);
    data += num_read;
    length -= num_read;
    _total_num_read += num_read;
  }

  static constexpr int kULogHeaderLength = static_cast<int>(sizeof(ulog_message_header_s));

  while (length > 0 && !_need_recovery) {
    // Try to get a full ulog message. There's 2 options:
    // - we have some partial data in the buffer. We need to append and use that buffer
    // - no partial data left. Use 'data' if it contains a full message
    const uint8_t* ulog_message = nullptr;
    bool clear_from_partial_message_buffer = false;
    if (_partial_message_buffer_length > 0) {
      auto ensure_enough_data_in_partial_buffer = [&](int required_data) -> bool {
        if (_partial_message_buffer_length < required_data) {
          // Try to append
          const int num_append = std::min(required_data - _partial_message_buffer_length, length);
          if (_partial_message_buffer_length + num_append >
              _partial_message_buffer_length_capacity) {
            // Overflow, resize buffer
            _partial_message_buffer_length_capacity = _partial_message_buffer_length + num_append;
            _partial_message_buffer = static_cast<uint8_t*>(
                realloc(_partial_message_buffer, _partial_message_buffer_length_capacity));
            DBG_PRINTF("%i: resized partial buffer to %i\n", _total_num_read,
                       _partial_message_buffer_length_capacity);
          }
          memcpy(_partial_message_buffer + _partial_message_buffer_length, data, num_append);
          _partial_message_buffer_length += num_append;
          data += num_append;
          length -= num_append;
          _total_num_read += num_append;
        }
        return _partial_message_buffer_length >= required_data;
      };
      if (ensure_enough_data_in_partial_buffer(kULogHeaderLength)) {
        const ulog_message_header_s* header =
            reinterpret_cast<const ulog_message_header_s*>(_partial_message_buffer);
        if (ensure_enough_data_in_partial_buffer(header->msg_size + kULogHeaderLength)) {
          ulog_message = reinterpret_cast<const uint8_t*>(_partial_message_buffer);
          clear_from_partial_message_buffer = true;
        } else {
          // Not enough data yet (length == 0) or overflow
          DBG_PRINTF("%i: not enough data (length=%i)\n", _total_num_read, length);
        }
      }

    } else {
      int full_message_length = 0;
      if (length > kULogHeaderLength) {
        const ulog_message_header_s* header = reinterpret_cast<const ulog_message_header_s*>(data);
        if (length >= header->msg_size + kULogHeaderLength) {
          full_message_length = header->msg_size + kULogHeaderLength;
        }
      }
      if (full_message_length > 0) {
        ulog_message = data;
        data += full_message_length;
        length -= full_message_length;
        _total_num_read += full_message_length;
      } else {
        // Not a full message in buffer -> add to partial buffer
        const int num_append = appendToPartialBuffer(data, length);
        data += num_append;
        length -= num_append;
        _total_num_read += num_append;
      }
    }

    if (ulog_message) {
      const ulog_message_header_s* header =
          reinterpret_cast<const ulog_message_header_s*>(ulog_message);

      // Check for corruption
      if (header->msg_size == 0 || header->msg_type == 0) {
        DBG_PRINTF("%i: Invalid msg detected\n", _total_num_read);
        corruptionDetected();
        // We'll exit the loop afterwards
      } else {
        // Parse the message
        try {
          if (_state == State::ReadHeader) {
            readHeaderMessage(ulog_message);
          }
          if (_state == State::ReadData) {
            readDataMessage(ulog_message);
          }
        } catch (const ParsingException& exception) {
          DBG_PRINTF("%i: parser exception: %s\n", _total_num_read, exception.what());
          corruptionDetected();
        }
      }

      if (clear_from_partial_message_buffer) {
        // In most cases this will clear the whole buffer, but in case of corruptions we might have
        // more data
        const int num_remove = header->msg_size + kULogHeaderLength;
        memmove(_partial_message_buffer, _partial_message_buffer + num_remove,
                _partial_message_buffer_length - num_remove);
        _partial_message_buffer_length -= num_remove;
      }
    }
  }

  if (_need_recovery) {
    tryToRecover(data, length);
  }
}

void Reader::tryToRecover(const uint8_t* data, int length)
{
  // Try to find a valid message in 'data' by moving data into the partial buffer and search for a
  // message
  while (length > 0) {
    const int num_append = appendToPartialBuffer(data, length);
    data += num_append;
    length -= num_append;
    _total_num_read += num_append;

    if (_partial_message_buffer_length >= static_cast<int>(sizeof(ulog_message_header_s))) {
      bool found = false;
      int index = 0;
      // If the partial buffer was already full, skip the first index, otherwise we risk infinite
      // recursion
      if (num_append == 0) {
        index = 1;
      }
      for (;
           index < _partial_message_buffer_length - static_cast<int>(sizeof(ulog_message_header_s));
           ++index) {
        const ulog_message_header_s* header =
            reinterpret_cast<const ulog_message_header_s*>(_partial_message_buffer + index);
        // Try to use it if it looks sane (we could also check for a SYNC message)
        if (header->msg_size != 0 && header->msg_type != 0 && header->msg_size < 10000 &&
            kKnownMessageTypes.find(static_cast<ULogMessageType>(header->msg_type)) !=
                kKnownMessageTypes.end()) {
          found = true;
          break;
        }
      }

      // Discard unused data
      if (index > 0) {
        memmove(_partial_message_buffer, _partial_message_buffer + index,
                _partial_message_buffer_length - index);
        _partial_message_buffer_length -= index;
      }

      if (found) {
        DBG_PRINTF(
            "%i: recovered, recursive call (index = %i, length = %i, partial buf len = %i)\n",
            _total_num_read, index, length, _partial_message_buffer_length);
        _need_recovery = false;
        readChunk(data, length);

        return;
      }
      DBG_PRINTF("%i: no valid msg found (length = %i, partial buf len = %i)\n", _total_num_read,
                 length, _partial_message_buffer_length);
    }
  }
}

void Reader::corruptionDetected()
{
  if (!_corruption_reported) {
    _data_handler_interface->error("Message corruption detected", true);
    _corruption_reported = true;
  }
  _need_recovery = true;
}

int Reader::appendToPartialBuffer(const uint8_t* data, int length)
{
  const int num_append =
      std::min(length, _partial_message_buffer_length_capacity - _partial_message_buffer_length);
  memcpy(_partial_message_buffer + _partial_message_buffer_length, data, num_append);
  _partial_message_buffer_length += num_append;
  return num_append;
}

int Reader::readMagic(const uint8_t* data, int length)
{
  // Assume we read the whole magic in one piece. If needed, we could handle reading it in multiple
  // bits. Note that this could also happen for truncated files.
  if (length < static_cast<int>(sizeof(ulog_file_header_s))) {
    _data_handler_interface->error("Not enough data to read file magic", false);
    _state = State::InvalidData;
    return 0;
  }

  const ulog_file_header_s* header = reinterpret_cast<const ulog_file_header_s*>(data);

  // Check magic bytes
  if (memcmp(header->magic, ulog_file_magic_bytes, sizeof(ulog_file_magic_bytes)) != 0) {
    _data_handler_interface->error("Invalid file format (incorrect header bytes)", false);
    _state = State::InvalidData;
    return 0;
  }

  _state = State::ReadFlagBits;
  _file_header = *header;

  return sizeof(ulog_file_header_s);
}

int Reader::readFlagBits(const uint8_t* data, int length)
{
  // Assume we read the whole flags in one piece (for simplicity of the parser)
  int ret = 0;
  if (length < static_cast<int>(sizeof(ulog_message_flag_bits_s))) {
    _data_handler_interface->error("Not enough data to read file flags", false);
    _state = State::InvalidData;
    return 0;
  }
  // This message is optional and follows directly the file magic
  const ulog_message_flag_bits_s* flag_bits =
      reinterpret_cast<const ulog_message_flag_bits_s*>(data);
  if (static_cast<ULogMessageType>(flag_bits->msg_type) == ULogMessageType::FLAG_BITS) {
    // This is expected to be the first message after the file magic
    if (flag_bits->appended_offsets[0] != 0) {
      // TODO: handle appended data
      _data_handler_interface->error("File contains appended offsets - this is not supported",
                                     true);
    }
    // Check incompat flags
    bool has_incompat_flags = false;
    if (flag_bits->incompat_flags[0] & ~(ULOG_INCOMPAT_FLAG0_DATA_APPENDED_MASK)) {
      has_incompat_flags = true;
    }
    for (unsigned i = 1;
         i < sizeof(flag_bits->incompat_flags) / sizeof(flag_bits->incompat_flags[0]); ++i) {
      if (flag_bits->incompat_flags[i]) {
        has_incompat_flags = true;
      }
    }
    if (has_incompat_flags) {
      _data_handler_interface->error("Unknown incompatible flag set: cannot parse the log", false);
      _state = State::InvalidData;
    } else {
      _data_handler_interface->fileHeader({_file_header, *flag_bits});
      ret = flag_bits->msg_size + ULOG_MSG_HEADER_LEN;
      _state = State::ReadHeader;
    }
  } else {
    // Create header w/o flag bits
    _data_handler_interface->fileHeader(FileHeader{_file_header});
    _state = State::ReadHeader;
  }
  return ret;
}

void Reader::readHeaderMessage(const uint8_t* message)
{
  const ulog_message_header_s* header = reinterpret_cast<const ulog_message_header_s*>(message);
  switch (static_cast<ULogMessageType>(header->msg_type)) {
    case ULogMessageType::INFO:
      _data_handler_interface->messageInfo(MessageInfo{message, false});
      break;
    case ULogMessageType::INFO_MULTIPLE:
      _data_handler_interface->messageInfo(MessageInfo{message, true});
      break;
    case ULogMessageType::FORMAT:
      _data_handler_interface->messageFormat(MessageFormat{message});
      break;
    case ULogMessageType::PARAMETER:
      _data_handler_interface->parameter(Parameter{message});
      break;
    case ULogMessageType::PARAMETER_DEFAULT:
      _data_handler_interface->parameterDefault(ParameterDefault{message});
      break;
    case ULogMessageType::ADD_LOGGED_MSG:
    case ULogMessageType::LOGGING:
    case ULogMessageType::LOGGING_TAGGED:
      DBG_PRINTF("%i: Header completed\n", _total_num_read);
      _state = State::ReadData;
      _data_handler_interface->headerComplete();
      break;
    default:
      DBG_PRINTF("%i: Unknown/unexpected message type in header: %i\n", _total_num_read,
                 header->msg_size);
      break;
  }
}

void Reader::readDataMessage(const uint8_t* message)
{
  const ulog_message_header_s* header = reinterpret_cast<const ulog_message_header_s*>(message);
  switch (static_cast<ULogMessageType>(header->msg_type)) {
    case ULogMessageType::INFO:
      _data_handler_interface->messageInfo(MessageInfo{message, false});
      break;
    case ULogMessageType::INFO_MULTIPLE:
      _data_handler_interface->messageInfo(MessageInfo{message, true});
      break;
    case ULogMessageType::PARAMETER:
      _data_handler_interface->parameter(Parameter{message});
      break;
    case ULogMessageType::PARAMETER_DEFAULT:
      _data_handler_interface->parameterDefault(ParameterDefault{message});
      break;
    case ULogMessageType::ADD_LOGGED_MSG:
      _data_handler_interface->addLoggedMessage(AddLoggedMessage{message});
      break;
    case ULogMessageType::LOGGING:
      _data_handler_interface->logging(Logging{message});
      break;
    case ULogMessageType::LOGGING_TAGGED:
      _data_handler_interface->logging(Logging{message, true});
      break;
    case ULogMessageType::DATA:
      _data_handler_interface->data(Data{message});
      break;
    case ULogMessageType::DROPOUT:
      _data_handler_interface->dropout(Dropout{message});
      break;
    case ULogMessageType::SYNC:
      _data_handler_interface->sync(Sync{message});
      break;
    default:
      DBG_PRINTF("%i: Unknown/unexpected message type in data: %i\n", _total_num_read,
                 header->msg_size);
      break;
  }
}

}  // namespace ulog_cpp
