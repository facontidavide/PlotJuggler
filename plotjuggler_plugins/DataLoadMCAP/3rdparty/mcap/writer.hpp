#pragma once

#include "types.hpp"
#include "visibility.hpp"
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

// Forward declaration
#ifndef MCAP_COMPRESSION_NO_ZSTD
struct ZSTD_CCtx_s;
#endif

namespace mcap {

/**
 * @brief Configuration options for McapWriter.
 */
struct MCAP_PUBLIC McapWriterOptions {
  /**
   * @brief Disable CRC calculations for Chunks.
   */
  bool noChunkCRC = false;
  /**
   * @brief Disable CRC calculations for Attachments.
   */
  bool noAttachmentCRC = false;
  /**
   * @brief Enable CRC calculations for all records in the data section.
   */
  bool enableDataCRC = false;
  /**
   * @brief Disable CRC calculations for the summary section.
   */
  bool noSummaryCRC = false;
  /**
   * @brief Do not write Chunks to the file, instead writing Schema, Channel,
   * and Message records directly into the Data section.
   */
  bool noChunking = false;
  /**
   * @brief Do not write Message Index records to the file. If
   * `noMessageIndex=true` and `noChunkIndex=false`, Chunk Index records will
   * still be written to the Summary section, providing a coarse message index.
   */
  bool noMessageIndex = false;
  /**
   * @brief Do not write Summary or Summary Offset sections to the file, placing
   * the Footer record immediately after DataEnd. This can provide some speed
   * boost to file writing and produce smaller files, at the expense of
   * requiring a conversion process later if fast summarization or indexed
   * access is desired.
   */
  bool noSummary = false;
  /**
   * @brief Target uncompressed Chunk payload size in bytes. Once a Chunk's
   * uncompressed data is about to exceed this size, the Chunk will be
   * compressed (if enabled) and written to disk. Note that this is a 'soft'
   * ceiling as some Chunks could exceed this size due to either indexing
   * data or when a single message is larger than `chunkSize`, in which case,
   * the Chunk will contain only this one large message.
   * This option is ignored if `noChunking=true`.
   */
  uint64_t chunkSize = DefaultChunkSize;
  /**
   * @brief Compression algorithm to use when writing Chunks. This option is
   * ignored if `noChunking=true`.
   */
  Compression compression = Compression::Zstd;
  /**
   * @brief Compression level to use when writing Chunks. Slower generally
   * produces smaller files, at the expense of more CPU time. These levels map
   * to different internal settings for each compression algorithm.
   */
  CompressionLevel compressionLevel = CompressionLevel::Default;
  /**
   * @brief By default, Chunks that do not benefit from compression will be
   * written uncompressed. This option can be used to force compression on all
   * Chunks. This option is ignored if `noChunking=true`.
   */
  bool forceCompression = false;
  /**
   * @brief The recording profile. See
   * https://mcap.dev/spec/registry#well-known-profiles
   * for more information on well-known profiles.
   */
  std::string profile;
  /**
   * @brief A freeform string written by recording libraries. For this library,
   * the default is "libmcap {Major}.{Minor}.{Patch}".
   */
  std::string library = "libmcap " MCAP_LIBRARY_VERSION;

  // The following options are less commonly used, providing more fine-grained
  // control of index records and the Summary section

  bool noRepeatedSchemas = false;
  bool noRepeatedChannels = false;
  bool noAttachmentIndex = false;
  bool noMetadataIndex = false;
  bool noChunkIndex = false;
  bool noStatistics = false;
  bool noSummaryOffsets = false;

  McapWriterOptions(const std::string_view profile)
      : profile(profile) {}
};

/**
 * @brief An abstract interface for writing MCAP data.
 */
class MCAP_PUBLIC IWritable {
public:
  bool crcEnabled = false;

  IWritable() noexcept;
  virtual ~IWritable() = default;

