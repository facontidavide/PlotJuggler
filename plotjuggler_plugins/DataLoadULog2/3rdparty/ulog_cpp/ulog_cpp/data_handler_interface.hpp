/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <string>

#include "messages.hpp"

namespace ulog_cpp {

class DataHandlerInterface {
 public:
  virtual void headerComplete() {}

  virtual void error(const std::string& msg, bool is_recoverable) {}

  // Data methods
  virtual void fileHeader(const FileHeader& header) {}
  virtual void messageInfo(const MessageInfo& message_info) {}
  virtual void messageFormat(const MessageFormat& message_format) {}
  virtual void parameter(const Parameter& parameter) {}
  virtual void parameterDefault(const ParameterDefault& parameter_default) {}
  virtual void addLoggedMessage(const AddLoggedMessage& add_logged_message) {}
  virtual void logging(const Logging& logging) {}
  virtual void data(const Data& data) {}
  virtual void dropout(const Dropout& dropout) {}
  virtual void sync(const Sync& sync) {}

 private:
};

}  // namespace ulog_cpp
