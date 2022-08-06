// Copyright 2022, Foxglove Technologies. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "message_definition_cache.hpp"

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <ament_index_cpp/get_resource.hpp>
#include <ament_index_cpp/get_resources.hpp>

#include <fstream>
#include <regex>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

namespace RosMsgParser {

// Match datatype names (foo_msgs/Bar or foo_msgs/msg/Bar)
static const std::regex MSG_DATATYPE_REGEX{R"(^([a-zA-Z0-9_]+)/(?:msg/)?([a-zA-Z0-9_]+)$)"};

// Match field types from .msg definitions ("foo_msgs/Bar" in "foo_msgs/Bar[] bar")
static const std::regex FIELD_TYPE_REGEX{R"((?:^|\n)\s*([a-zA-Z0-9_/]+)(?:\[[^\]]*\])?\s+)"};

static const std::unordered_set<std::string> PRIMITIVE_TYPES{
  "bool",  "byte",   "char",  "float32", "float64", "int8",   "uint8",
  "int16", "uint16", "int32", "uint32",  "int64",   "uint64", "string"};

static std::set<std::string> parse_dependencies(const std::string& text,
                                                const std::string& package_context) {
  std::set<std::string> dependencies;
  for (std::sregex_iterator iter(text.begin(), text.end(), FIELD_TYPE_REGEX);
       iter != std::sregex_iterator(); ++iter) {
    std::string type = (*iter)[1];
    if (PRIMITIVE_TYPES.find(type) != PRIMITIVE_TYPES.end()) {
      continue;
    }
    if (type.find('/') == std::string::npos) {
      dependencies.insert(package_context + '/' + std::move(type));
    } else {
      dependencies.insert(std::move(type));
    }
  }
  return dependencies;
}

MessageSpec::MessageSpec(std::string text, const std::string& package_context)
    : dependencies(parse_dependencies(text, package_context))
    , text(std::move(text)) {}

const MessageSpec& MessageDefinitionCache::load_message_spec(const std::string& datatype) {
  if (auto it = msg_specs_by_datatype_.find(datatype); it != msg_specs_by_datatype_.end()) {
    return it->second;
  }

  std::smatch match;
  if (!std::regex_match(datatype, match, MSG_DATATYPE_REGEX)) {
    throw std::invalid_argument("Invalid datatype name: " + datatype);
  }
  std::string package = match[1];
  std::string share_dir = ament_index_cpp::get_package_share_directory(package);
  std::ifstream file{share_dir + "/msg/" + match[2].str() + ".msg"};

  std::string contents{std::istreambuf_iterator(file), {}};
  const MessageSpec& spec =
    msg_specs_by_datatype_.emplace(datatype, MessageSpec(std::move(contents), package))
      .first->second;

  // "References and pointers to data stored in the container are only invalidated by erasing that
  // element, even when the corresponding iterator is invalidated."
  return spec;
}

std::string MessageDefinitionCache::get_full_text(const std::string& root_datatype) {
  std::string result;
  std::unordered_set<std::string> seen_deps = {root_datatype};
  std::function<void(const std::string&)> append_recursive = [&](const std::string& datatype) {
    const MessageSpec& spec = load_message_spec(datatype);
    if (!result.empty()) {
      result +=
        "\n================================================================================\nMSG: ";
      result += datatype;
      result += '\n';
    }
    result += spec.text;
    for (const auto& dep : spec.dependencies) {
      bool inserted = seen_deps.insert(dep).second;
      if (inserted) {
        append_recursive(dep);
      }
    }
  };
  append_recursive(root_datatype);
  return result;
}

}  // namespace rosbag2_storage_mcap::internal
