/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <cstdint>
#include <functional>

#include "data_handler_interface.hpp"

/**
 * Low-level class for serializing ULog data. This exposes the full ULog functionality, but does
 * not do integrity checks. Use SimpleWriter for a simpler API with checks.
 */
namespace ulog_cpp {

class Writer : public DataHandlerInterface {
 public:
  explicit Writer(DataWriteCB data_write_cb);
  virtual ~Writer() = default;

  void headerComplete() override;

  void fileHeader(const FileHeader& header) override;
  void messageInfo(const MessageInfo& message_info) override;
  void messageFormat(const MessageFormat& message_format) override;
  void parameter(const Parameter& parameter) override;
  void parameterDefault(const ParameterDefault& parameter_default) override;
  void addLoggedMessage(const AddLoggedMessage& add_logged_message) override;
  void logging(const Logging& logging) override;
  void data(const Data& data) override;
  void dropout(const Dropout& dropout) override;
  void sync(const Sync& sync) override;

 private:
  const DataWriteCB _data_write_cb;
  bool _header_complete{false};
};

}  // namespace ulog_cpp
