#include "rosx_introspection/flat_message_writer.hpp"

#include "rosx_introspection/ros_parser.hpp"

namespace RosMsgParser {

namespace {
template <typename Container>
inline void ExpandVectorIfNecessary(Container& container, size_t new_size) {
  if (container.size() <= new_size) {
    const size_t increased_size = std::max(size_t(32), container.size() * 2);
    container.resize(increased_size);
  }
}

FieldLeaf leafWithTerminalSuffix(const FieldLeaf& leaf, const char* suffix, size_t suffix_size) {
  FieldLeaf suffixed_leaf = leaf;
  suffixed_leaf.key_suffix.assign(suffix, suffix_size);
  return suffixed_leaf;
}
}  // namespace

struct FlatMessageWriter::Impl {
  FlatMessage* flat;
  Parser::BlobPolicy blob_policy;
  size_t value_index = 0;
  size_t blob_index = 0;
  size_t blob_storage_index = 0;
};

FlatMessageWriter::FlatMessageWriter(FlatMessage* flat, int blob_policy) : _impl(std::make_unique<Impl>()) {
  _impl->flat = flat;
  _impl->blob_policy = static_cast<Parser::BlobPolicy>(blob_policy);
}

FlatMessageWriter::~FlatMessageWriter() = default;

void FlatMessageWriter::writeValue(const FieldLeaf& leaf, const Variant& value) {
  ExpandVectorIfNecessary(_impl->flat->value, _impl->value_index);
  _impl->flat->value[_impl->value_index].first = leaf;
  _impl->flat->value[_impl->value_index].second = value;
  _impl->value_index++;
}

void FlatMessageWriter::writeString(const FieldLeaf& leaf, const std::string& str) {
  ExpandVectorIfNecessary(_impl->flat->value, _impl->value_index);
  _impl->flat->value[_impl->value_index].first = leaf;
  _impl->flat->value[_impl->value_index].second = str;
  _impl->value_index++;
}

void FlatMessageWriter::writeEnum(const FieldLeaf& leaf, int32_t value, const std::string& name) {
  writeString(leafWithTerminalSuffix(leaf, "/enum_str", sizeof("/enum_str") - 1), name);
  writeValue(leafWithTerminalSuffix(leaf, "/enum_val", sizeof("/enum_val") - 1), Variant(value));
}

void FlatMessageWriter::writeBlob(const FieldLeaf& leaf, Span<const uint8_t> data) {
  ExpandVectorIfNecessary(_impl->flat->blob, _impl->blob_index);
  _impl->flat->blob[_impl->blob_index].first = leaf;

  if (_impl->blob_policy == Parser::STORE_BLOB_AS_COPY) {
    ExpandVectorIfNecessary(_impl->flat->blob_storage, _impl->blob_storage_index);
    auto& storage = _impl->flat->blob_storage[_impl->blob_storage_index];
    storage.assign(data.data(), data.data() + data.size());
    _impl->flat->blob[_impl->blob_index].second = Span<const uint8_t>(storage.data(), storage.size());
    _impl->blob_storage_index++;
  } else {
    _impl->flat->blob[_impl->blob_index].second = data;
  }
  _impl->blob_index++;
}

void FlatMessageWriter::finish() {
  _impl->flat->value.resize(_impl->value_index);
  _impl->flat->blob.resize(_impl->blob_index);
  _impl->flat->blob_storage.resize(_impl->blob_storage_index);
}

}  // namespace RosMsgParser
