#include "internal.hpp"
#include <algorithm>
#include <cassert>
#ifndef MCAP_COMPRESSION_NO_LZ4
#  include <lz4frame.h>
#endif
#ifndef MCAP_COMPRESSION_NO_ZSTD
#  include <zstd.h>
#  include <zstd_errors.h>
#endif

namespace mcap {

bool CompareChunkIndexes(const ChunkIndex& a, const ChunkIndex& b) {
  return a.chunkStartOffset < b.chunkStartOffset;
}

// BufferReader ////////////////////////////////////////////////////////////////

void BufferReader::reset(const std::byte* data, uint64_t size, uint64_t uncompressedSize) {
  (void)uncompressedSize;
  assert(size == uncompressedSize);
  data_ = data;
  size_ = size;
}

uint64_t BufferReader::read(std::byte** output, uint64_t offset, uint64_t size) {
  if (!data_ || offset >= size_) {
    return 0;
  }

  const auto available = size_ - offset;
  *output = const_cast<std::byte*>(data_) + offset;
  return std::min(size, available);
}

uint64_t BufferReader::size() const {
  return size_;
}

Status BufferReader::status() const {
  return StatusCode::Success;
}

// FileReader //////////////////////////////////////////////////////////////////

FileReader::FileReader(std::FILE* file)
    : file_(file)
    , size_(0)
    , position_(0) {
  assert(file_);

  // Determine the size of the file
  std::fseek(file_, 0, SEEK_END);
  size_ = std::ftell(file_);
  std::fseek(file_, 0, SEEK_SET);
}

uint64_t FileReader::size() const {
  return size_;
}

uint64_t FileReader::read(std::byte** output, uint64_t offset, uint64_t size) {
  if (offset >= size_) {
    return 0;
  }

  if (offset != position_) {
    std::fseek(file_, (long)(offset), SEEK_SET);
    std::fflush(file_);
    position_ = offset;
  }

  if (size > buffer_.size()) {
    buffer_.resize(size);
  }

  const uint64_t bytesRead = uint64_t(std::fread(buffer_.data(), 1, size, file_));
  *output = buffer_.data();

  position_ += bytesRead;
  return bytesRead;
}

// FileStreamReader ////////////////////////////////////////////////////////////

FileStreamReader::FileStreamReader(std::ifstream& stream)
    : stream_(stream)
    , position_(0) {
  assert(stream.is_open());

  // Determine the size of the file
  stream_.seekg(0, stream.end);
  size_ = stream_.tellg();
  stream_.seekg(0, stream.beg);
}

uint64_t FileStreamReader::size() const {
  return size_;
}

uint64_t FileStreamReader::read(std::byte** output, uint64_t offset, uint64_t size) {
  if (offset >= size_) {
    return 0;
  }

  if (offset != position_) {
    stream_.seekg(offset);
    position_ = offset;
  }

  if (size > buffer_.size()) {
    buffer_.resize(size);
  }

  stream_.read(reinterpret_cast<char*>(buffer_.data()), size);
  *output = buffer_.data();

  const uint64_t bytesRead = stream_.gcount();
  position_ += bytesRead;
  return bytesRead;
}

// LZ4Reader ///////////////////////////////////////////////////////////////////

#ifndef MCAP_COMPRESSION_NO_LZ4
LZ4Reader::LZ4Reader() {
  const LZ4F_errorCode_t err =
    LZ4F_createDecompressionContext((LZ4F_dctx**)&decompressionContext_, LZ4F_VERSION);
  if (LZ4F_isError(err)) {
    const auto msg =
      internal::StrCat("failed to create lz4 decompression context: ", LZ4F_getErrorName(err));
    status_ = Status{StatusCode::DecompressionFailed, msg};
    decompressionContext_ = nullptr;
  }
}

LZ4Reader::~LZ4Reader() {
  if (decompressionContext_) {
    LZ4F_freeDecompressionContext((LZ4F_dctx*)decompressionContext_);
  }
}

void LZ4Reader::reset(const std::byte* data, uint64_t size, uint64_t uncompressedSize) {
  if (!decompressionContext_) {
    return;
  }
  compressedData_ = data;
  compressedSize_ = size;
  status_ = decompressAll(data, size, uncompressedSize, &uncompressedData_);
  uncompressedSize_ = uncompressedData_.size();
}

uint64_t LZ4Reader::read(std::byte** output, uint64_t offset, uint64_t size) {
  if (offset >= uncompressedSize_) {
    return 0;
  }

  const auto available = uncompressedSize_ - offset;
  *output = uncompressedData_.data() + offset;
  return std::min(size, available);
}

uint64_t LZ4Reader::size() const {
  return uncompressedSize_;
}

Status LZ4Reader::status() const {
  return status_;
}
Status LZ4Reader::decompressAll(const std::byte* data, uint64_t compressedSize,
                                uint64_t uncompressedSize, ByteArray* output) {
  if (!decompressionContext_) {
    return status_;
  }
  auto result = Status();
  // Allocate space for the uncompressed data
  output->resize(uncompressedSize);

  size_t dstSize = uncompressedSize;
  size_t srcSize = compressedSize;
  LZ4F_resetDecompressionContext((LZ4F_dctx*)decompressionContext_);
  const auto status = LZ4F_decompress((LZ4F_dctx*)decompressionContext_, output->data(), &dstSize,
                                      data, &srcSize, nullptr);
  if (status != 0) {
    if (LZ4F_isError(status)) {
      const auto msg = internal::StrCat("lz4 decompression of ", compressedSize, " bytes into ",
                                        uncompressedSize, " output bytes failed with error ",
                                        (int)status, " (", LZ4F_getErrorName(status), ")");
      result = Status{StatusCode::DecompressionFailed, msg};
    } else {
      const auto msg =
        internal::StrCat("lz4 decompression of ", compressedSize, " bytes into ", uncompressedSize,
                         " incomplete: consumed ", srcSize, " and produced ", dstSize,
                         " bytes so far, expect ", status, " more input bytes");
      result = Status{StatusCode::DecompressionSizeMismatch, msg};
    }
    output->clear();
  } else if (srcSize != compressedSize) {
    const auto msg =
      internal::StrCat("lz4 decompression of ", compressedSize, " bytes into ", uncompressedSize,
                       " output bytes only consumed ", srcSize, " bytes");
    result = Status{StatusCode::DecompressionSizeMismatch, msg};
    output->clear();
  } else if (dstSize != uncompressedSize) {
    const auto msg =
      internal::StrCat("lz4 decompression of ", compressedSize, " bytes into ", uncompressedSize,
                       " output bytes only produced ", dstSize, " bytes");
    result = Status{StatusCode::DecompressionSizeMismatch, msg};
    output->clear();
  }
  return result;
}
#endif

// ZStdReader //////////////////////////////////////////////////////////////////

#ifndef MCAP_COMPRESSION_NO_ZSTD
void ZStdReader::reset(const std::byte* data, uint64_t size, uint64_t uncompressedSize) {
  status_ = DecompressAll(data, size, uncompressedSize, &uncompressedData_);
}

uint64_t ZStdReader::read(std::byte** output, uint64_t offset, uint64_t size) {
  if (offset >= uncompressedData_.size()) {
    return 0;
  }

  const auto available = uncompressedData_.size() - offset;
  *output = uncompressedData_.data() + offset;
  return std::min(size, available);
}

uint64_t ZStdReader::size() const {
  return uncompressedData_.size();
}

Status ZStdReader::status() const {
  return status_;
}

Status ZStdReader::DecompressAll(const std::byte* data, uint64_t compressedSize,
                                 uint64_t uncompressedSize, ByteArray* output) {
  auto result = Status();

  // Allocate space for the decompressed data
  output->resize(uncompressedSize);

  const auto status = ZSTD_decompress(output->data(), uncompressedSize, data, compressedSize);
  if (status != uncompressedSize) {
    if (ZSTD_isError(status)) {
      const auto msg =
        internal::StrCat("zstd decompression of ", compressedSize, " bytes into ", uncompressedSize,
                         " output bytes failed with error ", ZSTD_getErrorName(status));
      result = Status{StatusCode::DecompressionFailed, msg};
    } else {
      const auto msg =
        internal::StrCat("zstd decompression of ", compressedSize, " bytes into ", uncompressedSize,
                         " output bytes only produced ", status, " bytes");
      result = Status{StatusCode::DecompressionSizeMismatch, msg};
    }
    output->clear();
  }
  return result;
}
#endif

// McapReader //////////////////////////////////////////////////////////////////

McapReader::~McapReader() {
  close();
}

Status McapReader::open(IReadable& reader) {
  reset_();

  const uint64_t fileSize = reader.size();

  if (fileSize < internal::MinHeaderLength + internal::FooterLength) {
    return StatusCode::FileTooSmall;
  }

  std::byte* data = nullptr;
  uint64_t bytesRead;

  // Read the magic bytes and header up to the first variable length string
  bytesRead = reader.read(&data, 0, sizeof(Magic) + 1 + 8 + 4);
  if (bytesRead != sizeof(Magic) + 1 + 8 + 4) {
    return StatusCode::ReadFailed;
  }

  // Check the header magic bytes
  if (std::memcmp(data, Magic, sizeof(Magic)) != 0) {
    const auto msg =
      internal::StrCat("invalid magic bytes in Header: 0x", internal::MagicToHex(data));
    return Status{StatusCode::MagicMismatch, msg};
  }

  // Read the Header record
  Record record;
  if (auto status = ReadRecord(reader, sizeof(Magic), &record); !status.ok()) {
    return status;
  }
  if (record.opcode != OpCode::Header) {
    const auto msg = internal::StrCat("invalid opcode, expected Header: 0x",
                                      internal::ToHex(uint8_t(record.opcode)));
    return Status{StatusCode::InvalidFile, msg};
  }
  Header header;
  if (auto status = ParseHeader(record, &header); !status.ok()) {
    return status;
  }
  header_ = header;

  // The Data section starts after the magic bytes and Header record
  dataStart_ = sizeof(Magic) + record.recordSize();
  // Set dataEnd_ to just before the Footer for now. This will be updated when
  // the Data End record is encountered and/or the summary section is parsed
  dataEnd_ = fileSize - internal::FooterLength;

  input_ = &reader;

  return StatusCode::Success;
}

Status McapReader::open(std::string_view filename) {
  if (file_) {
    std::fclose(file_);
    file_ = nullptr;
  }
  file_ = std::fopen(filename.data(), "rb");
  if (!file_) {
    const auto msg = internal::StrCat("failed to open \"", filename, "\"");
    return Status{StatusCode::OpenFailed, msg};
  }

  fileInput_ = std::make_unique<FileReader>(file_);
  return open(*fileInput_);
}

Status McapReader::open(std::ifstream& stream) {
  fileStreamInput_ = std::make_unique<FileStreamReader>(stream);
  return open(*fileStreamInput_);
}

void McapReader::close() {
  input_ = nullptr;
  if (file_) {
    std::fclose(file_);
    file_ = nullptr;
  }
  fileInput_.reset();
  fileStreamInput_.reset();
  reset_();
}

void McapReader::reset_() {
  header_ = std::nullopt;
  footer_ = std::nullopt;
  statistics_ = std::nullopt;
  chunkIndexes_.clear();
  attachmentIndexes_.clear();
  schemas_.clear();
  channels_.clear();
  dataStart_ = 0;
  dataEnd_ = EndOffset;
  parsedSummary_ = false;
}

Status McapReader::readSummary(ReadSummaryMethod method, const ProblemCallback& onProblem) {
  if (!input_) {
    const Status status{StatusCode::NotOpen};
    onProblem(status);
    return status;
  }

  auto& reader = *input_;
  bool parsed = false;

  if (method != ReadSummaryMethod::ForceScan) {
    // Build indexes and read stats from the Summary section
    const auto status = readSummarySection_(reader);
    if (status.ok()) {
      // Summary section parsing was successful
      parsed = true;
    } else if (method == ReadSummaryMethod::NoFallbackScan) {
      // No fallback allowed, fail immediately
      onProblem(status);
      return status;
    }
  }

  if (!parsed) {
    const auto status = readSummaryFromScan_(reader);
    if (!status.ok()) {
      // Scanning failed, fail immediately
      onProblem(status);
      return status;
    }
  }

  // Convert the list of chunk indexes to an interval tree indexed by message start/end times
  std::vector<ChunkInterval> chunkIntervals;
  chunkIntervals.reserve(chunkIndexes_.size());
  for (const auto& chunkIndex : chunkIndexes_) {
    chunkIntervals.emplace_back(chunkIndex.messageStartTime, chunkIndex.messageEndTime, chunkIndex);
  }
  chunkRanges_ = internal::IntervalTree<ByteOffset, ChunkIndex>{std::move(chunkIntervals)};

  parsedSummary_ = true;
  return StatusCode::Success;
}

Status McapReader::readSummarySection_(IReadable& reader) {
  const uint64_t fileSize = reader.size();

  // Read the footer
  auto footer = Footer{};
  if (auto status = ReadFooter(reader, fileSize - internal::FooterLength, &footer); !status.ok()) {
    return status;
  }
  footer_ = footer;

  // Get summaryStart and summaryOffsetStart, allowing for zeroed values
  const ByteOffset summaryStart =
    footer.summaryStart != 0 ? footer.summaryStart : fileSize - internal::FooterLength;
  const ByteOffset summaryOffsetStart =
    footer.summaryOffsetStart != 0 ? footer.summaryOffsetStart : fileSize - internal::FooterLength;
  // Sanity check the ordering
  if (summaryOffsetStart < summaryStart) {
    const auto msg = internal::StrCat("summary_offset_start ", summaryOffsetStart,
                                      " < summary_start ", summaryStart);
    return Status{StatusCode::InvalidFooter, msg};
  }

  attachmentIndexes_.clear();
  metadataIndexes_.clear();
  chunkIndexes_.clear();

  // Read the Summary section
  bool readStatistics = false;
  TypedRecordReader typedReader{reader, summaryStart, summaryOffsetStart};
  typedReader.onSchema = [&](SchemaPtr schemaPtr, ByteOffset, std::optional<ByteOffset>) {
    schemas_.try_emplace(schemaPtr->id, schemaPtr);
  };
  typedReader.onChannel = [&](ChannelPtr channelPtr, ByteOffset, std::optional<ByteOffset>) {
    channels_.try_emplace(channelPtr->id, channelPtr);
  };
  typedReader.onAttachmentIndex = [&](const AttachmentIndex& attachmentIndex, ByteOffset) {
    attachmentIndexes_.emplace(attachmentIndex.name, attachmentIndex);
  };
  typedReader.onMetadataIndex = [&](const MetadataIndex& metadataIndex, ByteOffset) {
    metadataIndexes_.emplace(metadataIndex.name, metadataIndex);
  };
  typedReader.onChunkIndex = [&](const ChunkIndex chunkIndex, ByteOffset) {
    // Check if this chunk index is a duplicate
    if (std::binary_search(chunkIndexes_.begin(), chunkIndexes_.end(), chunkIndex,
                           CompareChunkIndexes)) {
      return;
    }

    // Check if this chunk index is out of order
    const bool needsSorting =
      !chunkIndexes_.empty() && chunkIndexes_.back().chunkStartOffset > chunkIndex.chunkStartOffset;
    // Add the new chunk index interval
    chunkIndexes_.push_back(chunkIndex);
    // Sort if the new chunk index is out of order
    if (needsSorting) {
      std::sort(chunkIndexes_.begin(), chunkIndexes_.end(), CompareChunkIndexes);
    }
  };
  typedReader.onStatistics = [&](const Statistics& statistics, ByteOffset) {
    statistics_ = statistics;
    readStatistics = true;
  };

  while (typedReader.next()) {
    const auto& status = typedReader.status();
    if (!status.ok()) {
      return status;
    }
  }

  dataEnd_ = summaryStart;
  return readStatistics ? StatusCode::Success : StatusCode::MissingStatistics;
}

Status McapReader::readSummaryFromScan_(IReadable& reader) {
  bool done = false;
  Statistics statistics{};
  statistics.messageStartTime = EndOffset;

  schemas_.clear();
  channels_.clear();
  attachmentIndexes_.clear();
  metadataIndexes_.clear();
  chunkIndexes_.clear();

  TypedRecordReader typedReader{reader, dataStart_, dataEnd_};
  typedReader.onSchema = [&](SchemaPtr schemaPtr, ByteOffset, std::optional<ByteOffset>) {
    schemas_.try_emplace(schemaPtr->id, schemaPtr);
  };
  typedReader.onChannel = [&](ChannelPtr channelPtr, ByteOffset, std::optional<ByteOffset>) {
    channels_.try_emplace(channelPtr->id, channelPtr);
  };
  typedReader.onAttachment = [&](const Attachment& attachment, ByteOffset fileOffset) {
    AttachmentIndex attachmentIndex{attachment, fileOffset};
    attachmentIndexes_.emplace(attachment.name, attachmentIndex);
  };
  typedReader.onMetadata = [&](const Metadata& metadata, ByteOffset fileOffset) {
    MetadataIndex metadataIndex{metadata, fileOffset};
    metadataIndexes_.emplace(metadata.name, metadataIndex);
  };
  typedReader.onChunk = [&](const Chunk& chunk, ByteOffset fileOffset) {
    ChunkIndex chunkIndex{};
    chunkIndex.messageStartTime = chunk.messageStartTime;
    chunkIndex.messageEndTime = chunk.messageEndTime;
    chunkIndex.chunkStartOffset = fileOffset;
    chunkIndex.chunkLength =
      9 + 8 + 8 + 8 + 4 + 4 + chunk.compression.size() + 8 + chunk.compressedSize;
    chunkIndex.messageIndexLength = 0;
    chunkIndex.compression = chunk.compression;
    chunkIndex.compressedSize = chunk.compressedSize;
    chunkIndex.uncompressedSize = chunk.uncompressedSize;

    chunkIndexes_.emplace_back(std::move(chunkIndex));
  };
  typedReader.onMessage = [&](const Message& message, ByteOffset, std::optional<ByteOffset>) {
    if (message.logTime < statistics.messageStartTime) {
      statistics.messageStartTime = message.logTime;
    }
    if (message.logTime > statistics.messageEndTime) {
      statistics.messageEndTime = message.logTime;
    }
    statistics.messageCount++;
    statistics.channelMessageCounts[message.channelId]++;
  };
  typedReader.onDataEnd = [&](const DataEnd&, ByteOffset fileOffset) {
    dataEnd_ = fileOffset;
    done = true;
  };

  while (!done && typedReader.next()) {
    const auto& status = typedReader.status();
    if (!status.ok()) {
      return status;
    }
  }

  if (statistics.messageStartTime == EndOffset) {
    statistics.messageStartTime = 0;
  }
  statistics.schemaCount = (uint16_t)(schemas_.size());
  statistics.channelCount = (uint32_t)(channels_.size());
  statistics.attachmentCount = (uint32_t)(attachmentIndexes_.size());
  statistics.metadataCount = (uint32_t)(metadataIndexes_.size());
  statistics.chunkCount = (uint32_t)(chunkIndexes_.size());
  statistics_ = std::move(statistics);

  return StatusCode::Success;
}

LinearMessageView McapReader::readMessages(Timestamp startTime, Timestamp endTime) {
  const auto onProblem = [](const Status&) {};
  return readMessages(onProblem, startTime, endTime);
}

LinearMessageView McapReader::readMessages(const ProblemCallback& onProblem, Timestamp startTime,
                                           Timestamp endTime) {
  ReadMessageOptions options;
  options.startTime = startTime;
  options.endTime = endTime;
  return readMessages(onProblem, options);
}

LinearMessageView McapReader::readMessages(const ProblemCallback& onProblem,
                                           const ReadMessageOptions& options) {
  // Check that open() has been successfully called
  if (!dataSource() || dataStart_ == 0) {
    onProblem(StatusCode::NotOpen);
    return LinearMessageView{*this, onProblem};
  }

  const auto [startOffset, endOffset] = byteRange(options.startTime, options.endTime);
  return LinearMessageView{*this, options, startOffset, endOffset, onProblem};
}

std::pair<ByteOffset, ByteOffset> McapReader::byteRange(Timestamp startTime,
                                                        Timestamp endTime) const {
  if (!parsedSummary_ || chunkRanges_.empty()) {
    return {dataStart_, dataEnd_};
  }

  ByteOffset dataStart = dataEnd_;
  ByteOffset dataEnd = dataStart_;
  chunkRanges_.visit_overlapping(startTime, endTime, [&](const auto& interval) {
    const auto& chunkIndex = interval.value;
    dataStart = std::min(dataStart, chunkIndex.chunkStartOffset);
    dataEnd = std::max(dataEnd, chunkIndex.chunkStartOffset + chunkIndex.chunkLength);
  });
  dataEnd = std::max(dataEnd, dataStart);

  if (dataStart == dataEnd) {
    return {0, 0};
  }
  return {dataStart, dataEnd};
}

IReadable* McapReader::dataSource() {
  return input_;
}

const std::optional<Header>& McapReader::header() const {
  return header_;
}

const std::optional<Footer>& McapReader::footer() const {
  return footer_;
}

const std::optional<Statistics>& McapReader::statistics() const {
  return statistics_;
}

const std::unordered_map<ChannelId, ChannelPtr> McapReader::channels() const {
  return channels_;
}

const std::unordered_map<SchemaId, SchemaPtr> McapReader::schemas() const {
  return schemas_;
}

ChannelPtr McapReader::channel(ChannelId channelId) const {
  const auto& maybeChannel = channels_.find(channelId);
  return (maybeChannel == channels_.end()) ? nullptr : maybeChannel->second;
}

SchemaPtr McapReader::schema(SchemaId schemaId) const {
  const auto& maybeSchema = schemas_.find(schemaId);
  return (maybeSchema == schemas_.end()) ? nullptr : maybeSchema->second;
}

const std::vector<ChunkIndex>& McapReader::chunkIndexes() const {
  return chunkIndexes_;
}

const std::multimap<std::string, MetadataIndex>& McapReader::metadataIndexes() const {
  return metadataIndexes_;
}

const std::multimap<std::string, AttachmentIndex>& McapReader::attachmentIndexes() const {
  return attachmentIndexes_;
}

Status McapReader::ReadRecord(IReadable& reader, uint64_t offset, Record* record) {
  // Check that we can read at least 9 bytes (opcode + length)
  auto maxSize = reader.size() - offset;
  if (maxSize < 9) {
    const auto msg =
      internal::StrCat("cannot read record at offset ", offset, ", ", maxSize, " bytes remaining");
    return Status{StatusCode::InvalidFile, msg};
  }

  // Read opcode and length
  std::byte* data;
  uint64_t bytesRead = reader.read(&data, offset, 9);
  if (bytesRead != 9) {
    return StatusCode::ReadFailed;
  }

  // Parse opcode and length
  record->opcode = OpCode(data[0]);
  record->dataSize = internal::ParseUint64(data + 1);

  // Read payload
  maxSize -= 9;
  if (maxSize < record->dataSize) {
    const auto msg = internal::StrCat("record type 0x", internal::ToHex(uint8_t(record->opcode)),
                                      " at offset ", offset, " has length ", record->dataSize,
                                      " but only ", maxSize, " bytes remaining");
    return Status{StatusCode::InvalidRecord, msg};
  }
  bytesRead = reader.read(&record->data, offset + 9, record->dataSize);
  if (bytesRead != record->dataSize) {
    const auto msg =
      internal::StrCat("attempted to read ", record->dataSize, " bytes for record type 0x",
                       internal::ToHex(uint8_t(record->opcode)), " at offset ", offset,
                       " but only read ", bytesRead, " bytes");
    return Status{StatusCode::ReadFailed, msg};
  }

  return StatusCode::Success;
}

Status McapReader::ReadFooter(IReadable& reader, uint64_t offset, Footer* footer) {
  std::byte* data;
  uint64_t bytesRead = reader.read(&data, offset, internal::FooterLength);
  if (bytesRead != internal::FooterLength) {
    return StatusCode::ReadFailed;
  }

  // Check the footer magic bytes
  if (std::memcmp(data + internal::FooterLength - sizeof(Magic), Magic, sizeof(Magic)) != 0) {
    const auto msg =
      internal::StrCat("invalid magic bytes in Footer: 0x",
                       internal::MagicToHex(data + internal::FooterLength - sizeof(Magic)));
    return Status{StatusCode::MagicMismatch, msg};
  }

  if (OpCode(data[0]) != OpCode::Footer) {
    const auto msg =
      internal::StrCat("invalid opcode, expected Footer: 0x", internal::ToHex(data[0]));
    return Status{StatusCode::InvalidFile, msg};
  }

  // Sanity check the record length. This is just an additional safeguard, since the footer has a
  // fixed length
  const uint64_t length = internal::ParseUint64(data + 1);
  if (length != 8 + 8 + 4) {
    const auto msg = internal::StrCat("invalid Footer length: ", length);
    return Status{StatusCode::InvalidRecord, msg};
  }

  footer->summaryStart = internal::ParseUint64(data + 1 + 8);
  footer->summaryOffsetStart = internal::ParseUint64(data + 1 + 8 + 8);
  footer->summaryCrc = internal::ParseUint32(data + 1 + 8 + 8 + 8);
  return StatusCode::Success;
}

Status McapReader::ParseHeader(const Record& record, Header* header) {
  constexpr uint64_t MinSize = 4 + 4;

  assert(record.opcode == OpCode::Header);
  if (record.dataSize < MinSize) {
    const auto msg = internal::StrCat("invalid Header length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  if (auto status = internal::ParseString(record.data, record.dataSize, &header->profile);
      !status.ok()) {
    return status;
  }
  const size_t libraryOffset = 4 + header->profile.size();
  const std::byte* libraryData = &(record.data[libraryOffset]);
  const size_t maxSize = record.dataSize - libraryOffset;
  auto status = internal::ParseString(libraryData, maxSize, &header->library);
  if (!status.ok()) {
    return status;
  }
  return StatusCode::Success;
}

Status McapReader::ParseFooter(const Record& record, Footer* footer) {
  constexpr uint64_t FooterSize = 8 + 8 + 4;

  assert(record.opcode == OpCode::Footer);
  if (record.dataSize != FooterSize) {
    const auto msg = internal::StrCat("invalid Footer length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  footer->summaryStart = internal::ParseUint64(record.data);
  footer->summaryOffsetStart = internal::ParseUint64(record.data + 8);
  footer->summaryCrc = internal::ParseUint32(record.data + 8 + 8);

  return StatusCode::Success;
}

Status McapReader::ParseSchema(const Record& record, Schema* schema) {
  constexpr uint64_t MinSize = 2 + 4 + 4 + 4;

  assert(record.opcode == OpCode::Schema);
  if (record.dataSize < MinSize) {
    const auto msg = internal::StrCat("invalid Schema length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  size_t offset = 0;

  // id
  schema->id = internal::ParseUint16(record.data);
  offset += 2;
  // name
  if (auto status =
        internal::ParseString(record.data + offset, record.dataSize - offset, &schema->name);
      !status.ok()) {
    return status;
  }
  offset += 4 + schema->name.size();
  // encoding
  if (auto status =
        internal::ParseString(record.data + offset, record.dataSize - offset, &schema->encoding);
      !status.ok()) {
    return status;
  }
  offset += 4 + schema->encoding.size();
  // data
  if (auto status =
        internal::ParseByteArray(record.data + offset, record.dataSize - offset, &schema->data);
      !status.ok()) {
    return status;
  }

  return StatusCode::Success;
}

Status McapReader::ParseChannel(const Record& record, Channel* channel) {
  constexpr uint64_t MinSize = 2 + 4 + 4 + 2 + 4;

  assert(record.opcode == OpCode::Channel);
  if (record.dataSize < MinSize) {
    const auto msg = internal::StrCat("invalid Channel length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  size_t offset = 0;

  // id
  channel->id = internal::ParseUint16(record.data);
  offset += 2;
  // schema_id
  channel->schemaId = internal::ParseUint16(record.data + offset);
  offset += 2;
  // topic
  if (auto status =
        internal::ParseString(record.data + offset, record.dataSize - offset, &channel->topic);
      !status.ok()) {
    return status;
  }
  offset += 4 + channel->topic.size();
  // message_encoding
  if (auto status = internal::ParseString(record.data + offset, record.dataSize - offset,
                                          &channel->messageEncoding);
      !status.ok()) {
    return status;
  }
  offset += 4 + channel->messageEncoding.size();
  // metadata
  if (auto status = internal::ParseKeyValueMap(record.data + offset, record.dataSize - offset,
                                               &channel->metadata);
      !status.ok()) {
    return status;
  }
  return StatusCode::Success;
}

Status McapReader::ParseMessage(const Record& record, Message* message) {
  constexpr uint64_t MessagePreambleSize = 2 + 4 + 8 + 8;

  assert(record.opcode == OpCode::Message);
  if (record.dataSize < MessagePreambleSize) {
    const auto msg = internal::StrCat("invalid Message length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  message->channelId = internal::ParseUint16(record.data);
  message->sequence = internal::ParseUint32(record.data + 2);
  message->logTime = internal::ParseUint64(record.data + 2 + 4);
  message->publishTime = internal::ParseUint64(record.data + 2 + 4 + 8);
  message->dataSize = record.dataSize - MessagePreambleSize;
  message->data = record.data + MessagePreambleSize;
  return StatusCode::Success;
}

Status McapReader::ParseChunk(const Record& record, Chunk* chunk) {
  constexpr uint64_t ChunkPreambleSize = 8 + 8 + 8 + 4 + 4;

  assert(record.opcode == OpCode::Chunk);
  if (record.dataSize < ChunkPreambleSize) {
    const auto msg = internal::StrCat("invalid Chunk length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  chunk->messageStartTime = internal::ParseUint64(record.data);
  chunk->messageEndTime = internal::ParseUint64(record.data + 8);
  chunk->uncompressedSize = internal::ParseUint64(record.data + 8 + 8);
  chunk->uncompressedCrc = internal::ParseUint32(record.data + 8 + 8 + 8);

  size_t offset = 8 + 8 + 8 + 4;

  // compression
  if (auto status =
        internal::ParseString(record.data + offset, record.dataSize - offset, &chunk->compression);
      !status.ok()) {
    return status;
  }
  offset += 4 + chunk->compression.size();
  // compressed_size
  if (auto status = internal::ParseUint64(record.data + offset, record.dataSize - offset,
                                          &chunk->compressedSize);
      !status.ok()) {
    return status;
  }
  offset += 8;
  if (chunk->compressedSize > record.dataSize - offset) {
    const auto msg = internal::StrCat("invalid Chunk.records length: ", chunk->compressedSize);
    return Status{StatusCode::InvalidRecord, msg};
  }
  // records
  chunk->records = record.data + offset;

  return StatusCode::Success;
}

Status McapReader::ParseMessageIndex(const Record& record, MessageIndex* messageIndex) {
  constexpr uint64_t PreambleSize = 2 + 4;

  assert(record.opcode == OpCode::MessageIndex);
  if (record.dataSize < PreambleSize) {
    const auto msg = internal::StrCat("invalid MessageIndex length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  messageIndex->channelId = internal::ParseUint16(record.data);
  const uint32_t recordsSize = internal::ParseUint32(record.data + 2);

  if (recordsSize % 16 != 0 || recordsSize > record.dataSize - PreambleSize) {
    const auto msg = internal::StrCat("invalid MessageIndex.records length: ", recordsSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  const size_t recordsCount = size_t(recordsSize / 16);
  messageIndex->records.reserve(recordsCount);
  for (size_t i = 0; i < recordsCount; ++i) {
    const auto timestamp = internal::ParseUint64(record.data + PreambleSize + i * 16);
    const auto offset = internal::ParseUint64(record.data + PreambleSize + i * 16 + 8);
    messageIndex->records.emplace_back(timestamp, offset);
  }
  return StatusCode::Success;
}

Status McapReader::ParseChunkIndex(const Record& record, ChunkIndex* chunkIndex) {
  constexpr uint64_t PreambleSize = 8 + 8 + 8 + 8 + 4;

  assert(record.opcode == OpCode::ChunkIndex);
  if (record.dataSize < PreambleSize) {
    const auto msg = internal::StrCat("invalid ChunkIndex length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  chunkIndex->messageStartTime = internal::ParseUint64(record.data);
  chunkIndex->messageEndTime = internal::ParseUint64(record.data + 8);
  chunkIndex->chunkStartOffset = internal::ParseUint64(record.data + 8 + 8);
  chunkIndex->chunkLength = internal::ParseUint64(record.data + 8 + 8 + 8);
  const uint32_t messageIndexOffsetsSize = internal::ParseUint32(record.data + 8 + 8 + 8 + 8);

  if (messageIndexOffsetsSize % 10 != 0 ||
      messageIndexOffsetsSize > record.dataSize - PreambleSize) {
    const auto msg =
      internal::StrCat("invalid ChunkIndex.message_index_offsets length:", messageIndexOffsetsSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  const size_t messageIndexOffsetsCount = size_t(messageIndexOffsetsSize / 10);
  chunkIndex->messageIndexOffsets.reserve(messageIndexOffsetsCount);
  for (size_t i = 0; i < messageIndexOffsetsCount; ++i) {
    const auto channelId = internal::ParseUint16(record.data + PreambleSize + i * 10);
    const auto offset = internal::ParseUint64(record.data + PreambleSize + i * 10 + 2);
    chunkIndex->messageIndexOffsets.emplace(channelId, offset);
  }

  uint64_t offset = PreambleSize + messageIndexOffsetsSize;
  // message_index_length
  if (auto status = internal::ParseUint64(record.data + offset, record.dataSize - offset,
                                          &chunkIndex->messageIndexLength);
      !status.ok()) {
    return status;
  }
  offset += 8;
  // compression
  if (auto status = internal::ParseString(record.data + offset, record.dataSize - offset,
                                          &chunkIndex->compression);
      !status.ok()) {
    return status;
  }
  offset += 4 + chunkIndex->compression.size();
  // compressed_size
  if (auto status = internal::ParseUint64(record.data + offset, record.dataSize - offset,
                                          &chunkIndex->compressedSize);
      !status.ok()) {
    return status;
  }
  offset += 8;
  // uncompressed_size
  if (auto status = internal::ParseUint64(record.data + offset, record.dataSize - offset,
                                          &chunkIndex->uncompressedSize);
      !status.ok()) {
    return status;
  }

  return StatusCode::Success;
}

Status McapReader::ParseAttachment(const Record& record, Attachment* attachment) {
  constexpr uint64_t MinSize = /* log_time */ 8 +
                               /* create_time */ 8 +
                               /* name */ 4 +
                               /* media_type */ 4 +
                               /* data_size */ 8 +
                               /* crc */ 4;

  assert(record.opcode == OpCode::Attachment);
  if (record.dataSize < MinSize) {
    const auto msg = internal::StrCat("invalid Attachment length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  uint32_t offset = 0;
  // log_time
  if (auto status =
        internal::ParseUint64(record.data + offset, record.dataSize - offset, &attachment->logTime);
      !status.ok()) {
    return status;
  }
  offset += 8;
  // create_time
  if (auto status = internal::ParseUint64(record.data + offset, record.dataSize - offset,
                                          &attachment->createTime);
      !status.ok()) {
    return status;
  }
  offset += 8;
  // name
  if (auto status =
        internal::ParseString(record.data + offset, record.dataSize - offset, &attachment->name);
      !status.ok()) {
    return status;
  }
  offset += 4 + (uint32_t)(attachment->name.size());
  // media_type
  if (auto status = internal::ParseString(record.data + offset, record.dataSize - offset,
                                          &attachment->mediaType);
      !status.ok()) {
    return status;
  }
  offset += 4 + (uint32_t)(attachment->mediaType.size());
  // data_size
  if (auto status = internal::ParseUint64(record.data + offset, record.dataSize - offset,
                                          &attachment->dataSize);
      !status.ok()) {
    return status;
  }
  offset += 8;
  // data
  if (attachment->dataSize > record.dataSize - offset) {
    const auto msg = internal::StrCat("invalid Attachment.data length: ", attachment->dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }
  attachment->data = record.data + offset;
  offset += (uint32_t)(attachment->dataSize);
  // crc
  if (auto status =
        internal::ParseUint32(record.data + offset, record.dataSize - offset, &attachment->crc);
      !status.ok()) {
    return status;
  }

  return StatusCode::Success;
}

Status McapReader::ParseAttachmentIndex(const Record& record, AttachmentIndex* attachmentIndex) {
  constexpr uint64_t PreambleSize = 8 + 8 + 8 + 8 + 8 + 4;

  assert(record.opcode == OpCode::AttachmentIndex);
  if (record.dataSize < PreambleSize) {
    const auto msg = internal::StrCat("invalid AttachmentIndex length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  attachmentIndex->offset = internal::ParseUint64(record.data);
  attachmentIndex->length = internal::ParseUint64(record.data + 8);
  attachmentIndex->logTime = internal::ParseUint64(record.data + 8 + 8);
  attachmentIndex->createTime = internal::ParseUint64(record.data + 8 + 8 + 8);
  attachmentIndex->dataSize = internal::ParseUint64(record.data + 8 + 8 + 8 + 8);

  uint32_t offset = 8 + 8 + 8 + 8 + 8;

  // name
  if (auto status = internal::ParseString(record.data + offset, record.dataSize - offset,
                                          &attachmentIndex->name);
      !status.ok()) {
    return status;
  }
  offset += 4 + (uint32_t)(attachmentIndex->name.size());
  // media_type
  if (auto status = internal::ParseString(record.data + offset, record.dataSize - offset,
                                          &attachmentIndex->mediaType);
      !status.ok()) {
    return status;
  }

  return StatusCode::Success;
}

Status McapReader::ParseStatistics(const Record& record, Statistics* statistics) {
  constexpr uint64_t PreambleSize = 8 + 2 + 4 + 4 + 4 + 4 + 8 + 8 + 4;

  assert(record.opcode == OpCode::Statistics);
  if (record.dataSize < PreambleSize) {
    const auto msg = internal::StrCat("invalid Statistics length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  statistics->messageCount = internal::ParseUint64(record.data);
  statistics->schemaCount = internal::ParseUint16(record.data + 8);
  statistics->channelCount = internal::ParseUint32(record.data + 8 + 2);
  statistics->attachmentCount = internal::ParseUint32(record.data + 8 + 2 + 4);
  statistics->metadataCount = internal::ParseUint32(record.data + 8 + 2 + 4 + 4);
  statistics->chunkCount = internal::ParseUint32(record.data + 8 + 2 + 4 + 4 + 4);
  statistics->messageStartTime = internal::ParseUint64(record.data + 8 + 2 + 4 + 4 + 4 + 4);
  statistics->messageEndTime = internal::ParseUint64(record.data + 8 + 2 + 4 + 4 + 4 + 4 + 8);

  const uint32_t channelMessageCountsSize =
    internal::ParseUint32(record.data + 8 + 2 + 4 + 4 + 4 + 4 + 8 + 8);
  if (channelMessageCountsSize % 10 != 0 ||
      channelMessageCountsSize > record.dataSize - PreambleSize) {
    const auto msg =
      internal::StrCat("invalid Statistics.channelMessageCounts length:", channelMessageCountsSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  const size_t channelMessageCountsCount = size_t(channelMessageCountsSize / 10);
  statistics->channelMessageCounts.reserve(channelMessageCountsCount);
  for (size_t i = 0; i < channelMessageCountsCount; ++i) {
    const auto channelId = internal::ParseUint16(record.data + PreambleSize + i * 10);
    const auto messageCount = internal::ParseUint64(record.data + PreambleSize + i * 10 + 2);
    statistics->channelMessageCounts.emplace(channelId, messageCount);
  }

  return StatusCode::Success;
}

Status McapReader::ParseMetadata(const Record& record, Metadata* metadata) {
  constexpr uint64_t MinSize = 4 + 4;

  assert(record.opcode == OpCode::Metadata);
  if (record.dataSize < MinSize) {
    const auto msg = internal::StrCat("invalid Metadata length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  // name
  if (auto status = internal::ParseString(record.data, record.dataSize, &metadata->name);
      !status.ok()) {
    return status;
  }
  uint64_t offset = 4 + metadata->name.size();
  // metadata
  if (auto status = internal::ParseKeyValueMap(record.data + offset, record.dataSize - offset,
                                               &metadata->metadata);
      !status.ok()) {
    return status;
  }

  return StatusCode::Success;
}

Status McapReader::ParseMetadataIndex(const Record& record, MetadataIndex* metadataIndex) {
  constexpr uint64_t PreambleSize = 8 + 8 + 4;

  assert(record.opcode == OpCode::MetadataIndex);
  if (record.dataSize < PreambleSize) {
    const auto msg = internal::StrCat("invalid MetadataIndex length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  metadataIndex->offset = internal::ParseUint64(record.data);
  metadataIndex->length = internal::ParseUint64(record.data + 8);
  uint64_t offset = 8 + 8;
  if (auto status =
        internal::ParseString(record.data + offset, record.dataSize - offset, &metadataIndex->name);
      !status.ok()) {
    return status;
  }

  return StatusCode::Success;
}

Status McapReader::ParseSummaryOffset(const Record& record, SummaryOffset* summaryOffset) {
  constexpr uint64_t MinSize = 1 + 8 + 8;

  assert(record.opcode == OpCode::SummaryOffset);
  if (record.dataSize < MinSize) {
    const auto msg = internal::StrCat("invalid SummaryOffset length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  summaryOffset->groupOpCode = OpCode(record.data[0]);
  summaryOffset->groupStart = internal::ParseUint64(record.data + 1);
  summaryOffset->groupLength = internal::ParseUint64(record.data + 1 + 8);

  return StatusCode::Success;
}

Status McapReader::ParseDataEnd(const Record& record, DataEnd* dataEnd) {
  constexpr uint64_t MinSize = 4;

  assert(record.opcode == OpCode::DataEnd);
  if (record.dataSize < MinSize) {
    const auto msg = internal::StrCat("invalid DataEnd length: ", record.dataSize);
    return Status{StatusCode::InvalidRecord, msg};
  }

  dataEnd->dataSectionCrc = internal::ParseUint32(record.data);
  return StatusCode::Success;
}

std::optional<Compression> McapReader::ParseCompression(const std::string_view compression) {
  if (compression == "") {
    return Compression::None;
  } else if (compression == "lz4") {
    return Compression::Lz4;
  } else if (compression == "zstd") {
    return Compression::Zstd;
  } else {
    return std::nullopt;
  }
}

// RecordReader ////////////////////////////////////////////////////////////////

RecordReader::RecordReader(IReadable& dataSource, ByteOffset startOffset, ByteOffset endOffset)
    : offset(startOffset)
    , endOffset(endOffset)
    , dataSource_(&dataSource)
    , status_(StatusCode::Success)
    , curRecord_{} {}

void RecordReader::reset(IReadable& dataSource, ByteOffset startOffset, ByteOffset _endOffset) {
  dataSource_ = &dataSource;
  this->offset = startOffset;
  this->endOffset = _endOffset;
  status_ = StatusCode::Success;
  curRecord_ = {};
}

std::optional<Record> RecordReader::next() {
  if (!dataSource_ || offset >= endOffset) {
    return std::nullopt;
  }
  status_ = McapReader::ReadRecord(*dataSource_, offset, &curRecord_);
  if (!status_.ok()) {
    offset = EndOffset;
    return std::nullopt;
  }
  offset += curRecord_.recordSize();
  return curRecord_;
}

const Status& RecordReader::status() const {
  return status_;
}

ByteOffset RecordReader::curRecordOffset() const {
  return offset - curRecord_.recordSize();
}

// TypedChunkReader ////////////////////////////////////////////////////////////

TypedChunkReader::TypedChunkReader()
    : reader_{uncompressedReader_, 0, 0}
    , status_{StatusCode::Success} {}

void TypedChunkReader::reset(const Chunk& chunk, Compression compression) {
  ICompressedReader* decompressor;

  switch (compression) {
#ifndef MCAP_COMPRESSION_NO_LZ4
    case Compression::Lz4:
      decompressor = static_cast<ICompressedReader*>(&lz4Reader_);
      break;
#endif
#ifndef MCAP_COMPRESSION_NO_ZSTD
    case Compression::Zstd:
      decompressor = static_cast<ICompressedReader*>(&zstdReader_);
      break;
#endif
    case Compression::None:
      decompressor = static_cast<ICompressedReader*>(&uncompressedReader_);
      break;
    default:
      status_ = Status(StatusCode::UnsupportedCompression,
                       internal::StrCat("unsupported compression: ", chunk.compression));
      return;
  }
  decompressor->reset(chunk.records, chunk.compressedSize, chunk.uncompressedSize);
  reader_.reset(*decompressor, 0, decompressor->size());
  status_ = decompressor->status();
}

bool TypedChunkReader::next() {
  const auto maybeRecord = reader_.next();
  status_ = reader_.status();
  if (!maybeRecord.has_value()) {
    return false;
  }
  const Record& record = maybeRecord.value();
  switch (record.opcode) {
    case OpCode::Schema: {
      if (onSchema) {
        SchemaPtr schemaPtr = std::make_shared<Schema>();
        status_ = McapReader::ParseSchema(record, schemaPtr.get());
        if (status_.ok()) {
          onSchema(schemaPtr, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::Channel: {
      if (onChannel) {
        ChannelPtr channelPtr = std::make_shared<Channel>();
        status_ = McapReader::ParseChannel(record, channelPtr.get());
        if (status_.ok()) {
          onChannel(channelPtr, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::Message: {
      if (onMessage) {
        Message message;
        status_ = McapReader::ParseMessage(record, &message);
        if (status_.ok()) {
          onMessage(message, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::Header:
    case OpCode::Footer:
    case OpCode::Chunk:
    case OpCode::MessageIndex:
    case OpCode::ChunkIndex:
    case OpCode::Attachment:
    case OpCode::AttachmentIndex:
    case OpCode::Statistics:
    case OpCode::Metadata:
    case OpCode::MetadataIndex:
    case OpCode::SummaryOffset:
    case OpCode::DataEnd: {
      // These opcodes should not appear inside chunks
      const auto msg =
        internal::StrCat("record type ", uint8_t(record.opcode), " cannot appear in Chunk");
      status_ = Status{StatusCode::InvalidOpCode, msg};
      break;
    }
    default: {
      // Unknown opcode
      if (onUnknownRecord) {
        onUnknownRecord(record, reader_.curRecordOffset());
      }
      break;
    }
  }

  return true;
}

ByteOffset TypedChunkReader::offset() const {
  return reader_.offset;
}

const Status& TypedChunkReader::status() const {
  return status_;
}

// TypedRecordReader ///////////////////////////////////////////////////////////

TypedRecordReader::TypedRecordReader(IReadable& dataSource, ByteOffset startOffset,
                                     ByteOffset endOffset)
    : reader_(dataSource, startOffset, std::min(endOffset, dataSource.size()))
    , status_(StatusCode::Success)
    , parsingChunk_(false) {
  chunkReader_.onSchema = [&](const SchemaPtr schema, ByteOffset chunkOffset) {
    if (onSchema) {
      onSchema(schema, reader_.curRecordOffset(), chunkOffset);
    }
  };
  chunkReader_.onChannel = [&](const ChannelPtr channel, ByteOffset chunkOffset) {
    if (onChannel) {
      onChannel(channel, reader_.curRecordOffset(), chunkOffset);
    }
  };
  chunkReader_.onMessage = [&](const Message& message, ByteOffset chunkOffset) {
    if (onMessage) {
      onMessage(message, reader_.curRecordOffset(), chunkOffset);
    }
  };
  chunkReader_.onUnknownRecord = [&](const Record& record, ByteOffset chunkOffset) {
    if (onUnknownRecord) {
      onUnknownRecord(record, reader_.curRecordOffset(), chunkOffset);
    }
  };
}

bool TypedRecordReader::next() {
  if (parsingChunk_) {
    const bool chunkInProgress = chunkReader_.next();
    status_ = chunkReader_.status();
    if (!chunkInProgress) {
      parsingChunk_ = false;
      if (onChunkEnd) {
        onChunkEnd(reader_.offset);
      }
    }
    return true;
  }

  const auto maybeRecord = reader_.next();
  status_ = reader_.status();
  if (!maybeRecord.has_value()) {
    return false;
  }
  const Record& record = maybeRecord.value();

  switch (record.opcode) {
    case OpCode::Header: {
      if (onHeader) {
        Header header;
        if (status_ = McapReader::ParseHeader(record, &header); status_.ok()) {
          onHeader(header, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::Footer: {
      if (onFooter) {
        Footer footer;
        if (status_ = McapReader::ParseFooter(record, &footer); status_.ok()) {
          onFooter(footer, reader_.curRecordOffset());
        }
      }
      reader_.offset = EndOffset;
      break;
    }
    case OpCode::Schema: {
      if (onSchema) {
        SchemaPtr schemaPtr = std::make_shared<Schema>();
        if (status_ = McapReader::ParseSchema(record, schemaPtr.get()); status_.ok()) {
          onSchema(schemaPtr, reader_.curRecordOffset(), std::nullopt);
        }
      }
      break;
    }
    case OpCode::Channel: {
      if (onChannel) {
        ChannelPtr channelPtr = std::make_shared<Channel>();
        if (status_ = McapReader::ParseChannel(record, channelPtr.get()); status_.ok()) {
          onChannel(channelPtr, reader_.curRecordOffset(), std::nullopt);
        }
      }
      break;
    }
    case OpCode::Message: {
      if (onMessage) {
        Message message;
        if (status_ = McapReader::ParseMessage(record, &message); status_.ok()) {
          onMessage(message, reader_.curRecordOffset(), std::nullopt);
        }
      }
      break;
    }
    case OpCode::Chunk: {
      if (onMessage || onChunk || onSchema || onChannel) {
        Chunk chunk;
        status_ = McapReader::ParseChunk(record, &chunk);
        if (!status_.ok()) {
          return true;
        }
        if (onChunk) {
          onChunk(chunk, reader_.curRecordOffset());
        }
        if (onMessage || onSchema || onChannel) {
          const auto maybeCompression = McapReader::ParseCompression(chunk.compression);
          if (!maybeCompression.has_value()) {
            const auto msg =
              internal::StrCat("unrecognized compression \"", chunk.compression, "\"");
            status_ = Status{StatusCode::UnrecognizedCompression, msg};
            return true;
          }

          // Start iterating through this chunk
          chunkReader_.reset(chunk, maybeCompression.value());
          status_ = chunkReader_.status();
          parsingChunk_ = true;
        }
      }
      break;
    }
    case OpCode::MessageIndex: {
      if (onMessageIndex) {
        MessageIndex messageIndex;
        if (status_ = McapReader::ParseMessageIndex(record, &messageIndex); status_.ok()) {
          onMessageIndex(messageIndex, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::ChunkIndex: {
      if (onChunkIndex) {
        ChunkIndex chunkIndex;
        if (status_ = McapReader::ParseChunkIndex(record, &chunkIndex); status_.ok()) {
          onChunkIndex(chunkIndex, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::Attachment: {
      if (onAttachment) {
        Attachment attachment;
        if (status_ = McapReader::ParseAttachment(record, &attachment); status_.ok()) {
          onAttachment(attachment, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::AttachmentIndex: {
      if (onAttachmentIndex) {
        AttachmentIndex attachmentIndex;
        if (status_ = McapReader::ParseAttachmentIndex(record, &attachmentIndex); status_.ok()) {
          onAttachmentIndex(attachmentIndex, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::Statistics: {
      if (onStatistics) {
        Statistics statistics;
        if (status_ = McapReader::ParseStatistics(record, &statistics); status_.ok()) {
          onStatistics(statistics, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::Metadata: {
      if (onMetadata) {
        Metadata metadata;
        if (status_ = McapReader::ParseMetadata(record, &metadata); status_.ok()) {
          onMetadata(metadata, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::MetadataIndex: {
      if (onMetadataIndex) {
        MetadataIndex metadataIndex;
        if (status_ = McapReader::ParseMetadataIndex(record, &metadataIndex); status_.ok()) {
          onMetadataIndex(metadataIndex, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::SummaryOffset: {
      if (onSummaryOffset) {
        SummaryOffset summaryOffset;
        if (status_ = McapReader::ParseSummaryOffset(record, &summaryOffset); status_.ok()) {
          onSummaryOffset(summaryOffset, reader_.curRecordOffset());
        }
      }
      break;
    }
    case OpCode::DataEnd: {
      if (onDataEnd) {
        DataEnd dataEnd;
        if (status_ = McapReader::ParseDataEnd(record, &dataEnd); status_.ok()) {
          onDataEnd(dataEnd, reader_.curRecordOffset());
        }
      }
      break;
    }
    default:
      if (onUnknownRecord) {
        onUnknownRecord(record, reader_.curRecordOffset(), std::nullopt);
      }
      break;
  }

  return true;
}

ByteOffset TypedRecordReader::offset() const {
  return reader_.offset + (parsingChunk_ ? chunkReader_.offset() : 0);
}

const Status& TypedRecordReader::status() const {
  return status_;
}

// LinearMessageView ///////////////////////////////////////////////////////////

LinearMessageView::LinearMessageView(McapReader& mcapReader, const ProblemCallback& onProblem)
    : mcapReader_(mcapReader)
    , dataStart_(0)
    , dataEnd_(0)
    , onProblem_(onProblem) {}

LinearMessageView::LinearMessageView(McapReader& mcapReader, ByteOffset dataStart,
                                     ByteOffset dataEnd, Timestamp startTime, Timestamp endTime,
                                     const ProblemCallback& onProblem)
    : mcapReader_(mcapReader)
    , dataStart_(dataStart)
    , dataEnd_(dataEnd)
    , readMessageOptions_(startTime, endTime)
    , onProblem_(onProblem) {}

LinearMessageView::LinearMessageView(McapReader& mcapReader, const ReadMessageOptions& options,
                                     ByteOffset dataStart, ByteOffset dataEnd,
                                     const ProblemCallback& onProblem)
    : mcapReader_(mcapReader)
    , dataStart_(dataStart)
    , dataEnd_(dataEnd)
    , readMessageOptions_(options)
    , onProblem_(onProblem) {}

LinearMessageView::Iterator LinearMessageView::begin() {
  if (dataStart_ == dataEnd_ || !mcapReader_.dataSource()) {
    return end();
  }
  return LinearMessageView::Iterator{*this};
}

LinearMessageView::Iterator LinearMessageView::end() {
  return LinearMessageView::Iterator();
}

// LinearMessageView::Iterator /////////////////////////////////////////////////

LinearMessageView::Iterator::Iterator(LinearMessageView& view)
    : impl_(std::make_unique<Impl>(view)) {
  if (!impl_->has_value()) {
    impl_ = nullptr;
  }
}

LinearMessageView::Iterator::Impl::Impl(LinearMessageView& view)
    : view_(view) {
  auto dataStart = view.dataStart_;
  auto dataEnd = view.dataEnd_;
  auto readMessageOptions = view.readMessageOptions_;
  if (readMessageOptions.readOrder == ReadMessageOptions::ReadOrder::FileOrder) {
    recordReader_.emplace(*(view_.mcapReader_.dataSource()), dataStart, dataEnd);

    recordReader_->onSchema = [this](const SchemaPtr schema, ByteOffset,
                                     std::optional<ByteOffset>) {
      view_.mcapReader_.schemas_.insert_or_assign(schema->id, schema);
    };
    recordReader_->onChannel = [this](const ChannelPtr channel, ByteOffset,
                                      std::optional<ByteOffset>) {
      view_.mcapReader_.channels_.insert_or_assign(channel->id, channel);
    };
    recordReader_->onMessage = [this](const Message& message, ByteOffset messageStartOffset,
                                      std::optional<ByteOffset> chunkStartOffset) {
      RecordOffset offset;
      offset.chunkOffset = chunkStartOffset;
      offset.offset = messageStartOffset;
      onMessage(message, offset);
    };
  } else {
    indexedMessageReader_.emplace(view_.mcapReader_, readMessageOptions,
                                  std::bind(&LinearMessageView::Iterator::Impl::onMessage, this,
                                            std::placeholders::_1, std::placeholders::_2));
  }

  increment();
}

/**
 * @brief Receives a message from either the linear TypedRecordReader or IndexedMessageReader.
 * Sets `curMessageView` with the message along with its associated Channel and Schema.
 */
void LinearMessageView::Iterator::Impl::onMessage(const Message& message, RecordOffset offset) {
  // make sure the message is within the expected time range
  if (message.logTime < view_.readMessageOptions_.startTime) {
    return;
  }
  if (message.logTime >= view_.readMessageOptions_.endTime) {
    return;
  }
  auto maybeChannel = view_.mcapReader_.channel(message.channelId);
  if (!maybeChannel) {
    view_.onProblem_(
      Status{StatusCode::InvalidChannelId,
             internal::StrCat("message at log_time ", message.logTime, " (seq ", message.sequence,
                              ") references missing channel id ", message.channelId)});
    return;
  }

  auto& channel = *maybeChannel;
  // make sure the message is on the right topic
  if (view_.readMessageOptions_.topicFilter &&
      !view_.readMessageOptions_.topicFilter(channel.topic)) {
    return;
  }
  SchemaPtr maybeSchema;
  if (channel.schemaId != 0) {
    maybeSchema = view_.mcapReader_.schema(channel.schemaId);
    if (!maybeSchema) {
      view_.onProblem_(
        Status{StatusCode::InvalidSchemaId,
               internal::StrCat("channel ", channel.id, " (", channel.topic,
                                ") references missing schema id ", channel.schemaId)});
      return;
    }
  }

  curMessage_ = message;  // copy message, which may be a reference to a temporary
  curMessageView_.emplace(curMessage_, maybeChannel, maybeSchema, offset);
}

void LinearMessageView::Iterator::Impl::increment() {
  curMessageView_ = std::nullopt;

  if (recordReader_.has_value()) {
    while (!curMessageView_.has_value()) {
      // Iterate through records until curMessageView_ gets filled with a value.
      const bool found = recordReader_->next();

      // Surface any problem that may have occurred while reading
      auto& status = recordReader_->status();
      if (!status.ok()) {
        view_.onProblem_(status);
      }

      if (!found) {
        recordReader_ = std::nullopt;
        return;
      }
    }
  } else if (indexedMessageReader_.has_value()) {
    while (!curMessageView_.has_value()) {
      // Iterate through records until curMessageView_ gets filled with a value.
      if (!indexedMessageReader_->next()) {
        // No message was found on last iteration - if this was because of an error,
        // alert with onProblem_.
        auto status = indexedMessageReader_->status();
        if (!status.ok()) {
          view_.onProblem_(status);
        }
        indexedMessageReader_ = std::nullopt;
        return;
      }
    }
  }
}

LinearMessageView::Iterator::reference LinearMessageView::Iterator::Impl::dereference() const {
  return *curMessageView_;
}

bool LinearMessageView::Iterator::Impl::has_value() const {
  return curMessageView_.has_value();
}

LinearMessageView::Iterator::reference LinearMessageView::Iterator::operator*() const {
  return impl_->dereference();
}

LinearMessageView::Iterator::pointer LinearMessageView::Iterator::operator->() const {
  return &impl_->dereference();
}

LinearMessageView::Iterator& LinearMessageView::Iterator::operator++() {
  begun_ = true;
  impl_->increment();
  if (!impl_->has_value()) {
    impl_ = nullptr;
  }
  return *this;
}

void LinearMessageView::Iterator::operator++(int) {
  ++*this;
}

bool operator==(const LinearMessageView::Iterator& a, const LinearMessageView::Iterator& b) {
  if (a.impl_ == nullptr || b.impl_ == nullptr) {
    // special case for Iterator::end() == Iterator::end()
    return a.impl_ == b.impl_;
  }
  if (!a.begun_ && !b.begun_) {
    // special case for Iterator::begin() == Iterator::begin()
    // comparing iterators to the beginning of the same view should return true.
    return &(a.impl_->view_) == &(b.impl_->view_);
  }
  // In all other cases, compare by object identity.
  return &(a) == &(b);
}

bool operator!=(const LinearMessageView::Iterator& a, const LinearMessageView::Iterator& b) {
  return !(a == b);
}

Status ReadMessageOptions::validate() const {
  if (startTime > endTime) {
    return Status(StatusCode::InvalidMessageReadOptions, "start time must be before end time");
  }
  return Status();
}

// IndexedMessageReader ///////////////////////////////////////////////////////////
IndexedMessageReader::IndexedMessageReader(
  McapReader& reader, const ReadMessageOptions& options,
  const std::function<void(const Message&, RecordOffset)> onMessage)
    : mcapReader_(reader)
    , recordReader_(*mcapReader_.dataSource(), 0, 0)
    , options_(options)
    , onMessage_(onMessage)
    , queue_(options_.readOrder == ReadMessageOptions::ReadOrder::ReverseLogTimeOrder) {
  auto chunkIndexes = mcapReader_.chunkIndexes();
  if (chunkIndexes.size() == 0) {
    status_ = mcapReader_.readSummary(ReadSummaryMethod::AllowFallbackScan);
    if (!status_.ok()) {
      return;
    }
    chunkIndexes = mcapReader_.chunkIndexes();
  }
  if (chunkIndexes.size() == 0 ||
      std::all_of(chunkIndexes.begin(), chunkIndexes.end(), [](const ChunkIndex& ci) {
        return ci.messageIndexLength == 0;
      })) {
    status_ = Status(StatusCode::NoMessageIndexesAvailable,
                     "cannot read MCAP in time order with no message indexes");
    return;
  }
  for (const auto& [channelId, channel] : mcapReader_.channels()) {
    if (!options_.topicFilter || options_.topicFilter(channel->topic)) {
      selectedChannels_.insert(channelId);
    }
  }
  // Initialize the read job queue by finding all of the chunks that need to be read from.
  for (const auto& chunkIndex : mcapReader_.chunkIndexes()) {
    if (chunkIndex.messageStartTime >= options_.endTime) {
      // chunk starts after requested time range, skip it.
      continue;
    }
    if (chunkIndex.messageEndTime < options_.startTime) {
      // chunk end before requested time range starts, skip it.
      continue;
    }
    for (const auto& channelId : selectedChannels_) {
      if (chunkIndex.messageIndexOffsets.find(channelId) != chunkIndex.messageIndexOffsets.end()) {
        internal::DecompressChunkJob job;
        job.chunkStartOffset = chunkIndex.chunkStartOffset;
        job.messageIndexEndOffset =
          chunkIndex.chunkStartOffset + chunkIndex.chunkLength + chunkIndex.messageIndexLength;
        job.messageStartTime = chunkIndex.messageStartTime;
        job.messageEndTime = chunkIndex.messageEndTime;
        queue_.push(std::move(job));
        break;
      }
    }
  }
}

size_t IndexedMessageReader::findFreeChunkSlot() {
  for (size_t chunkReaderIndex = 0; chunkReaderIndex < chunkSlots_.size(); chunkReaderIndex++) {
    if (chunkSlots_[chunkReaderIndex].unreadMessages == 0) {
      return chunkReaderIndex;
    }
  }
  chunkSlots_.emplace_back();
  return chunkSlots_.size() - 1;
}

void IndexedMessageReader::decompressChunk(const Chunk& chunk,
                                           IndexedMessageReader::ChunkSlot& slot) {
  auto compression = McapReader::ParseCompression(chunk.compression);
  if (!compression.has_value()) {
    status_ = Status(StatusCode::UnrecognizedCompression,
                     internal::StrCat("unrecognized compression: ", chunk.compression));
    return;
  }
  slot.decompressedChunk.clear();
  if (*compression == Compression::None) {
    slot.decompressedChunk.insert(slot.decompressedChunk.end(), &chunk.records[0],
                                  &chunk.records[chunk.uncompressedSize]);
  }
#ifndef MCAP_COMPRESSION_NO_LZ4
  else if (*compression == Compression::Lz4) {
    status_ = lz4Reader_.decompressAll(chunk.records, chunk.compressedSize, chunk.uncompressedSize,
                                       &slot.decompressedChunk);
  }
#endif
#ifndef MCAP_COMPRESSION_NO_ZSTD
  else if (*compression == Compression::Zstd) {
    status_ = ZStdReader::DecompressAll(chunk.records, chunk.compressedSize, chunk.uncompressedSize,
                                        &slot.decompressedChunk);
  }
#endif
  else {
    status_ = Status(StatusCode::UnsupportedCompression,
                     internal::StrCat("unhandled compression: ", chunk.compression));
  }
}

bool IndexedMessageReader::next() {
  while (queue_.len() != 0) {
    auto nextItem = queue_.pop();
    if (std::holds_alternative<internal::DecompressChunkJob>(nextItem)) {
      const auto& decompressChunkJob = std::get<internal::DecompressChunkJob>(nextItem);
      // The job here is to decompress the chunk into a slot, then use the message
      // indices after the chunk to push ReadMessageJobs onto the queue for every message
      // in that chunk that needs to be read.

      // First, find a chunk slot to decompress this chunk into.
      size_t chunkReaderIndex = findFreeChunkSlot();
      auto& chunkSlot = chunkSlots_[chunkReaderIndex];
      chunkSlot.chunkStartOffset = decompressChunkJob.chunkStartOffset;
      // Point the record reader at the chunk and message indices after it.
      recordReader_.reset(*mcapReader_.dataSource(), decompressChunkJob.chunkStartOffset,
                          decompressChunkJob.messageIndexEndOffset);
      for (auto record = recordReader_.next(); record != std::nullopt;
           record = recordReader_.next()) {
        switch (record->opcode) {
          case OpCode::Chunk: {
            Chunk chunk;
            status_ = McapReader::ParseChunk(*record, &chunk);
            if (!status_.ok()) {
              return false;
            }
            decompressChunk(chunk, chunkSlot);
            if (!status_.ok()) {
              return false;
            }
          } break;
          case OpCode::MessageIndex: {
            MessageIndex messageIndex;
            status_ = McapReader::ParseMessageIndex(*record, &messageIndex);
            if (!status_.ok()) {
              return false;
            }
            if (selectedChannels_.find(messageIndex.channelId) != selectedChannels_.end()) {
              for (const auto& [timestamp, byteOffset] : messageIndex.records) {
                if (timestamp >= options_.startTime && timestamp < options_.endTime) {
                  internal::ReadMessageJob job;
                  job.chunkReaderIndex = chunkReaderIndex;
                  job.offset.offset = byteOffset;
                  job.offset.chunkOffset = decompressChunkJob.chunkStartOffset;
                  job.timestamp = timestamp;
                  queue_.push(std::move(job));
                  chunkSlot.unreadMessages++;
                }
              }
            }
          } break;
          default:
            status_ = Status(StatusCode::InvalidRecord,
                             internal::StrCat("expected only chunks and message indices, found ",
                                              OpCodeString(record->opcode)));
            return false;
        }
      }
    } else if (std::holds_alternative<internal::ReadMessageJob>(nextItem)) {
      // Read the message out of the already-decompressed chunk.
      const auto& readMessageJob = std::get<internal::ReadMessageJob>(nextItem);
      auto& chunkSlot = chunkSlots_[readMessageJob.chunkReaderIndex];
      assert(chunkSlot.unreadMessages > 0);
      chunkSlot.unreadMessages--;
      BufferReader reader;
      reader.reset(chunkSlot.decompressedChunk.data(), chunkSlot.decompressedChunk.size(),
                   chunkSlot.decompressedChunk.size());
      recordReader_.reset(reader, readMessageJob.offset.offset, chunkSlot.decompressedChunk.size());
      auto record = recordReader_.next();
      status_ = recordReader_.status();
      if (!status_.ok()) {
        return false;
      }
      if (record->opcode != OpCode::Message) {
        status_ =
          Status(StatusCode::InvalidRecord,
                 internal::StrCat("expected a message record, got ", OpCodeString(record->opcode)));
        return false;
      }
      Message message;
      status_ = McapReader::ParseMessage(*record, &message);
      if (!status_.ok()) {
        return false;
      }
      onMessage_(message, readMessageJob.offset);
      return true;
    }
  }
  return false;
}

Status IndexedMessageReader::status() const {
  return status_;
}

}  // namespace mcap
