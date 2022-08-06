/***** MIT License ****
*
*   Copyright (c) 2016-2022 Davide Faconti
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*/

#ifndef ROS_INTROSPECTION_ROSMESSAGE_H
#define ROS_INTROSPECTION_ROSMESSAGE_H

#include <unordered_map>
#include "rosx_introspection/tree.hpp"
#include "rosx_introspection/ros_field.hpp"

namespace RosMsgParser
{

class ROSMessage
{
public:
  using Ptr = std::shared_ptr<ROSMessage>;

  /// This constructor does most of the work in terms of parsing.
  /// It uses the message definition to extract fields and types.
  ROSMessage(const std::string& msg_def );

  const ROSField& field(size_t i) const { return _fields[i]; }

  const std::vector<ROSField>& fields() const { return _fields; }

  std::vector<ROSField>& fields() { return _fields; }

  const ROSType& type() const { return _type; }

  void setType(const ROSType& new_type ) { _type = new_type; }

private:

  ROSType _type;
  std::vector<ROSField> _fields;
};

typedef details::TreeNode<const ROSField*> FieldTreeNode;
typedef details::Tree<const ROSField*> FieldTree;

struct MessageSchema
{
  using Ptr = std::shared_ptr<MessageSchema>;

  std::string topic_name;
  FieldTree   field_tree;
  ROSMessage::Ptr root_msg;
  RosMessageLibrary msg_library;
};

//------------------------------------------------

inline std::ostream& operator<<(std::ostream &os, const ROSMessage& msg )
{
  os << msg.type();
  return os;
}

inline std::ostream& operator<<(std::ostream &os, const ROSMessage* msg )
{
  os << msg->type();
  return os;
}

std::vector<ROSMessage::Ptr> ParseMessageDefinitions(const std::string& multi_def,
                                                     const ROSType &type);

MessageSchema::Ptr BuildMessageSchema(const std::string& topic_name,
                                 const std::vector<ROSMessage::Ptr>& parsed_msgs);

}

#endif
