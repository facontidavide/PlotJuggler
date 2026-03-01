/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include "writer.hpp"

namespace ulog_cpp {

Writer::Writer(DataWriteCB data_write_cb) : _data_write_cb(std::move(data_write_cb))
{
  // Writer assumes to run on little endian
  // TODO: use std::endian from C++20
  int num = 1;
  // cppcheck-suppress [knownConditionTrueFalse,unmatchedSuppression]
  if (*reinterpret_cast<char*>(&num) != 1) {
    throw UsageException("Writer requires little endian");
  }
}

void Writer::headerComplete()
{
  _header_complete = true;
}
void Writer::fileHeader(const FileHeader& header)
{
  header.serialize(_data_write_cb);
}
void Writer::messageInfo(const MessageInfo& message_info)
{
  message_info.serialize(_data_write_cb);
}
void Writer::messageFormat(const MessageFormat& message_format)
{
  if (_header_complete) {
    throw UsageException("Header completed, cannot write formats");
  }
  message_format.serialize(_data_write_cb);
}
void Writer::parameter(const Parameter& parameter)
{
  parameter.serialize(_data_write_cb, ULogMessageType::PARAMETER);
}
void Writer::parameterDefault(const ParameterDefault& parameter_default)
{
  parameter_default.serialize(_data_write_cb);
}
void Writer::addLoggedMessage(const AddLoggedMessage& add_logged_message)
{
  if (!_header_complete) {
    throw UsageException("Header not yet completed, cannot write AddLoggedMessage");
  }
  add_logged_message.serialize(_data_write_cb);
}
void Writer::logging(const Logging& logging)
{
  logging.serialize(_data_write_cb);
}
void Writer::data(const Data& data)
{
  data.serialize(_data_write_cb);
}
void Writer::dropout(const Dropout& dropout)
{
  dropout.serialize(_data_write_cb);
}
void Writer::sync(const Sync& sync)
{
  sync.serialize(_data_write_cb);
}
}  // namespace ulog_cpp