  /**
   * @brief Called whenever the writer needs to write data to the output MCAP
   * file.
   *
   * @param data A pointer to the data to write.
   * @param size Size of the data in bytes.
   */
  void write(const std::byte* data, uint64_t size);
  /**
   * @brief Called when the writer is finished writing data to the output MCAP
   * file.
   */
  virtual void end() = 0;
  /**
   * @brief Returns the current size of the file in bytes. This must be equal to
   * the sum of all `size` parameters passed to `write()`.
   */
  virtual uint64_t size() const = 0;
  /**
   * @brief Returns the CRC32 of the uncompressed data.
   */
  uint32_t crc();
  /**
   * @brief Resets the CRC32 calculation.
   */
  void resetCrc();

  /**
   * @brief flushes any buffered data to the output. This is called by McapWriter after every
   * completed chunk. Callers may also retain a reference to the writer and call flush() at their
   * own cadence. Defaults to a no-op.
   */
  virtual void flush() {}

protected:
  virtual void handleWrite(const std::byte* data, uint64_t size) = 0;

private:
  uint32_t crc_;
};

/**
 * @brief Implements the IWritable interface used by McapWriter by wrapping a
 * FILE* pointer created by fopen().
 */
class MCAP_PUBLIC FileWriter final : public IWritable {
public:
  ~FileWriter() override;

  Status open(std::string_view filename);

  void handleWrite(const std::byte* data, uint64_t size) override;
  void end() override;
  void flush() override;
  uint64_t size() const override;

private:
  std::FILE* file_ = nullptr;
  uint64_t size_ = 0;
};

/**
 * @brief Implements the IWritable interface used by McapWriter by wrapping a
 * std::ostream stream.
 */
class MCAP_PUBLIC StreamWriter final : public IWritable {
public:
  StreamWriter(std::ostream& stream);

  void handleWrite(const std::byte* data, uint64_t size) override;
  void end() override;
  void flush() override;
  uint64_t size() const override;

private:
  std::ostream& stream_;
  uint64_t size_ = 0;
};

/**
 * @brief An abstract interface for writing Chunk data. Chunk data is buffered
 * in memory and written to disk as a single record, to support optimal
 * compression and calculating the final Chunk data size.
 */
class MCAP_PUBLIC IChunkWriter : public IWritable {
public:
  virtual ~IChunkWriter() override = default;

  /**
   * @brief Called when the writer wants to close the current output Chunk.
   * After this call, `data()` and `size()` should return the data and size of
   * the compressed data.
   */
  virtual void end() override = 0;
  /**
   * @brief Returns the size in bytes of the uncompressed data.
   */
  virtual uint64_t size() const override = 0;

  /**
   * @brief Returns the size in bytes of the compressed data. This will only be
   * called after `end()`.
   */
  virtual uint64_t compressedSize() const = 0;
  /**
   * @brief Returns true if `write()` has never been called since initialization
   * or the last call to `clear()`.
   */
  virtual bool empty() const = 0;
  /**
   * @brief Clear the internal state of the writer, discarding any input or
   * output buffers.
   */
  void clear();
  /**
   * @brief Returns a pointer to the uncompressed data.
   */
  virtual const std::byte* data() const = 0;
  /**
   * @brief Returns a pointer to the compressed data. This will only be called
   * after `end()`.
   */
  virtual const std::byte* compressedData() const = 0;

protected:
  virtual void handleClear() = 0;
};

/**
 * @brief An in-memory IChunkWriter implementation backed by a
 * growable buffer.
 */
class MCAP_PUBLIC BufferWriter final : public IChunkWriter {
public:
  void handleWrite(const std::byte* data, uint64_t size) override;
  void end() override;
  uint64_t size() const override;
  uint64_t compressedSize() const override;
  bool empty() const override;
  void handleClear() override;
  const std::byte* data() const override;
  const std::byte* compressedData() const override;

private:
  std::vector<std::byte> buffer_;
};

#ifndef MCAP_COMPRESSION_NO_LZ4
/**
 * @brief An in-memory IChunkWriter implementation that holds data in a
 * temporary buffer before flushing to an LZ4-compressed buffer.
 */
class MCAP_PUBLIC LZ4Writer final : public IChunkWriter {
public:
  LZ4Writer(CompressionLevel compressionLevel, uint64_t chunkSize);

