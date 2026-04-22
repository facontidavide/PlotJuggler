#pragma once

#include <string>

#include "rosx_introspection/ros_message.hpp"

namespace RosMsgParser {

/// Parse a DDS IDL schema string and build a MessageSchema.
///
/// @param topic_name   Name used as the root of the field tree
/// @param root_type    The message type to use as root (e.g.,
/// "AsensusMessaging/ArmState")
/// @param idl_schema   Complete IDL text (all #include'd content must be concatenated)
/// @return             Populated MessageSchema ready for deserialization
MessageSchema::Ptr ParseIDL(const std::string& topic_name, const ROSType& root_type, const std::string& idl_schema);

}  // namespace RosMsgParser
