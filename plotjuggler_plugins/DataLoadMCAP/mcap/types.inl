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

}  // namespace mcap
