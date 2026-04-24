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

#include "rosx_introspection/stringtree_leaf.hpp"

namespace RosMsgParser {

// Helper: fill bracket placeholders in a cached path template using segment memcpy.
static size_t fillBrackets(
    char* buf, const char* tmpl, size_t tmpl_size, const uint16_t* offsets, uint8_t num_brackets,
    const SmallVector<uint16_t, 4>& index_array) {
  size_t out_off = 0;
  size_t src_off = 0;

  for (uint8_t i = 0; i < num_brackets; i++) {
    size_t seg_len = offsets[i] - src_off;
    std::memcpy(buf + out_off, tmpl + src_off, seg_len);
    out_off += seg_len;

    buf[out_off++] = '[';
    if (i < index_array.size()) {
      out_off += print_number(buf + out_off, index_array[i]);
    }
    buf[out_off++] = ']';
    src_off = offsets[i] + 2;
  }

  size_t tail_len = tmpl_size - src_off;
  if (tail_len > 0) {
    std::memcpy(buf + out_off, tmpl + src_off, tail_len);
    out_off += tail_len;
  }

  return out_off;
}

template <typename KeySuffixes>
static size_t keySuffixBytes(const KeySuffixes& key_suffixes) {
  size_t total = 0;
  for (const auto& entry : key_suffixes) {
    total += entry.suffix.len;
  }
  return total;
}

template <typename KeySuffixes>
static void renderPathByDepth(
    const FieldTreeNode* node, const SmallVector<uint16_t, 4>& index_array,
    const SmallVector<uint16_t, 4>& index_depth_array, const KeySuffixes& key_suffixes,
    const KeySuffix& legacy_key_suffix, std::string& out) {
  SmallVector<const FieldTreeNode*, 8> nodes;
  for (const auto* current = node; current != nullptr; current = current->parent()) {
    nodes.push_back(current);
  }

  size_t reserve_size = nodes.size() > 1 ? nodes.size() - 1 : 0;
  for (const auto* current : nodes) {
    if (current->value()) {
      reserve_size += current->value()->name().size();
    }
  }
  reserve_size += index_array.size() * 5 + keySuffixBytes(key_suffixes) + legacy_key_suffix.len;

  out.clear();
  out.reserve(reserve_size);

  for (size_t reverse_index = nodes.size(); reverse_index > 0; reverse_index--) {
    const auto* current = nodes[reverse_index - 1];
    const auto depth = static_cast<uint16_t>(nodes.size() - reverse_index);

    if (depth > 0) {
      out.push_back('/');
    }
    if (const auto* field = current->value()) {
      out += field->name();
    }

    for (size_t i = 0; i < index_array.size(); i++) {
      if (i >= index_depth_array.size() || index_depth_array[i] != depth) {
        continue;
      }
      char num_buf[16];
      out.push_back('[');
      out.append(num_buf, print_number(num_buf, index_array[i]));
      out.push_back(']');
    }

    for (const auto& entry : key_suffixes) {
      if (entry.depth == depth && !entry.suffix.empty()) {
        out.append(entry.suffix.data, entry.suffix.len);
      }
    }
  }

  if (!legacy_key_suffix.empty()) {
    out.append(legacy_key_suffix.data, legacy_key_suffix.len);
  }
}

static void renderCachedPath(
    const FieldTreeNode* node, const SmallVector<uint16_t, 4>& index_array, const KeySuffix& key_suffix,
    std::string& out) {
  if (!node) {
    out.clear();
    return;
  }
  const auto& tmpl = node->cachedPath();
  const uint8_t num_brackets = node->bracketCount();

  if (num_brackets == 0 && key_suffix.empty()) {
    out = tmpl;
    return;
  }

  size_t extra = num_brackets * 5 + key_suffix.len;
  out.resize(tmpl.size() + extra);

  size_t offset = fillBrackets(out.data(), tmpl.data(), tmpl.size(), node->bracketOffsets(), num_brackets, index_array);

  if (!key_suffix.empty()) {
    std::memcpy(out.data() + offset, key_suffix.data, key_suffix.len);
    offset += key_suffix.len;
  }

  out.resize(offset);
}

// FieldLeaf::toStr — renders array indices and key suffixes at their path segment depth.
void FieldLeaf::toStr(std::string& out) const {
  if (!node) {
    out.clear();
    return;
  }

  if (index_depth_array.size() != index_array.size()) {
    renderCachedPath(node, index_array, key_suffix, out);
    return;
  }

  renderPathByDepth(node, index_array, index_depth_array, key_suffixes, key_suffix, out);
}

// FieldsVector — kept for backward compatibility
FieldsVector::FieldsVector(const FieldLeaf& leaf) : _node(leaf.node) {
  index_array = leaf.index_array;
  index_depth_array = leaf.index_depth_array;
  key_suffixes = leaf.key_suffixes;
  key_suffix = leaf.key_suffix;
}

void FieldsVector::toStr(std::string& out) const {
  if (!_node) {
    out.clear();
    return;
  }

  if (index_depth_array.size() != index_array.size()) {
    renderCachedPath(_node, index_array, key_suffix, out);
    return;
  }

  renderPathByDepth(_node, index_array, index_depth_array, key_suffixes, key_suffix, out);
}

}  // namespace RosMsgParser