  void handleWrite(const std::byte* data, uint64_t size) override;
  void end() override;
  uint64_t size() const override;
  uint64_t compressedSize() const override;
  bool empty() const override;
  void handleClear() override;
  const std::byte* data() const override;
  const std::byte* compressedData() const override;

private:
  std::vector<std::byte> uncompressedBuffer_;
  std::vector<std::byte> compressedBuffer_;
  CompressionLevel compressionLevel_;
};
#endif

#ifndef MCAP_COMPRESSION_NO_ZSTD
/**
 * @brief An in-memory IChunkWriter implementation that holds data in a
 * temporary buffer before flushing to an ZStandard-compressed buffer.
 */
class MCAP_PUBLIC ZStdWriter final : public IChunkWriter {
public:
  ZStdWriter(CompressionLevel compressionLevel, uint64_t chunkSize);
  ~ZStdWriter() override;

  void handleWrite(const std::byte* data, uint64_t size) override;
  void end() override;
  uint64_t size() const override;
  uint64_t compressedSize() const override;
  bool empty() const override;
  void handleClear() override;
  const std::byte* data() const override;
  const std::byte* compressedData() const override;

private:
  std::vector<std::byte> uncompressedBuffer_;
  std::vector<std::byte> compressedBuffer_;
  ZSTD_CCtx_s* zstdContext_ = nullptr;
};
#endif

/**
 * @brief Provides a write interface to an MCAP file.
 */
class MCAP_PUBLIC McapWriter final {
public:
  ~McapWriter();

  /**
   * @brief Open a new MCAP file for writing and write the header.
   *
   * If the writer was already opened, this calls `close`() first to reset the state.
   * A writer may be re-used after being reset via `close`() or `terminate`().
   *
   * @param filename Filename of the MCAP file to write.
   * @param options Options for MCAP writing. `profile` is required.
   * @return A non-success status if the file could not be opened for writing.
   */
  Status open(std::string_view filename, const McapWriterOptions& options);

  /**
   * @brief Open a new MCAP file for writing and write the header.
   *
   * If the writer was already opened, this calls `close`() first to reset the state.
   * A writer may be re-used after being reset via `close`() or `terminate`().
   *
   * @param writer An implementation of the IWritable interface. Output bytes
   *   will be written to this object.
   * @param options Options for MCAP writing. `profile` is required.
   */
  void open(IWritable& writer, const McapWriterOptions& options);

  /**
   * @brief Open a new MCAP file for writing and write the header.
   *
   * @param stream Output stream to write to.
   * @param options Options for MCAP writing. `profile` is required.
   */
  void open(std::ostream& stream, const McapWriterOptions& options);

  /**
   * @brief Write the MCAP footer, flush pending writes to the output stream,
   * and reset internal state. The writer may be re-used with another call to open afterwards.
   */
  void close();

  /**
   * @brief Reset internal state without writing the MCAP footer or flushing
   * pending writes. This should only be used in error cases as the output MCAP
   * file will be truncated. The writer may be re-used with another call to open afterwards.
   */
  void terminate();

  /**
   * @brief Add a new schema to the MCAP file and set `schema.id` to a generated
   * schema id. The schema id is used when adding channels to the file.
   *
   * Schemas are not cleared when the state is reset via `close`() or `terminate`().
   * If you're re-using a writer for multiple files in a row, the schemas only need
   * to be added once, before first use.
   *
   * This method does not de-duplicate schemas.
   *
   * @param schema Description of the schema to register. The `id` field is
   *   ignored and will be set to a generated schema id.
   */
  void addSchema(Schema& schema);

  /**
   * @brief Add a new channel to the MCAP file and set `channel.id` to a
   * generated channel id. The channel id is used when adding messages to the
   * file.
   *
   * Channels are not cleared when the state is reset via `close`() or `terminate`().
   * If you're re-using a writer for multiple files in a row, the channels only need
   * to be added once, before first use.
   *
   * This method does not de-duplicate channels.
   *
   * @param channel Description of the channel to register. The `id` value is
   *   ignored and will be set to a generated channel id.
   */
  void addChannel(Channel& channel);

  /**
   * @brief Write a message to the output stream.
   *
   * @param msg Message to add.
   * @return A non-zero error code on failure.
   */
  Status write(const Message& message);

