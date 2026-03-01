/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <memory>
#include <set>

#include "data_handler_interface.hpp"

namespace ulog_cpp {

/**
 * Class to deserialize an ULog file. Parsed messages are passed back to a DataHandlerInterface
 * class.
 */
class Reader {
 public:
  explicit Reader(std::shared_ptr<DataHandlerInterface> data_handler_interface);
  ~Reader();

  /**
   * Parse next chunk of serialized ULog data. Call this iteratively, e.g. over a complete file.
   * data_handler_interface will be called immediately for each parsed ULog message.
   */
  void readChunk(const uint8_t* data, int length);

 private:
  static constexpr int kBufferSizeInit = 2048;

  static const std::set<ULogMessageType> kKnownMessageTypes;

  int readMagic(const uint8_t* data, int length);
  int readFlagBits(const uint8_t* data, int length);
  void corruptionDetected();
  int appendToPartialBuffer(const uint8_t* data, int length);
  void tryToRecover(const uint8_t* data, int length);

  void readHeaderMessage(const uint8_t* message);
  void readDataMessage(const uint8_t* message);

  enum class State {
    ReadMagic,
    ReadFlagBits,
    ReadHeader,
    ReadData,
    InvalidData,
  };

  State _state{State::ReadMagic};
  const std::shared_ptr<DataHandlerInterface> _data_handler_interface;

  uint8_t* _partial_message_buffer{nullptr};  ///< contains at most one ULog message (unless
                                              ///< _need_recovery==true)
  int _partial_message_buffer_length_capacity{0};
  int _partial_message_buffer_length{0};

  bool _need_recovery{false};
  bool _corruption_reported{false};

  int _total_num_read{};  ///< statistics, total number of bytes read (includes current partial
                          ///< buffer data)

  ulog_file_header_s _file_header{};
};

}  // namespace ulog_cpp
