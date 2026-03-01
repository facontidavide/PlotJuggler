/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#include "data_container.hpp"

namespace ulog_cpp {

DataContainer::DataContainer(DataContainer::StorageConfig storage_config)
    : _storage_config(storage_config)
{
}
void DataContainer::error(const std::string& msg, bool is_recoverable)
{
  if (!is_recoverable) {
    _had_fatal_error = true;
  }
  _parsing_errors.push_back(msg);
}
void DataContainer::headerComplete()
{
  // try to resolve all fields in message formats
  for (auto& it : _message_formats) {
    auto& message_format = it.second;
    message_format->resolveDefinition(_message_formats);
  }

  // try to resolve all fields for all message infos
  for (auto& it : _message_info) {
    auto& message_info = it.second;
    message_info.field().resolveDefinition(_message_formats, 0);
  }

  // try to resolve all fields for all multi message infos
  for (auto& it : _message_info_multi) {
    for (auto& message_infos_for_field : it.second) {
      for (auto& message_info : message_infos_for_field) {
        message_info.field().resolveDefinition(_message_formats, 0);
      }
    }
  }

  // try to resolve all fields for params
  for (auto& it : _default_parameters) {
    auto& parameter_value = it.second;
    parameter_value.field().resolveDefinition(_message_formats, 0);
  }

  for (auto& it : _initial_parameters) {
    auto& parameter_value = it.second;
    parameter_value.field().resolveDefinition(_message_formats, 0);
  }

  for (auto& parameter_value : _changed_parameters) {
    parameter_value.field().resolveDefinition(_message_formats, 0);
  }

  _header_complete = true;
}
void DataContainer::fileHeader(const FileHeader& header)
{
  _file_header = header;
}
void DataContainer::messageInfo(const MessageInfo& message_info_arg)
{
  // create mutable copy
  MessageInfo message_info = message_info_arg;
  if (_header_complete) {
    // if header is complete, we can resolve definition here
    message_info.field().resolveDefinition(_message_formats, 0);
  }
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  if (message_info.isMulti()) {
    if (message_info.isContinued()) {
      auto& messages = _message_info_multi[message_info.field().name()];
      if (messages.empty()) {
        throw ParsingException("info_multi msg is continued, but no previous");
      }
      messages[messages.size() - 1].push_back(message_info);
    } else {
      _message_info_multi[message_info.field().name()].push_back({message_info});
    }
  } else {
    _message_info.insert({message_info.field().name(), message_info});
  }
}
void DataContainer::messageFormat(const MessageFormat& message_format)
{
  if (_message_formats.find(message_format.name()) != _message_formats.end()) {
    throw ParsingException("Duplicate message format");
  }
  _message_formats.insert({message_format.name(), std::make_shared<MessageFormat>(message_format)});
}
void DataContainer::parameter(const Parameter& parameter_arg)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  if (_header_complete) {
    auto parameter_value = parameter_arg;
    // if header is complete, we can resolve definition here
    parameter_value.field().resolveDefinition(_message_formats, 0);
    _changed_parameters.push_back(parameter_value);
  } else {
    _initial_parameters.insert({parameter_arg.field().name(), parameter_arg});
  }
}
void DataContainer::parameterDefault(const ParameterDefault& parameter_default_arg)
{
  ParameterDefault parameter_default = parameter_default_arg;
  if (_header_complete) {
    // if header is already complete, we can resolve definition here
    parameter_default.field().resolveDefinition(_message_formats, 0);
  }
  _default_parameters.insert({parameter_default.field().name(), parameter_default});
}
void DataContainer::addLoggedMessage(const AddLoggedMessage& add_logged_message)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  if (_subscriptions_by_message_id.find(add_logged_message.msgId()) !=
      _subscriptions_by_message_id.end()) {
    throw ParsingException("Duplicate AddLoggedMessage message ID");
  }

  auto format_iter = _message_formats.find(add_logged_message.messageName());
  if (format_iter == _message_formats.end()) {
    throw ParsingException("AddLoggedMessage message format not found");
  }

  auto new_subscription =
      std::make_shared<Subscription>(add_logged_message, std::vector<Data>{}, format_iter->second);
  _subscriptions_by_message_id.insert({add_logged_message.msgId(), new_subscription});

  const NameAndMultiIdKey key{add_logged_message.messageName(),
                              static_cast<int>(add_logged_message.multiId())};
  _subscriptions_by_name_and_multi_id.insert({key, new_subscription});
}
void DataContainer::logging(const Logging& logging)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  _logging.emplace_back(std::move(logging));
}
void DataContainer::data(const Data& data)
{
  if (_storage_config == StorageConfig::Header) {
    return;
  }
  const auto& iter = _subscriptions_by_message_id.find(data.msgId());
  if (iter == _subscriptions_by_message_id.end()) {
    throw ParsingException("Invalid subscription");
  }
  iter->second->emplaceSample(std::move(data));
}
void DataContainer::dropout(const Dropout& dropout)
{
  if (_header_complete && _storage_config == StorageConfig::Header) {
    return;
  }
  _dropouts.emplace_back(std::move(dropout));
}
}  // namespace ulog_cpp
