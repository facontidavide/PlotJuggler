#pragma once

#include "intervaltree.hpp"
#include "read_job_queue.hpp"
#include "types.hpp"
#include "visibility.hpp"
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mcap {

enum struct ReadSummaryMethod {
  /**
   * @brief Parse the Summary section to produce seeking indexes and summary
   * statistics. If the Summary section is not present or corrupt, a failure
   * Status is returned and the seeking indexes and summary statistics are not
   * populated.
   */
  NoFallbackScan,
  /**
   * @brief If the Summary section is missing or incomplete, allow falling back
   * to reading the file sequentially to produce seeking indexes and summary
   * statistics.
   */
  AllowFallbackScan,
  /**
   * @brief Read the file sequentially from Header to DataEnd to produce seeking
   * indexes and summary statistics.
   */
  ForceScan,
};

/**
 * @brief An abstract interface for reading MCAP data.
 */
struct MCAP_PUBLIC IReadable {
  virtual ~IReadable() = default;

  /**
   * @brief Returns the size of the file in bytes.
   *
   * @return uint64_t The total number of bytes in the MCAP file.
   */
  virtual uint64_t size() const = 0;
  /**
   * @brief This method is called by MCAP reader classes when they need to read
   * a portion of the file.
   *
   * @param output A pointer to a pointer to the buffer to write to. This method
   *   is expected to either maintain an internal buffer, read data into it, and
   *   update this pointer to point at the internal buffer, or update this
   *   pointer to point directly at the source data if possible. The pointer and
   *   data must remain valid and unmodified until the next call to read().
   * @param offset The offset in bytes from the beginning of the file to read.
   * @param size The number of bytes to read.
   * @return uint64_t Number of bytes actually read. This may be less than the
   *   requested size if the end of the file is reached. The output pointer must
   *   be readable from `output` to `output + size`. If the read fails, this
   *   method should return 0.
   */
  virtual uint64_t read(std::byte** output, uint64_t offset, uint64_t size) = 0;
};

/**
 * @brief IReadable implementation wrapping a FILE* pointer created by fopen()
 * and a read buffer.
 */
class MCAP_PUBLIC FileReader final : public IReadable {
public:
  FileReader(std::FILE* file);

  uint64_t size() const override;
  uint64_t read(std::byte** output, uint64_t offset, uint64_t size) override;

private:
  std::FILE* file_;
  std::vector<std::byte> buffer_;
  uint64_t size_;
  uint64_t position_;
};

/**
 * @brief IReadable implementation wrapping a std::ifstream input file stream.
 */
class MCAP_PUBLIC FileStreamReader final : public IReadable {
public:
  FileStreamReader(std::ifstream& stream);

  uint64_t size() const override;
  uint64_t read(std::byte** output, uint64_t offset, uint64_t size) override;

private:
  std::ifstream& stream_;
  std::vector<std::byte> buffer_;
  uint64_t size_;
  uint64_t position_;
};

/**
 * @brief An abstract interface for compressed readers.
 */
class MCAP_PUBLIC ICompressedReader : public IReadable {
public:
  virtual ~ICompressedReader() override = default;

  /**
   * @brief Reset the reader state, clearing any internal buffers and state, and
   * initialize with new compressed data.
   *
   * @param data Compressed data to read from.
   * @param size Size of the compressed data in bytes.
   * @param uncompressedSize Size of the data in bytes after decompression. A
   *   buffer of this size will be allocated for the uncompressed data.
   */
  virtual void reset(const std::byte* data, uint64_t size, uint64_t uncompressedSize) = 0;
  /**
   * @brief Report the current status of decompression. A StatusCode other than
   * `StatusCode::Success` after `reset()` is called indicates the decompression
   * was not successful and the reader is in an invalid state.
   */
  virtual Status status() const = 0;
};

/**
 * @brief A "null" compressed reader that directly passes through uncompressed
 * data. No internal buffers are allocated.
 */
class MCAP_PUBLIC BufferReader final : public ICompressedReader {
public:
  void reset(const std::byte* data, uint64_t size, uint64_t uncompressedSize) override;
  uint64_t read(std::byte** output, uint64_t offset, uint64_t size) override;
  uint64_t size() const override;
  Status status() const override;

  BufferReader() = default;
  BufferReader(const BufferReader&) = delete;
  BufferReader& operator=(const BufferReader&) = delete;
  BufferReader(BufferReader&&) = delete;
  BufferReader& operator=(BufferReader&&) = delete;

private:
  const std::byte* data_;
  uint64_t size_;
};

#ifndef MCAP_COMPRESSION_NO_ZSTD
/**
 * @brief ICompressedReader implementation that decompresses Zstandard
 * (https://facebook.github.io/zstd/) data.
 */
class MCAP_PUBLIC ZStdReader final : public ICompressedReader {
public:
  void reset(const std::byte* data, uint64_t size, uint64_t uncompressedSize) override;
  uint64_t read(std::byte** output, uint64_t offset, uint64_t size) override;
  uint64_t size() const override;
  Status status() const override;

  /**
   * @brief Decompresses an entire Zstd-compressed chunk into `output`.
   *
   * @param data The Zstd-compressed input chunk.
   * @param compressedSize The size of the Zstd-compressed input.
   * @param uncompressedSize The size of the data once uncompressed.
   * @param output The output vector. This will be resized to `uncompressedSize` to fit the data,
   * or 0 if the decompression encountered an error.
   * @return Status
   */
  static Status DecompressAll(const std::byte* data, uint64_t compressedSize,
                              uint64_t uncompressedSize, ByteArray* output);
  ZStdReader() = default;
  ZStdReader(const ZStdReader&) = delete;
  ZStdReader& operator=(const ZStdReader&) = delete;
  ZStdReader(ZStdReader&&) = delete;
  ZStdReader& operator=(ZStdReader&&) = delete;

private:
  Status status_;
  ByteArray uncompressedData_;
};
#endif

#ifndef MCAP_COMPRESSION_NO_LZ4
/**
 * @brief ICompressedReader implementation that decompresses LZ4
 * (https://lz4.github.io/lz4/) data.
 */
class MCAP_PUBLIC LZ4Reader final : public ICompressedReader {
public:
  void reset(const std::byte* data, uint64_t size, uint64_t uncompressedSize) override;
  uint64_t read(std::byte** output, uint64_t offset, uint64_t size) override;
  uint64_t size() const override;
  Status status() const override;

  /**
   * @brief Decompresses an entire LZ4-encoded chunk into `output`.
   *
   * @param data The LZ4-compressed input chunk.
   * @param size The size of the LZ4-compressed input.
   * @param uncompressedSize The size of the data once uncompressed.
   * @param output The output vector. This will be resized to `uncompressedSize` to fit the data,
   * or 0 if the decompression encountered an error.
   * @return Status
   */
  Status decompressAll(const std::byte* data, uint64_t size, uint64_t uncompressedSize,
                       ByteArray* output);
  LZ4Reader();
  LZ4Reader(const LZ4Reader&) = delete;
  LZ4Reader& operator=(const LZ4Reader&) = delete;
  LZ4Reader(LZ4Reader&&) = delete;
  LZ4Reader& operator=(LZ4Reader&&) = delete;
  ~LZ4Reader() override;

private:
  void* decompressionContext_ = nullptr;  // LZ4F_dctx*
  Status status_;
  const std::byte* compressedData_;
  ByteArray uncompressedData_;
  uint64_t compressedSize_;
  uint64_t uncompressedSize_;
};
#endif

struct LinearMessageView;

/**
 * @brief Options for reading messages out of an MCAP file.
 */
struct MCAP_PUBLIC ReadMessageOptions {
public:
  /**
   * @brief Only messages with log timestamps greater or equal to startTime will be included.
   */
  Timestamp startTime = 0;
  /**
   * @brief Only messages with log timestamps less than endTime will be included.
   */
  Timestamp endTime = MaxTime;
  /**
   * @brief If provided, `topicFilter` is called on all topics found in the MCAP file. If
   * `topicFilter` returns true for a given channel, messages from that channel will be included.
   * if not provided, messages from all channels are provided.
   */
  std::function<bool(std::string_view)> topicFilter;
  enum struct ReadOrder { FileOrder, LogTimeOrder, ReverseLogTimeOrder };
  /**
   * @brief Set the expected order that messages should be returned in.
   * if readOrder == FileOrder, messages will be returned in the order they appear in the MCAP file.
   * if readOrder == LogTimeOrder, messages will be returned in ascending log time order.
   * if readOrder == ReverseLogTimeOrder, messages will be returned in descending log time order.
   */
  ReadOrder readOrder = ReadOrder::FileOrder;

  ReadMessageOptions(Timestamp start, Timestamp end)
      : startTime(start)
      , endTime(end) {}

  ReadMessageOptions() = default;

  /**
   * @brief validate the configuration.
   */
  Status validate() const;
};

/**
 * @brief Provides a read interface to an MCAP file.
 */
class MCAP_PUBLIC McapReader final {
public:
  ~McapReader();

  /**
   * @brief Opens an MCAP file for reading from an already constructed IReadable
   * implementation.
   *
   * @param reader An implementation of the IReader interface that provides raw
   *   MCAP data.
   * @return Status StatusCode::Success on success. If a non-success Status is
   *   returned, the data source is not considered open and McapReader is not
   *   usable until `open()` is called and a success response is returned.
   */
  Status open(IReadable& reader);
  /**
   * @brief Opens an MCAP file for reading from a given filename.
   *
   * @param filename Filename to open.
   * @return Status StatusCode::Success on success. If a non-success Status is
   *   returned, the data source is not considered open and McapReader is not
   *   usable until `open()` is called and a success response is returned.
   */
  Status open(std::string_view filename);
  /**
   * @brief Opens an MCAP file for reading from a std::ifstream input file
   * stream.
   *
   * @param stream Input file stream to read MCAP data from.
   * @return Status StatusCode::Success on success. If a non-success Status is
   *   returned, the file is not considered open and McapReader is not usable
   *   until `open()` is called and a success response is returned.
   */
  Status open(std::ifstream& stream);

  /**
   * @brief Closes the MCAP file, clearing any internal data structures and
   * state and dropping the data source reference.
   *
   */
  void close();

  /**
   * @brief Read and parse the Summary section at the end of the MCAP file, if
   * available. This will populate internal indexes to allow for efficient
   * summarization and random access. This method will automatically be called
   * upon requesting summary data or first seek if Summary section parsing is
   * allowed by the configuration options.
   */
  Status readSummary(
    ReadSummaryMethod method, const ProblemCallback& onProblem = [](const Status&) {});

  /**
   * @brief Returns an iterable view with `begin()` and `end()` methods for
   * iterating Messages in the MCAP file. If a non-zero `startTime` is provided,
   * this will first parse the Summary section (by calling `readSummary()`) if
   * allowed by the configuration options and it has not been parsed yet.
   *
   * @param startTime Optional start time in nanoseconds. Messages before this
   *   time will not be returned.
   * @param endTime Optional end time in nanoseconds. Messages equal to or after
   *   this time will not be returned.
   */
  LinearMessageView readMessages(Timestamp startTime = 0, Timestamp endTime = MaxTime);
  /**
   * @brief Returns an iterable view with `begin()` and `end()` methods for
   * iterating Messages in the MCAP file. If a non-zero `startTime` is provided,
   * this will first parse the Summary section (by calling `readSummary()`) if
   * allowed by the configuration options and it has not been parsed yet.
   *
   * @param onProblem A callback that will be called when a parsing error
   *   occurs. Problems can either be recoverable, indicating some data could
   *   not be read, or non-recoverable, stopping the iteration.
   * @param startTime Optional start time in nanoseconds. Messages before this
   *   time will not be returned.
   * @param endTime Optional end time in nanoseconds. Messages equal to or after
   *   this time will not be returned.
   */
  LinearMessageView readMessages(const ProblemCallback& onProblem, Timestamp startTime = 0,
                                 Timestamp endTime = MaxTime);

  /**
   * @brief Returns an iterable view with `begin()` and `end()` methods for
   * iterating Messages in the MCAP file.
   * Uses the options from `options` to select the messages that are yielded.
   */
  LinearMessageView readMessages(const ProblemCallback& onProblem,
                                 const ReadMessageOptions& options);

  /**
   * @brief Returns starting and ending byte offsets that must be read to
   * iterate all messages in the given time range. If `readSummary()` has been
   * successfully called and the recording contains Chunk records, this range
   * will be narrowed to Chunk records that contain messages in the given time
   * range. Otherwise, this range will be the entire Data section if the Data
   * End record has been found or the entire file otherwise.
   *
   * This method is automatically used by `readMessages()`, and only needs to be
   * called directly if the caller is manually constructing an iterator.
   *
   * @param startTime Start time in nanoseconds.
   * @param endTime Optional end time in nanoseconds.
   * @return Start and end byte offsets.
   */
  std::pair<ByteOffset, ByteOffset> byteRange(Timestamp startTime,
                                              Timestamp endTime = MaxTime) const;

  /**
   * @brief Returns a pointer to the IReadable data source backing this reader.
   * Will return nullptr if the reader is not open.
   */
  IReadable* dataSource();

  /**
   * @brief Returns the parsed Header record, if it has been encountered.
   */
  const std::optional<Header>& header() const;
  /**
   * @brief Returns the parsed Footer record, if it has been encountered.
   */
  const std::optional<Footer>& footer() const;
  /**
   * @brief Returns the parsed Statistics record, if it has been encountered.
   */
  const std::optional<Statistics>& statistics() const;

  /**
   * @brief Returns all of the parsed Channel records. Call `readSummary()`
   * first to fully populate this data structure.
   */
  const std::unordered_map<ChannelId, ChannelPtr> channels() const;
  /**
   * @brief Returns all of the parsed Schema records. Call `readSummary()`
   * first to fully populate this data structure.
   */
  const std::unordered_map<SchemaId, SchemaPtr> schemas() const;

  /**
   * @brief Look up a Channel record by channel ID. If the Channel has not been
   * encountered yet or does not exist in the file, this will return nullptr.
   *
   * @param channelId Channel ID to search for
   * @return ChannelPtr A shared pointer to a Channel record, or nullptr
   */
  ChannelPtr channel(ChannelId channelId) const;
  /**
   * @brief Look up a Schema record by schema ID. If the Schema has not been
   * encountered yet or does not exist in the file, this will return nullptr.
   *
   * @param schemaId Schema ID to search for
   * @return SchemaPtr A shared pointer to a Schema record, or nullptr
   */
  SchemaPtr schema(SchemaId schemaId) const;

  /**
   * @brief Returns all of the parsed ChunkIndex records. Call `readSummary()`
   * first to fully populate this data structure.
   */
  const std::vector<ChunkIndex>& chunkIndexes() const;

  /**
   * @brief Returns all of the parsed MetadataIndex records. Call `readSummary()`
   * first to fully populate this data structure.
   * The multimap's keys are the `name` field from each indexed Metadata.
   */
  const std::multimap<std::string, MetadataIndex>& metadataIndexes() const;

  /**
   * @brief Returns all of the parsed AttachmentIndex records. Call `readSummary()`
   * first to fully populate this data structure.
   * The multimap's keys are the `name` field from each indexed Attachment.
   */
  const std::multimap<std::string, AttachmentIndex>& attachmentIndexes() const;

  // The following static methods are used internally for parsing MCAP records
  // and do not need to be called directly unless you are implementing your own
  // reader functionality or tests.

  static Status ReadRecord(IReadable& reader, uint64_t offset, Record* record);
  static Status ReadFooter(IReadable& reader, uint64_t offset, Footer* footer);

  static Status ParseHeader(const Record& record, Header* header);
  static Status ParseFooter(const Record& record, Footer* footer);
  static Status ParseSchema(const Record& record, Schema* schema);
  static Status ParseChannel(const Record& record, Channel* channel);
  static Status ParseMessage(const Record& record, Message* message);
  static Status ParseChunk(const Record& record, Chunk* chunk);
  static Status ParseMessageIndex(const Record& record, MessageIndex* messageIndex);
  static Status ParseChunkIndex(const Record& record, ChunkIndex* chunkIndex);
  static Status ParseAttachment(const Record& record, Attachment* attachment);
  static Status ParseAttachmentIndex(const Record& record, AttachmentIndex* attachmentIndex);
  static Status ParseStatistics(const Record& record, Statistics* statistics);
  static Status ParseMetadata(const Record& record, Metadata* metadata);
  static Status ParseMetadataIndex(const Record& record, MetadataIndex* metadataIndex);
  static Status ParseSummaryOffset(const Record& record, SummaryOffset* summaryOffset);
  static Status ParseDataEnd(const Record& record, DataEnd* dataEnd);

  /**
   * @brief Converts a compression string ("", "zstd", "lz4") to the Compression enum.
   */
  static std::optional<Compression> ParseCompression(const std::string_view compression);

private:
  using ChunkInterval = internal::Interval<ByteOffset, ChunkIndex>;
  friend LinearMessageView;

  IReadable* input_ = nullptr;
  std::FILE* file_ = nullptr;
  std::unique_ptr<FileReader> fileInput_;
  std::unique_ptr<FileStreamReader> fileStreamInput_;
  std::optional<Header> header_;
  std::optional<Footer> footer_;
  std::optional<Statistics> statistics_;
  std::vector<ChunkIndex> chunkIndexes_;
  internal::IntervalTree<ByteOffset, ChunkIndex> chunkRanges_;
  std::multimap<std::string, AttachmentIndex> attachmentIndexes_;
  std::multimap<std::string, MetadataIndex> metadataIndexes_;
  std::unordered_map<SchemaId, SchemaPtr> schemas_;
  std::unordered_map<ChannelId, ChannelPtr> channels_;
  ByteOffset dataStart_ = 0;
  ByteOffset dataEnd_ = EndOffset;
  bool parsedSummary_ = false;

  void reset_();
  Status readSummarySection_(IReadable& reader);
  Status readSummaryFromScan_(IReadable& reader);
};

/**
 * @brief A low-level interface for parsing MCAP-style TLV records from a data
 * source.
 */
struct MCAP_PUBLIC RecordReader {
  ByteOffset offset;
  ByteOffset endOffset;

  RecordReader(IReadable& dataSource, ByteOffset startOffset, ByteOffset endOffset = EndOffset);

  void reset(IReadable& dataSource, ByteOffset startOffset, ByteOffset endOffset);

  std::optional<Record> next();

  const Status& status() const;

  ByteOffset curRecordOffset() const;

private:
  IReadable* dataSource_ = nullptr;
  Status status_;
  Record curRecord_;
};

struct MCAP_PUBLIC TypedChunkReader {
  std::function<void(const SchemaPtr, ByteOffset)> onSchema;
  std::function<void(const ChannelPtr, ByteOffset)> onChannel;
  std::function<void(const Message&, ByteOffset)> onMessage;
  std::function<void(const Record&, ByteOffset)> onUnknownRecord;

  TypedChunkReader();
  TypedChunkReader(const TypedChunkReader&) = delete;
  TypedChunkReader& operator=(const TypedChunkReader&) = delete;
  TypedChunkReader(TypedChunkReader&&) = delete;
  TypedChunkReader& operator=(TypedChunkReader&&) = delete;

  void reset(const Chunk& chunk, Compression compression);

  bool next();

  ByteOffset offset() const;

  const Status& status() const;

private:
  RecordReader reader_;
  Status status_;
  BufferReader uncompressedReader_;
#ifndef MCAP_COMPRESSION_NO_LZ4
  LZ4Reader lz4Reader_;
#endif
#ifndef MCAP_COMPRESSION_NO_ZSTD
  ZStdReader zstdReader_;
#endif
};

/**
 * @brief A mid-level interface for parsing and validating MCAP records from a
 * data source.
 */
struct MCAP_PUBLIC TypedRecordReader {
  std::function<void(const Header&, ByteOffset)> onHeader;
  std::function<void(const Footer&, ByteOffset)> onFooter;
  std::function<void(const SchemaPtr, ByteOffset, std::optional<ByteOffset>)> onSchema;
  std::function<void(const ChannelPtr, ByteOffset, std::optional<ByteOffset>)> onChannel;
  std::function<void(const Message&, ByteOffset, std::optional<ByteOffset>)> onMessage;
  std::function<void(const Chunk&, ByteOffset)> onChunk;
  std::function<void(const MessageIndex&, ByteOffset)> onMessageIndex;
  std::function<void(const ChunkIndex&, ByteOffset)> onChunkIndex;
  std::function<void(const Attachment&, ByteOffset)> onAttachment;
  std::function<void(const AttachmentIndex&, ByteOffset)> onAttachmentIndex;
  std::function<void(const Statistics&, ByteOffset)> onStatistics;
  std::function<void(const Metadata&, ByteOffset)> onMetadata;
  std::function<void(const MetadataIndex&, ByteOffset)> onMetadataIndex;
  std::function<void(const SummaryOffset&, ByteOffset)> onSummaryOffset;
  std::function<void(const DataEnd&, ByteOffset)> onDataEnd;
  std::function<void(const Record&, ByteOffset, std::optional<ByteOffset>)> onUnknownRecord;
  std::function<void(ByteOffset)> onChunkEnd;

  TypedRecordReader(IReadable& dataSource, ByteOffset startOffset,
                    ByteOffset endOffset = EndOffset);

  TypedRecordReader(const TypedRecordReader&) = delete;
  TypedRecordReader& operator=(const TypedRecordReader&) = delete;
  TypedRecordReader(TypedRecordReader&&) = delete;
  TypedRecordReader& operator=(TypedRecordReader&&) = delete;

  bool next();

  ByteOffset offset() const;

  const Status& status() const;

private:
  RecordReader reader_;
  TypedChunkReader chunkReader_;
  Status status_;
  bool parsingChunk_;
};

/**
 * @brief Uses message indices to read messages out of an MCAP in log time order.
 * The underlying MCAP must be chunked, with a summary section and message indexes.
 * The required McapWriterOptions are:
 *  - noChunking: false
 *  - noMessageIndex: false
 *  - noSummary: false
 */
struct MCAP_PUBLIC IndexedMessageReader {
public:
  IndexedMessageReader(McapReader& reader, const ReadMessageOptions& options,
                       const std::function<void(const Message&, RecordOffset)> onMessage);

  /**
   * @brief reads the next message out of the MCAP.
   *
   * @return true if a message was found.
   * @return false if no more messages are to be read. If there was some error reading the MCAP,
   * `status()` will return a non-Success status.
   */
  bool next();

  /**
   * @brief gets the status of the reader.
   *
   * @return Status
   */
  Status status() const;

private:
  struct ChunkSlot {
    ByteArray decompressedChunk;
    ByteOffset chunkStartOffset;
    int unreadMessages = 0;
  };
  size_t findFreeChunkSlot();
  void decompressChunk(const Chunk& chunk, ChunkSlot& slot);
  Status status_;
  McapReader& mcapReader_;
  RecordReader recordReader_;
#ifndef MCAP_COMPRESSION_NO_LZ4
  LZ4Reader lz4Reader_;
#endif
  ReadMessageOptions options_;
  std::unordered_set<ChannelId> selectedChannels_;
  std::function<void(const Message&, RecordOffset)> onMessage_;
  internal::ReadJobQueue queue_;
  std::vector<ChunkSlot> chunkSlots_;
};

/**
 * @brief An iterable view of Messages in an MCAP file.
 */
struct MCAP_PUBLIC LinearMessageView {
  struct MCAP_PUBLIC Iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type = int64_t;
    using value_type = MessageView;
    using pointer = const MessageView*;
    using reference = const MessageView&;

    reference operator*() const;
    pointer operator->() const;
    Iterator& operator++();
    void operator++(int);
    MCAP_PUBLIC friend bool operator==(const Iterator& a, const Iterator& b);
    MCAP_PUBLIC friend bool operator!=(const Iterator& a, const Iterator& b);

  private:
    friend LinearMessageView;

    Iterator() = default;
    Iterator(LinearMessageView& view);

    class Impl {
    public:
      Impl(LinearMessageView& view);

      Impl(const Impl&) = delete;
      Impl& operator=(const Impl&) = delete;
      Impl(Impl&&) = delete;
      Impl& operator=(Impl&&) = delete;

      void increment();
      reference dereference() const;
      bool has_value() const;

      LinearMessageView& view_;

      std::optional<TypedRecordReader> recordReader_;
      std::optional<IndexedMessageReader> indexedMessageReader_;
      Message curMessage_;
      std::optional<MessageView> curMessageView_;

    private:
      void onMessage(const Message& message, RecordOffset offset);
    };

    bool begun_ = false;
    std::unique_ptr<Impl> impl_;
  };

  LinearMessageView(McapReader& mcapReader, const ProblemCallback& onProblem);
  LinearMessageView(McapReader& mcapReader, ByteOffset dataStart, ByteOffset dataEnd,
                    Timestamp startTime, Timestamp endTime, const ProblemCallback& onProblem);
  LinearMessageView(McapReader& mcapReader, const ReadMessageOptions& options, ByteOffset dataStart,
                    ByteOffset dataEnd, const ProblemCallback& onProblem);

  LinearMessageView(const LinearMessageView&) = delete;
  LinearMessageView& operator=(const LinearMessageView&) = delete;
  LinearMessageView(LinearMessageView&&) = default;
  LinearMessageView& operator=(LinearMessageView&&) = delete;

  Iterator begin();
  Iterator end();

private:
  McapReader& mcapReader_;
  ByteOffset dataStart_;
  ByteOffset dataEnd_;
  ReadMessageOptions readMessageOptions_;
  const ProblemCallback onProblem_;
};

}  // namespace mcap

#ifdef MCAP_IMPLEMENTATION
#  include "reader.inl"
#endif
