#pragma once

#include "errors.hpp"
#include "visibility.hpp"
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mcap {

#define MCAP_LIBRARY_VERSION "1.3.0"

using SchemaId = uint16_t;
using ChannelId = uint16_t;
using Timestamp = uint64_t;
using ByteOffset = uint64_t;
using KeyValueMap = std::unordered_map<std::string, std::string>;
using ByteArray = std::vector<std::byte>;
using ProblemCallback = std::function<void(const Status&)>;

constexpr char SpecVersion = '0';
constexpr char LibraryVersion[] = MCAP_LIBRARY_VERSION;
constexpr uint8_t Magic[] = {137, 77, 67, 65, 80, SpecVersion, 13, 10};  // "\x89MCAP0\r\n"
constexpr uint64_t DefaultChunkSize = 1024 * 768;
constexpr ByteOffset EndOffset = std::numeric_limits<ByteOffset>::max();
constexpr Timestamp MaxTime = std::numeric_limits<Timestamp>::max();

/**
 * @brief Supported MCAP compression algorithms.
 */
enum struct Compression {
  None,
  Lz4,
  Zstd,
};

/**
 * @brief Compression level to use when compression is enabled. Slower generally
 * produces smaller files, at the expense of more CPU time. These levels map to
 * different internal settings for each compression algorithm.
 */
enum struct CompressionLevel {
  Fastest,
  Fast,
  Default,
  Slow,
  Slowest,
};

/**
 * @brief MCAP record types.
 */
enum struct OpCode : uint8_t {
  Header = 0x01,
  Footer = 0x02,
  Schema = 0x03,
  Channel = 0x04,
  Message = 0x05,
  Chunk = 0x06,
  MessageIndex = 0x07,
  ChunkIndex = 0x08,
  Attachment = 0x09,
  AttachmentIndex = 0x0A,
  Statistics = 0x0B,
  Metadata = 0x0C,
  MetadataIndex = 0x0D,
  SummaryOffset = 0x0E,
  DataEnd = 0x0F,
};

/**
 * @brief Get the string representation of an OpCode.
 */
MCAP_PUBLIC
constexpr std::string_view OpCodeString(OpCode opcode);

/**
 * @brief A generic Type-Length-Value record using a uint8 type and uint64
 * length. This is the generic form of all MCAP records.
 */
struct MCAP_PUBLIC Record {
  OpCode opcode;
  uint64_t dataSize;
  std::byte* data;

  uint64_t recordSize() const {
    return sizeof(opcode) + sizeof(dataSize) + dataSize;
  }
};

/**
 * @brief Appears at the beginning of every MCAP file (after the magic byte
 * sequence) and contains the recording profile (see
 * <https://github.com/foxglove/mcap/tree/main/docs/specification/profiles>) and
 * a string signature of the recording library.
 */
struct MCAP_PUBLIC Header {
  std::string profile;
  std::string library;
};

/**
 * @brief The final record in an MCAP file (before the trailing magic byte
 * sequence). Contains byte offsets from the start of the file to the Summary
 * and Summary Offset sections, along with an optional CRC of the combined
 * Summary and Summary Offset sections. A `summaryStart` and
 * `summaryOffsetStart` of zero indicates no Summary section is available.
 */
struct MCAP_PUBLIC Footer {
  ByteOffset summaryStart;
  ByteOffset summaryOffsetStart;
  uint32_t summaryCrc;

  Footer() = default;
  Footer(ByteOffset summaryStart, ByteOffset summaryOffsetStart)
      : summaryStart(summaryStart)
      , summaryOffsetStart(summaryOffsetStart)
      , summaryCrc(0) {}
};

/**
 * @brief Describes a schema used for message encoding and decoding and/or
 * describing the shape of messages. One or more Channel records map to a single
 * Schema.
 */
struct MCAP_PUBLIC Schema {
  SchemaId id;
  std::string name;
  std::string encoding;
  ByteArray data;

  Schema() = default;

  Schema(const std::string_view name, const std::string_view encoding, const std::string_view data)
      : name(name)
      , encoding(encoding)
      , data{reinterpret_cast<const std::byte*>(data.data()),
             reinterpret_cast<const std::byte*>(data.data() + data.size())} {}

  Schema(const std::string_view name, const std::string_view encoding, const ByteArray& data)
      : name(name)
      , encoding(encoding)
      , data{data} {}
};

/**
 * @brief Describes a Channel that messages are written to. A Channel represents
 * a single connection from a publisher to a topic, so each topic will have one
 * Channel per publisher. Channels optionally reference a Schema, for message
 * encodings that are not self-describing (e.g. JSON) or when schema information
 * is available (e.g. JSONSchema).
 */
struct MCAP_PUBLIC Channel {
  ChannelId id;
  std::string topic;
  std::string messageEncoding;
  SchemaId schemaId;
  KeyValueMap metadata;

  Channel() = default;

  Channel(const std::string_view topic, const std::string_view messageEncoding, SchemaId schemaId,
          const KeyValueMap& metadata = {})
      : topic(topic)
      , messageEncoding(messageEncoding)
      , schemaId(schemaId)
      , metadata(metadata) {}
};

using SchemaPtr = std::shared_ptr<Schema>;
using ChannelPtr = std::shared_ptr<Channel>;

/**
 * @brief A single Message published to a Channel.
 */
struct MCAP_PUBLIC Message {
  ChannelId channelId;
  /**
   * @brief An optional sequence number. If non-zero, sequence numbers should be
   * unique per channel and increasing over time.
   */
  uint32_t sequence;
  /**
   * @brief Nanosecond timestamp when this message was recorded or received for
   * recording.
   */
  Timestamp logTime;
  /**
   * @brief Nanosecond timestamp when this message was initially published. If
   * not available, this should be set to `logTime`.
   */
  Timestamp publishTime;
  /**
   * @brief Size of the message payload in bytes, pointed to via `data`.
   */
  uint64_t dataSize;
  /**
   * @brief A pointer to the message payload. For readers, this pointer is only
   * valid for the lifetime of an onMessage callback or before the message
   * iterator is advanced.
   */
  const std::byte* data = nullptr;
};

/**
 * @brief An collection of Schemas, Channels, and Messages that supports
 * compression and indexing.
 */
struct MCAP_PUBLIC Chunk {
  Timestamp messageStartTime;
  Timestamp messageEndTime;
  ByteOffset uncompressedSize;
  uint32_t uncompressedCrc;
  std::string compression;
  ByteOffset compressedSize;
  const std::byte* records = nullptr;
};

/**
 * @brief A list of timestamps to byte offsets for a single Channel. This record
 * appears after each Chunk, one per Channel that appeared in that Chunk.
 */
struct MCAP_PUBLIC MessageIndex {
  ChannelId channelId;
  std::vector<std::pair<Timestamp, ByteOffset>> records;
};

/**
 * @brief Chunk Index records are found in the Summary section, providing
 * summary information for a single Chunk and pointing to each Message Index
 * record associated with that Chunk.
 */
struct MCAP_PUBLIC ChunkIndex {
  Timestamp messageStartTime;
  Timestamp messageEndTime;
  ByteOffset chunkStartOffset;
  ByteOffset chunkLength;
  std::unordered_map<ChannelId, ByteOffset> messageIndexOffsets;
  ByteOffset messageIndexLength;
  std::string compression;
  ByteOffset compressedSize;
  ByteOffset uncompressedSize;
};

/**
 * @brief An Attachment is an arbitrary file embedded in an MCAP file, including
 * a name, media type, timestamps, and optional CRC. Attachment records are
 * written in the Data section, outside of Chunks.
 */
struct MCAP_PUBLIC Attachment {
  Timestamp logTime;
  Timestamp createTime;
  std::string name;
  std::string mediaType;
  uint64_t dataSize;
  const std::byte* data = nullptr;
  uint32_t crc;
};

/**
 * @brief Attachment Index records are found in the Summary section, providing
 * summary information for a single Attachment.
 */
struct MCAP_PUBLIC AttachmentIndex {
  ByteOffset offset;
  ByteOffset length;
  Timestamp logTime;
  Timestamp createTime;
  uint64_t dataSize;
  std::string name;
  std::string mediaType;

  AttachmentIndex() = default;
  AttachmentIndex(const Attachment& attachment, ByteOffset fileOffset)
      : offset(fileOffset)
      , length(9 +
               /* name */ 4 + attachment.name.size() +
               /* log_time */ 8 +
               /* create_time */ 8 +
               /* media_type */ 4 + attachment.mediaType.size() +
               /* data */ 8 + attachment.dataSize +
               /* crc */ 4)
      , logTime(attachment.logTime)
      , createTime(attachment.createTime)
      , dataSize(attachment.dataSize)
      , name(attachment.name)
      , mediaType(attachment.mediaType) {}
};

/**
 * @brief The Statistics record is found in the Summary section, providing
 * counts and timestamp ranges for the entire file.
 */
struct MCAP_PUBLIC Statistics {
  uint64_t messageCount;
  uint16_t schemaCount;
  uint32_t channelCount;
  uint32_t attachmentCount;
  uint32_t metadataCount;
  uint32_t chunkCount;
  Timestamp messageStartTime;
  Timestamp messageEndTime;
  std::unordered_map<ChannelId, uint64_t> channelMessageCounts;
};

/**
 * @brief Holds a named map of key/value strings containing arbitrary user data.
 * Metadata records are found in the Data section, outside of Chunks.
 */
struct MCAP_PUBLIC Metadata {
  std::string name;
  KeyValueMap metadata;
};

/**
 * @brief Metadata Index records are found in the Summary section, providing
 * summary information for a single Metadata record.
 */
struct MCAP_PUBLIC MetadataIndex {
  uint64_t offset;
  uint64_t length;
  std::string name;

  MetadataIndex() = default;
  MetadataIndex(const Metadata& metadata, ByteOffset fileOffset);
};

/**
 * @brief Summary Offset records are found in the Summary Offset section.
 * Records in the Summary section are grouped together, and for each record type
 * found in the Summary section, a Summary Offset references the file offset and
 * length where that type of Summary record can be found.
 */
struct MCAP_PUBLIC SummaryOffset {
  OpCode groupOpCode;
  ByteOffset groupStart;
  ByteOffset groupLength;
};

/**
 * @brief The final record in the Data section, signaling the end of Data and
 * beginning of Summary. Optionally contains a CRC of the entire Data section.
 */
struct MCAP_PUBLIC DataEnd {
  uint32_t dataSectionCrc;
};

struct MCAP_PUBLIC RecordOffset {
  ByteOffset offset;
  std::optional<ByteOffset> chunkOffset;

  RecordOffset() = default;
  explicit RecordOffset(ByteOffset offset_)
      : offset(offset_) {}
  RecordOffset(ByteOffset offset_, ByteOffset chunkOffset_)
      : offset(offset_)
      , chunkOffset(chunkOffset_) {}

  bool operator==(const RecordOffset& other) const;
  bool operator>(const RecordOffset& other) const;

  bool operator!=(const RecordOffset& other) const {
    return !(*this == other);
  }
  bool operator>=(const RecordOffset& other) const {
    return ((*this == other) || (*this > other));
  }
  bool operator<(const RecordOffset& other) const {
    return !(*this >= other);
  }
  bool operator<=(const RecordOffset& other) const {
    return !(*this > other);
  }
};

/**
 * @brief Returned when iterating over Messages in a file, MessageView contains
 * a reference to one Message, a pointer to its Channel, and an optional pointer
 * to that Channel's Schema. The Channel pointer is guaranteed to be valid,
 * while the Schema pointer may be null if the Channel references schema_id 0.
 */
struct MCAP_PUBLIC MessageView {
  const Message& message;
  const ChannelPtr channel;
  const SchemaPtr schema;
  const RecordOffset messageOffset;

  MessageView(const Message& message, const ChannelPtr channel, const SchemaPtr schema,
              RecordOffset offset)
      : message(message)
      , channel(channel)
      , schema(schema)
      , messageOffset(offset) {}
};

}  // namespace mcap

#ifdef MCAP_IMPLEMENTATION
#  include "types.inl"
#endif
