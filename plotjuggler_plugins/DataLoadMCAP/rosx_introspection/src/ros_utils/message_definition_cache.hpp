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

#ifndef MESSAGE_DEFINITION_CACHE_HPP_
#define MESSAGE_DEFINITION_CACHE_HPP_

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace RosMsgParser {

struct MessageSpec {
  MessageSpec(std::string text, const std::string& package_context);
  const std::set<std::string> dependencies;
  const std::string text;
};

class MessageDefinitionCache final {
public:
  /**
   * Concatenate the message definition with its dependencies into a self-contained schema.
   * Uses a format similar to ROS 1's gendeps:
   * https://github.com/ros/ros/blob/93d8da32091b8b43702eab5d3202f4511dfeb7dc/core/roslib/src/roslib/gentools.py#L239
   */
  std::string get_full_text(const std::string& datatype);

private:
  /**
   * Load and parse the message file referenced by the given datatype, or return it from
   * msg_specs_by_datatype
   */
  const MessageSpec& load_message_spec(const std::string& datatype);

  std::unordered_map<std::string, MessageSpec> msg_specs_by_datatype_;
};

}  // namespace rosbag2_storage_mcap::internal

#endif  // MESSAGE_DEFINITION_CACHE_HPP_