  /**
   * @brief Write an attachment to the output stream.
   *
   * @param attachment Attachment to add. The `attachment.crc` will be
   * calculated and set if configuration options allow CRC calculation.
   * @return A non-zero error code on failure.
   */
  Status write(Attachment& attachment);

  /**
   * @brief Write a metadata record to the output stream.
   *
   * @param metadata Named group of key/value string pairs to add.
   * @return A non-zero error code on failure.
   */
  Status write(const Metadata& metadata);

  /**
   * @brief Current MCAP file-level statistics. This is written as a Statistics
   * record in the Summary section of the MCAP file.
   */
  const Statistics& statistics() const;

  /**
   * @brief Returns a pointer to the IWritable data destination backing this
   * writer. Will return nullptr if the writer is not open.
   */
  IWritable* dataSink();

  /**
   * @brief finishes the current chunk in progress and writes it to the file, if a chunk
   * is in progress.
   */
  void closeLastChunk();

  // The following static methods are used for serialization of records and
  // primitives to an output stream. They are not intended to be used directly
  // unless you are implementing a lower level writer or tests

  static void writeMagic(IWritable& output);

  static uint64_t write(IWritable& output, const Header& header);
  static uint64_t write(IWritable& output, const Footer& footer, bool crcEnabled);
  static uint64_t write(IWritable& output, const Schema& schema);
  static uint64_t write(IWritable& output, const Channel& channel);
  static uint64_t getRecordSize(const Message& message);
  static uint64_t write(IWritable& output, const Message& message);
  static uint64_t write(IWritable& output, const Attachment& attachment);
  static uint64_t write(IWritable& output, const Metadata& metadata);
  static uint64_t write(IWritable& output, const Chunk& chunk);
  static uint64_t write(IWritable& output, const MessageIndex& index);
  static uint64_t write(IWritable& output, const ChunkIndex& index);
  static uint64_t write(IWritable& output, const AttachmentIndex& index);
  static uint64_t write(IWritable& output, const MetadataIndex& index);
  static uint64_t write(IWritable& output, const Statistics& stats);
  static uint64_t write(IWritable& output, const SummaryOffset& summaryOffset);
  static uint64_t write(IWritable& output, const DataEnd& dataEnd);
  static uint64_t write(IWritable& output, const Record& record);

  static void write(IWritable& output, const std::string_view str);
  static void write(IWritable& output, const ByteArray bytes);
  static void write(IWritable& output, OpCode value);
  static void write(IWritable& output, uint16_t value);
  static void write(IWritable& output, uint32_t value);
  static void write(IWritable& output, uint64_t value);
  static void write(IWritable& output, const std::byte* data, uint64_t size);
  static void write(IWritable& output, const KeyValueMap& map, uint32_t size = 0);

private:
  McapWriterOptions options_{""};
  uint64_t chunkSize_ = DefaultChunkSize;
  IWritable* output_ = nullptr;
  std::unique_ptr<FileWriter> fileOutput_;
  std::unique_ptr<StreamWriter> streamOutput_;
  std::unique_ptr<BufferWriter> uncompressedChunk_;
#ifndef MCAP_COMPRESSION_NO_LZ4
  std::unique_ptr<LZ4Writer> lz4Chunk_;
#endif
#ifndef MCAP_COMPRESSION_NO_ZSTD
  std::unique_ptr<ZStdWriter> zstdChunk_;
#endif
  std::vector<Schema> schemas_;
  std::vector<Channel> channels_;
  std::vector<AttachmentIndex> attachmentIndex_;
  std::vector<MetadataIndex> metadataIndex_;
  std::vector<ChunkIndex> chunkIndex_;
  Statistics statistics_{};
  std::unordered_set<SchemaId> writtenSchemas_;
  std::unordered_map<ChannelId, MessageIndex> currentMessageIndex_;
  Timestamp currentChunkStart_ = MaxTime;
  Timestamp currentChunkEnd_ = 0;
  Compression compression_ = Compression::None;
  uint64_t uncompressedSize_ = 0;
  bool opened_ = false;

  IWritable& getOutput();
  IChunkWriter* getChunkWriter();
  void writeChunk(IWritable& output, IChunkWriter& chunkData);
};

}  // namespace mcap

#ifdef MCAP_IMPLEMENTATION
#  include "writer.inl"
#endif
