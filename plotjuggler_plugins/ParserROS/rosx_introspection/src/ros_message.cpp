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

#include <string_view>
#include <sstream>
#include <regex>
#include "rosx_introspection/ros_message.hpp"

namespace RosMsgParser{

ROSMessage::ROSMessage(const std::string &msg_def)
{
  std::istringstream messageDescriptor(msg_def);
  std::match_results<std::string::const_iterator> what;

  for (std::string line; std::getline(messageDescriptor, line, '\n') ; )
  {
    std::string::const_iterator begin = line.begin(), end = line.end();

    // Skip empty line or one that is a comment
    if (std::regex_search( begin, end, what,
                          std::regex("(^\\s*$|^\\s*#)")))
    {
      continue;
    }

    TrimStringLeft(line);

    if( line.compare(0, 5, "MSG: ") == 0)
    {
      line.erase(0,5);
      _type = ROSType(line);
    }
    else{
      auto new_field = ROSField(line);
      _fields.push_back(new_field);
    }
  }
}

std::vector<std::string> SplitMultipleMessageDefinitions(const std::string &multi_def)
{
  std::istringstream ss_msg(multi_def);

  std::vector<std::string> parts;
  std::string part;

  for (std::string line; std::getline(ss_msg, line, '\n') ; )
  {
    if( line.find("========") == 0)
    {
      parts.emplace_back( std::move(part) );
      part = {};
    }
    else{
      part.append(line);
      part.append("\n");
    }
  }
  parts.emplace_back( std::move(part) );

  return parts;
}

std::vector<ROSMessage::Ptr> ParseMessageDefinitions(
  const std::string& multi_def,
  const ROSType& root_type )
{
  auto parts = SplitMultipleMessageDefinitions(multi_def);
  std::vector<ROSType> known_type;
  std::vector<ROSMessage::Ptr> parsed_msgs;

  const ROSType no_type;

  // iterating in reverse to fill known_type in the right order
  // i.e. with no missing dependencies
  for( int i = parts.size()-1; i>=0; i--)
  {
    auto msg = std::make_shared<ROSMessage>(parts[i]);

    if( i == 0 )
    {
      // type NOT found in the definition, but provided as argument
      if( msg->type() == no_type && root_type != no_type )
      {
        msg->setType( root_type );
      }
      else if (msg->type() == no_type && root_type == no_type)
      {
        std::cout << multi_def << std::endl;
        throw std::runtime_error("Message type unspecified");
      }
    }

    // add to vector
    parsed_msgs.push_back( msg );
    known_type.push_back( msg->type() );
  }

  // adjust types with undefined package type
  for (auto& msg: parsed_msgs)
  {
    for (ROSField& field : msg->fields())
    {
      // if package name is missing, try to find msgName in the list of known_type
      if (field.type().pkgName().empty())
      {
        for (const ROSType& known_type : known_type)
        {
          if (field.type().msgName() == known_type.msgName())
          {
            field.changeType(known_type);
            break;
          }
        }
      }
    }
  }

  std::reverse(parsed_msgs.begin(), parsed_msgs.end());
  return parsed_msgs;
}

MessageSchema::Ptr BuildMessageSchema(const std::string &topic_name,
                               const std::vector<ROSMessage::Ptr>& parsed_msgs)
{
  auto schema = std::make_shared<MessageSchema>();
  schema->topic_name = topic_name;
  schema->root_msg = parsed_msgs.front();

  for(const auto& msg: parsed_msgs )
  {
    schema->msg_library.insert( {msg->type(), msg} );
  }

  /// build field tree
  std::function<void(ROSMessage::Ptr, FieldTreeNode*)> recursiveTreeCreator;

  recursiveTreeCreator = [&](ROSMessage::Ptr msg, FieldTreeNode* field_node)
  {
    // note: should use reserve here, NOT resize
    const size_t NUM_FIELDS = msg->fields().size();
    field_node->children().reserve(NUM_FIELDS);

    for (const ROSField& field : msg->fields())
    {
      if (field.isConstant())
      {
        continue;
      }
      // Let's add first a child to string_node
      field_node->addChild(&field);
      FieldTreeNode* new_string_node = &(field_node->children().back());

      // builtin types will not trigger a recursion
      if (field.type().isBuiltin() == false)
      {
        auto new_msg = field.getMessagePtr(schema->msg_library);
        if( !new_msg )
        {
          throw std::runtime_error("Missing ROSType in library");
        }

        recursiveTreeCreator(new_msg, new_string_node);

      }
    }    // end of for fields
  };     // end of recursiveTreeCreator

  // build root and start recursion
  auto root_field = new ROSField( schema->root_msg->type(), topic_name);
  schema->field_tree.root()->setValue( root_field );

  recursiveTreeCreator(schema->root_msg, schema->field_tree.root());

  return schema;
}

} // end namespace


