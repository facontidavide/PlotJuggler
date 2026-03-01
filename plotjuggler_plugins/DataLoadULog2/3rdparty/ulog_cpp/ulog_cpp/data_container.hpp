/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "data_handler_interface.hpp"
#include "subscription.hpp"

namespace ulog_cpp {

class DataContainer : public DataHandlerInterface {
 public:
  enum class StorageConfig {
    Header,   ///< keep header in memory
    FullLog,  ///< keep full log in memory
  };

  struct NameAndMultiIdKey {
    std::string name;
    int multi_id{0};

    NameAndMultiIdKey() = default;

    NameAndMultiIdKey(std::string name, int multi_id) : name(std::move(name)), multi_id(multi_id) {}

    bool operator==(const NameAndMultiIdKey& other) const
    {
      return name == other.name && multi_id == other.multi_id;
    }

    bool operator<(const NameAndMultiIdKey& other) const
    {
      return name < other.name || (name == other.name && multi_id < other.multi_id);
    }

    bool operator>(const NameAndMultiIdKey& other) const
    {
      return name > other.name || (name == other.name && multi_id > other.multi_id);
    }
  };

  explicit DataContainer(StorageConfig storage_config);
  virtual ~DataContainer() = default;

  void error(const std::string& msg, bool is_recoverable) override;

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

  // Stored data
  bool isHeaderComplete() const { return _header_complete; }
  bool hadFatalError() const { return _had_fatal_error; }
  const std::vector<std::string>& parsingErrors() const { return _parsing_errors; }
  const FileHeader& fileHeader() const { return _file_header; }
  const std::map<std::string, MessageInfo>& messageInfo() const { return _message_info; }
  const std::map<std::string, std::vector<std::vector<MessageInfo>>>& messageInfoMulti() const
  {
    return _message_info_multi;
  }
  const std::map<std::string, std::shared_ptr<MessageFormat>>& messageFormats() const
  {
    return _message_formats;
  }
  const std::map<std::string, Parameter>& initialParameters() const { return _initial_parameters; }
  const std::map<std::string, ParameterDefault>& defaultParameters() const
  {
    return _default_parameters;
  }
  const std::vector<Parameter>& changedParameters() const { return _changed_parameters; }
  const std::vector<Logging>& logging() const { return _logging; }
  const std::vector<Dropout>& dropouts() const { return _dropouts; }

  const std::map<NameAndMultiIdKey, std::shared_ptr<Subscription>>& subscriptionsByNameAndMultiId()
  {
    return _subscriptions_by_name_and_multi_id;
  }

  const std::map<uint16_t, std::shared_ptr<Subscription>>& subscriptionsByMessageId()
  {
    return _subscriptions_by_message_id;
  }

  std::set<std::string> subscriptionNames() const
  {
    std::set<std::string> names;
    for (const auto& kv : _subscriptions_by_name_and_multi_id) {
      names.insert(kv.first.name);
    }
    return names;
  }

  std::shared_ptr<Subscription> subscription(const std::string& name, int multi_id = 0) const
  {
    const auto it = _subscriptions_by_name_and_multi_id.find({name, multi_id});
    if (it == _subscriptions_by_name_and_multi_id.end()) {
      throw AccessException("Subscription not found: " + name);
    }
    return it->second;
  }

 protected:
  std::map<std::string, MessageInfo>& messageInfoRef() { return _message_info; }
  std::map<std::string, std::vector<std::vector<MessageInfo>>>& messageInfoMultiRef()
  {
    return _message_info_multi;
  }
  std::map<std::string, Parameter>& initialParametersRef() { return _initial_parameters; }
  std::map<std::string, ParameterDefault>& defaultParametersRef() { return _default_parameters; }
  std::vector<Parameter>& changedParametersRef() { return _changed_parameters; }
  std::vector<Logging>& loggingRef() { return _logging; }
  std::vector<Dropout>& dropoutsRef() { return _dropouts; }

 private:
  const StorageConfig _storage_config;

  bool _header_complete{false};
  bool _had_fatal_error{false};
  std::vector<std::string> _parsing_errors;

  FileHeader _file_header;
  std::map<std::string, MessageInfo> _message_info;
  std::map<std::string, std::vector<std::vector<MessageInfo>>> _message_info_multi;
  std::map<std::string, std::shared_ptr<MessageFormat>> _message_formats;
  std::map<std::string, Parameter> _initial_parameters;
  std::map<std::string, ParameterDefault> _default_parameters;
  std::vector<Parameter> _changed_parameters;
  std::map<uint16_t, std::shared_ptr<Subscription>> _subscriptions_by_message_id;
  std::map<NameAndMultiIdKey, std::shared_ptr<Subscription>> _subscriptions_by_name_and_multi_id;
  std::vector<Logging> _logging;
  std::vector<Dropout> _dropouts;
};

}  // namespace ulog_cpp
