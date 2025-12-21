#include "internal.hpp"

namespace mcap {

constexpr std::string_view OpCodeString(OpCode opcode) {
  switch (opcode) {
    case OpCode::Header:
      return "Header";
    case OpCode::Footer:
      return "Footer";
    case OpCode::Schema:
      return "Schema";
    case OpCode::Channel:
      return "Channel";
    case OpCode::Message:
      return "Message";
    case OpCode::Chunk:
      return "Chunk";
    case OpCode::MessageIndex:
      return "MessageIndex";
    case OpCode::ChunkIndex:
      return "ChunkIndex";
    case OpCode::Attachment:
      return "Attachment";
    case OpCode::AttachmentIndex:
      return "AttachmentIndex";
    case OpCode::Statistics:
      return "Statistics";
    case OpCode::Metadata:
      return "Metadata";
    case OpCode::MetadataIndex:
      return "MetadataIndex";
    case OpCode::SummaryOffset:
      return "SummaryOffset";
    case OpCode::DataEnd:
      return "DataEnd";
    default:
      return "Unknown";
  }
}

MetadataIndex::MetadataIndex(const Metadata& metadata, ByteOffset fileOffset)
    : offset(fileOffset)
    , length(9 + 4 + metadata.name.size() + 4 + internal::KeyValueMapSize(metadata.metadata))
    , name(metadata.name) {}

bool RecordOffset::operator==(const RecordOffset& other) const {
  if (chunkOffset != std::nullopt && other.chunkOffset != std::nullopt) {
    if (*chunkOffset != *other.chunkOffset) {
      // messages are in separate chunks, cannot be equal.
      return false;
    }
    // messages are in the same chunk, compare chunk-level offsets.
    return (offset == other.offset);
  }
  if (chunkOffset != std::nullopt || other.chunkOffset != std::nullopt) {
    // one message is in a chunk and one is not, cannot be equal.
    return false;
  }
  // neither message is in a chunk, compare file-level offsets.
  return (offset == other.offset);
}

bool RecordOffset::operator>(const RecordOffset& other) const {
  if (chunkOffset != std::nullopt) {
    if (other.chunkOffset != std::nullopt) {
      if (*chunkOffset == *other.chunkOffset) {
        // messages are in the same chunk, compare chunk-level offsets.
        return (offset > other.offset);
      }
      // messages are in separate chunks, compare file-level offsets
      return (*chunkOffset > *other.chunkOffset);
    } else {
      // this message is in a chunk, other is not, compare file-level offsets.
      return (*chunkOffset > other.offset);
    }
  }
  if (other.chunkOffset != std::nullopt) {
    // other message is in a chunk, this is not, compare file-level offsets.
    return (offset > *other.chunkOffset);
  }
  // neither message is in a chunk, compare file-level offsets.
  return (offset > other.offset);
}

}  // namespace mcap
