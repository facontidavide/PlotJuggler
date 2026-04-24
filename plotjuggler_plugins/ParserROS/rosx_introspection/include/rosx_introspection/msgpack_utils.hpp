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
#pragma once

#include <cstdint>
#include <vector>

#include "rosx_introspection/ros_parser.hpp"

namespace RosMsgParser {

/**
 * @brief Convert a FlatMessage to MessagePack format
 *
 * This function serializes a parsed ROS message (FlatMessage) into MessagePack binary format.
 * The FlatMessage contains key-value pairs where keys are field paths and values are variants
 * representing the actual field values.
 *
 * The MessagePack output is a map where:
 * - Keys are string field paths (e.g., "header.stamp.sec")
 * - Values are the corresponding field values encoded according to their type
 *
 * @param flat_msg The parsed ROS message to convert
 * @param msgpack_data Output buffer that will be filled with MessagePack binary data.
 *                     The buffer is cleared and resized as needed.
 */
void convertToMsgpack(const FlatMessage& flat_msg, std::vector<uint8_t>& msgpack_data);

/**
 * @brief Deserialize directly to MessagePack, bypassing FlatMessage.
 *
 * Uses MsgpackMessageWriter with walkSchema to produce msgpack output in a single pass.
 * This is faster than deserialize() + convertToMsgpack() because it avoids the
 * intermediate FlatMessage allocation and FieldLeaf copies.
 */
bool deserializeToMsgpack(
    const Parser& parser, Span<const uint8_t> buffer, Deserializer* deserializer, std::vector<uint8_t>& msgpack_data);

}  // namespace RosMsgParser
