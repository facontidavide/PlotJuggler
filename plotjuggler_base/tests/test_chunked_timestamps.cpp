#include <PlotJuggler/chunked_timestamps.h>
#include <PlotJuggler/plotdatabase.h>
#include <PlotJuggler/timeseries.h>
#include <gtest/gtest.h>
#include <cmath>
#include <random>

using namespace PJ;

// ===========================================================================
// TimestampChunk basic tests
// ===========================================================================

TEST(TimestampChunk, DefaultState)
{
  TimestampChunk chunk;
  EXPECT_EQ(chunk.count, 0u);
  EXPECT_EQ(chunk.start_offset, 0u);
  EXPECT_EQ(chunk.min_x, std::numeric_limits<double>::max());
  EXPECT_EQ(chunk.max_x, std::numeric_limits<double>::lowest());
  EXPECT_TRUE(chunk.values.empty());
}

TEST(TimestampChunk, ValueAccess)
{
  TimestampChunk chunk;
  chunk.values = { 1.0, 2.0, 3.0 };
  chunk.count = 3;
  chunk.start_offset = 0;

  EXPECT_DOUBLE_EQ(chunk.valueAt(0), 1.0);
  EXPECT_DOUBLE_EQ(chunk.valueAt(1), 2.0);
  EXPECT_DOUBLE_EQ(chunk.valueAt(2), 3.0);

  chunk.valueRef(1) = 42.0;
  EXPECT_DOUBLE_EQ(chunk.valueAt(1), 42.0);
}

TEST(TimestampChunk, StartOffsetAccess)
{
  TimestampChunk chunk;
  chunk.values = { 1.0, 2.0, 3.0, 4.0 };
  chunk.count = 2;
  chunk.start_offset = 2;

  EXPECT_DOUBLE_EQ(chunk.valueAt(0), 3.0);
  EXPECT_DOUBLE_EQ(chunk.valueAt(1), 4.0);
}

// ===========================================================================
// ChunkedTimestamps basic operations
// ===========================================================================

TEST(ChunkedTimestamps, EmptyState)
{
  ChunkedTimestamps ts;
  EXPECT_TRUE(ts.empty());
  EXPECT_EQ(ts.size(), 0u);
}

TEST(ChunkedTimestamps, PushBackAndAccess)
{
  ChunkedTimestamps ts;
  ts.push_back(1.0);
  ts.push_back(2.0);
  ts.push_back(3.0);

  EXPECT_EQ(ts.size(), 3u);
  EXPECT_FALSE(ts.empty());
  EXPECT_DOUBLE_EQ(ts[0], 1.0);
  EXPECT_DOUBLE_EQ(ts[1], 2.0);
  EXPECT_DOUBLE_EQ(ts[2], 3.0);
}

TEST(ChunkedTimestamps, FrontBack)
{
  ChunkedTimestamps ts;
  ts.push_back(10.0);
  ts.push_back(20.0);
  ts.push_back(30.0);

  EXPECT_DOUBLE_EQ(ts.front(), 10.0);
  EXPECT_DOUBLE_EQ(ts.back(), 30.0);
}

TEST(ChunkedTimestamps, Clear)
{
  ChunkedTimestamps ts;
  for (int i = 0; i < 100; i++)
  {
    ts.push_back(static_cast<double>(i));
  }
  EXPECT_EQ(ts.size(), 100u);
  ts.clear();
  EXPECT_TRUE(ts.empty());
  EXPECT_EQ(ts.size(), 0u);
}

// ===========================================================================
// Streaming data patterns (push_back / pop_front)
// ===========================================================================

TEST(ChunkedTimestamps, StreamingPushPop)
{
  ChunkedTimestamps ts;

  // Simulate streaming: push 2048 points, then pop first 1024
  for (int i = 0; i < 2048; i++)
  {
    ts.push_back(static_cast<double>(i));
  }
  EXPECT_EQ(ts.size(), 2048u);
  EXPECT_DOUBLE_EQ(ts.front(), 0.0);
  EXPECT_DOUBLE_EQ(ts.back(), 2047.0);

  for (int i = 0; i < 1024; i++)
  {
    ts.pop_front();
  }
  EXPECT_EQ(ts.size(), 1024u);
  EXPECT_DOUBLE_EQ(ts.front(), 1024.0);
  EXPECT_DOUBLE_EQ(ts.back(), 2047.0);

  // Verify random access after pop
  for (size_t i = 0; i < ts.size(); i++)
  {
    EXPECT_DOUBLE_EQ(ts[i], static_cast<double>(i + 1024));
  }
}

TEST(ChunkedTimestamps, StreamingContinuousPushPop)
{
  // Simulate real-time streaming: push + pop interleaved over many chunks
  ChunkedTimestamps ts;
  const size_t window = 500;
  const size_t total_pushes = 5000;

  for (size_t i = 0; i < total_pushes; i++)
  {
    ts.push_back(static_cast<double>(i));
    if (ts.size() > window)
    {
      ts.pop_front();
    }
  }

  EXPECT_EQ(ts.size(), window);
  EXPECT_DOUBLE_EQ(ts.front(), static_cast<double>(total_pushes - window));
  EXPECT_DOUBLE_EQ(ts.back(), static_cast<double>(total_pushes - 1));
}

TEST(ChunkedTimestamps, PopAllElements)
{
  ChunkedTimestamps ts;
  for (int i = 0; i < 100; i++)
  {
    ts.push_back(static_cast<double>(i));
  }
  for (int i = 0; i < 100; i++)
  {
    ts.pop_front();
  }
  EXPECT_TRUE(ts.empty());
  EXPECT_EQ(ts.size(), 0u);
}

// ===========================================================================
// Chunk boundary tests
// ===========================================================================

TEST(ChunkedTimestamps, ChunkBoundaryPushBack)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;

  // Fill exactly one chunk
  for (uint32_t i = 0; i < CAP; i++)
  {
    ts.push_back(static_cast<double>(i));
  }
  EXPECT_EQ(ts.size(), CAP);
  EXPECT_EQ(ts.chunks().size(), 1u);

  // Push one more — triggers new chunk
  ts.push_back(static_cast<double>(CAP));
  EXPECT_EQ(ts.size(), CAP + 1u);
  EXPECT_EQ(ts.chunks().size(), 2u);
  EXPECT_DOUBLE_EQ(ts[CAP], static_cast<double>(CAP));
}

TEST(ChunkedTimestamps, ChunkBoundaryPopFront)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;

  // Fill two chunks
  for (uint32_t i = 0; i < 2 * CAP; i++)
  {
    ts.push_back(static_cast<double>(i));
  }
  EXPECT_EQ(ts.chunks().size(), 2u);

  // Pop entire first chunk
  for (uint32_t i = 0; i < CAP; i++)
  {
    ts.pop_front();
  }
  EXPECT_EQ(ts.chunks().size(), 1u);
  EXPECT_EQ(ts.size(), CAP);
  EXPECT_DOUBLE_EQ(ts.front(), static_cast<double>(CAP));
}

TEST(ChunkedTimestamps, LocateAcrossChunks)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;
  const size_t N = 3 * CAP + 7;  // partial last chunk

  for (size_t i = 0; i < N; i++)
  {
    ts.push_back(static_cast<double>(i));
  }
  EXPECT_EQ(ts.size(), N);

  // Verify all elements via operator[]
  for (size_t i = 0; i < N; i++)
  {
    EXPECT_DOUBLE_EQ(ts[i], static_cast<double>(i));
  }
}

TEST(ChunkedTimestamps, LocateAfterPopFront)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;

  // Fill 3 chunks, pop 100 from front
  for (size_t i = 0; i < 3 * CAP; i++)
  {
    ts.push_back(static_cast<double>(i));
  }
  for (int i = 0; i < 100; i++)
  {
    ts.pop_front();
  }

  // All remaining must be correct
  for (size_t i = 0; i < ts.size(); i++)
  {
    EXPECT_DOUBLE_EQ(ts[i], static_cast<double>(i + 100));
  }
}

// ===========================================================================
// Min/max metadata correctness
// ===========================================================================

TEST(ChunkedTimestamps, MinMaxTrackingMonotonic)
{
  ChunkedTimestamps ts;
  for (int i = 0; i < 2048; i++)
  {
    ts.push_back(static_cast<double>(i));
  }

  for (const auto& chunk : ts.chunks())
  {
    // For monotonically increasing data, min is the first element, max is the last
    EXPECT_DOUBLE_EQ(chunk->min_x, chunk->valueAt(0));
    EXPECT_DOUBLE_EQ(chunk->max_x, chunk->valueAt(chunk->count - 1));
  }
}

TEST(ChunkedTimestamps, MinMaxTrackingNonMonotonic)
{
  // Timestamps that are locally non-monotonic within a chunk
  ChunkedTimestamps ts;
  // Push 5, 3, 8, 1, 9
  ts.push_back(5.0);
  ts.push_back(3.0);
  ts.push_back(8.0);
  ts.push_back(1.0);
  ts.push_back(9.0);

  auto& chunk = ts.chunks().front();
  EXPECT_DOUBLE_EQ(chunk->min_x, 1.0);
  EXPECT_DOUBLE_EQ(chunk->max_x, 9.0);
}

// ===========================================================================
// lowerBound tests
// ===========================================================================

TEST(ChunkedTimestamps, LowerBoundBasic)
{
  ChunkedTimestamps ts;
  for (int i = 0; i < 100; i++)
  {
    ts.push_back(static_cast<double>(i * 10));
  }

  // Exact match
  EXPECT_EQ(ts.lowerBound(0.0), 0u);
  EXPECT_EQ(ts.lowerBound(50.0), 5u);
  EXPECT_EQ(ts.lowerBound(990.0), 99u);

  // Between values
  EXPECT_EQ(ts.lowerBound(15.0), 2u);  // first >= 15 is 20 at index 2
  EXPECT_EQ(ts.lowerBound(51.0), 6u);  // first >= 51 is 60 at index 6

  // Before all
  EXPECT_EQ(ts.lowerBound(-1.0), 0u);

  // After all
  EXPECT_EQ(ts.lowerBound(1000.0), 100u);  // past the end
}

TEST(ChunkedTimestamps, LowerBoundEmpty)
{
  ChunkedTimestamps ts;
  EXPECT_EQ(ts.lowerBound(42.0), 0u);
}

TEST(ChunkedTimestamps, LowerBoundSingleElement)
{
  ChunkedTimestamps ts;
  ts.push_back(5.0);

  EXPECT_EQ(ts.lowerBound(3.0), 0u);
  EXPECT_EQ(ts.lowerBound(5.0), 0u);
  EXPECT_EQ(ts.lowerBound(7.0), 1u);
}

TEST(ChunkedTimestamps, LowerBoundMultipleChunks)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;
  // 3 full chunks + partial
  const size_t N = 3 * CAP + 100;

  for (size_t i = 0; i < N; i++)
  {
    ts.push_back(static_cast<double>(i));
  }

  // Test exact boundary points
  EXPECT_EQ(ts.lowerBound(0.0), 0u);
  EXPECT_EQ(ts.lowerBound(static_cast<double>(CAP)), CAP);
  EXPECT_EQ(ts.lowerBound(static_cast<double>(2 * CAP)), 2 * CAP);
  EXPECT_EQ(ts.lowerBound(static_cast<double>(3 * CAP)), 3 * CAP);

  // Test mid-chunk
  EXPECT_EQ(ts.lowerBound(static_cast<double>(CAP + 50)), CAP + 50);
}

TEST(ChunkedTimestamps, LowerBoundAfterPopFront)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;

  for (size_t i = 0; i < 3 * CAP; i++)
  {
    ts.push_back(static_cast<double>(i));
  }

  // Pop 100 elements
  for (int i = 0; i < 100; i++)
  {
    ts.pop_front();
  }

  // Lower bound should find the first element >= x in remaining data
  EXPECT_EQ(ts.lowerBound(100.0), 0u);  // first remaining element
  EXPECT_EQ(ts.lowerBound(200.0), 100u);
}

// ===========================================================================
// Insert (flatten-rebuild)
// ===========================================================================

TEST(ChunkedTimestamps, InsertMiddle)
{
  ChunkedTimestamps ts;
  ts.push_back(1.0);
  ts.push_back(3.0);
  ts.push_back(5.0);

  ts.insert(1, 2.0);

  EXPECT_EQ(ts.size(), 4u);
  EXPECT_DOUBLE_EQ(ts[0], 1.0);
  EXPECT_DOUBLE_EQ(ts[1], 2.0);
  EXPECT_DOUBLE_EQ(ts[2], 3.0);
  EXPECT_DOUBLE_EQ(ts[3], 5.0);
}

TEST(ChunkedTimestamps, InsertAtBeginning)
{
  ChunkedTimestamps ts;
  ts.push_back(2.0);
  ts.push_back(3.0);

  ts.insert(0, 1.0);

  EXPECT_EQ(ts.size(), 3u);
  EXPECT_DOUBLE_EQ(ts[0], 1.0);
  EXPECT_DOUBLE_EQ(ts[1], 2.0);
  EXPECT_DOUBLE_EQ(ts[2], 3.0);
}

TEST(ChunkedTimestamps, InsertAtEnd)
{
  ChunkedTimestamps ts;
  ts.push_back(1.0);
  ts.push_back(2.0);

  ts.insert(2, 3.0);

  EXPECT_EQ(ts.size(), 3u);
  EXPECT_DOUBLE_EQ(ts[2], 3.0);
}

// ===========================================================================
// mutableRef (COW for setPoint)
// ===========================================================================

TEST(ChunkedTimestamps, MutableRefBasic)
{
  ChunkedTimestamps ts;
  ts.push_back(1.0);
  ts.push_back(2.0);
  ts.push_back(3.0);

  ts.mutableRef(1) = 42.0;
  EXPECT_DOUBLE_EQ(ts[1], 42.0);
  EXPECT_DOUBLE_EQ(ts[0], 1.0);
  EXPECT_DOUBLE_EQ(ts[2], 3.0);
}

// ===========================================================================
// COW (Copy-On-Write) correctness
// ===========================================================================

TEST(ChunkedTimestamps, CowPopFrontPreservesSharedData)
{
  ChunkedTimestamps ts1;
  const uint32_t CAP = TimestampChunk::CAPACITY;

  // Fill 2 chunks
  for (uint32_t i = 0; i < 2 * CAP; i++)
  {
    ts1.push_back(static_cast<double>(i));
  }

  // Copy via shared_ptr sharing (simulate clonePoints)
  ChunkedTimestamps ts2;
  ts2 = ts1;

  // Verify they share chunks
  EXPECT_EQ(ts1.chunks()[0].get(), ts2.chunks()[0].get());

  // Pop from ts1 — should COW the front chunk
  ts1.pop_front();

  // ts2 should still be intact
  EXPECT_EQ(ts2.size(), 2 * CAP);
  EXPECT_DOUBLE_EQ(ts2.front(), 0.0);

  // ts1 should have the popped version
  EXPECT_EQ(ts1.size(), 2 * CAP - 1);
  EXPECT_DOUBLE_EQ(ts1.front(), 1.0);
}

TEST(ChunkedTimestamps, CowMutableRefPreservesSharedData)
{
  ChunkedTimestamps ts1;
  const uint32_t CAP = TimestampChunk::CAPACITY;

  for (uint32_t i = 0; i < 2 * CAP; i++)
  {
    ts1.push_back(static_cast<double>(i));
  }

  ChunkedTimestamps ts2;
  ts2 = ts1;

  // Mutate ts1 via mutableRef — should COW the affected chunk
  ts1.mutableRef(0) = 999.0;

  // ts2 unchanged
  EXPECT_DOUBLE_EQ(ts2[0], 0.0);
  // ts1 changed
  EXPECT_DOUBLE_EQ(ts1[0], 999.0);
}

// ===========================================================================
// Forward iterator
// ===========================================================================

TEST(ChunkedTimestamps, IteratorRangeFor)
{
  ChunkedTimestamps ts;
  for (int i = 0; i < 100; i++)
  {
    ts.push_back(static_cast<double>(i));
  }

  double expected = 0.0;
  for (double x : ts)
  {
    EXPECT_DOUBLE_EQ(x, expected);
    expected += 1.0;
  }
  EXPECT_DOUBLE_EQ(expected, 100.0);
}

TEST(ChunkedTimestamps, IteratorAcrossChunks)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;
  const size_t N = 2 * CAP + 50;

  for (size_t i = 0; i < N; i++)
  {
    ts.push_back(static_cast<double>(i));
  }

  size_t count = 0;
  for (double x : ts)
  {
    EXPECT_DOUBLE_EQ(x, static_cast<double>(count));
    count++;
  }
  EXPECT_EQ(count, N);
}

// ===========================================================================
// Dedup support methods
// ===========================================================================

TEST(ChunkedTimestamps, LastSealedChunkNoneWhenFew)
{
  ChunkedTimestamps ts;
  EXPECT_EQ(ts.lastSealedChunk(), nullptr);

  // Partially filled single chunk
  ts.push_back(1.0);
  EXPECT_EQ(ts.lastSealedChunk(), nullptr);
}

TEST(ChunkedTimestamps, LastSealedChunkReturnsFullChunk)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;

  // Fill one chunk + 1 extra (starts second chunk)
  for (uint32_t i = 0; i <= CAP; i++)
  {
    ts.push_back(static_cast<double>(i));
  }

  auto sealed = ts.lastSealedChunk();
  ASSERT_NE(sealed, nullptr);
  EXPECT_EQ(sealed->count, CAP);
}

TEST(ChunkedTimestamps, ReplaceChunk)
{
  ChunkedTimestamps ts;
  const uint32_t CAP = TimestampChunk::CAPACITY;

  for (uint32_t i = 0; i < CAP + 1; i++)
  {
    ts.push_back(static_cast<double>(i));
  }

  auto replacement = std::make_shared<TimestampChunk>();
  replacement->count = CAP;
  replacement->values.resize(CAP, 42.0);
  replacement->min_x = 42.0;
  replacement->max_x = 42.0;

  ts.replaceChunk(0, replacement);
  EXPECT_DOUBLE_EQ(ts[0], 42.0);
  EXPECT_DOUBLE_EQ(ts[CAP - 1], 42.0);
  // Last element (in second chunk) unchanged
  EXPECT_DOUBLE_EQ(ts[CAP], static_cast<double>(CAP));
}

// ===========================================================================
// TimestampRegistry tests
// ===========================================================================

TEST(TimestampRegistry, DeduplicateIdenticalChunks)
{
  TimestampRegistry registry;

  auto chunk1 = std::make_shared<TimestampChunk>();
  chunk1->count = 3;
  chunk1->values = { 1.0, 2.0, 3.0 };
  chunk1->min_x = 1.0;
  chunk1->max_x = 3.0;

  auto chunk2 = std::make_shared<TimestampChunk>();
  chunk2->count = 3;
  chunk2->values = { 1.0, 2.0, 3.0 };
  chunk2->min_x = 1.0;
  chunk2->max_x = 3.0;

  auto result1 = registry.deduplicate(chunk1);
  EXPECT_EQ(result1.get(), chunk1.get());  // first time: register

  auto result2 = registry.deduplicate(chunk2);
  EXPECT_EQ(result2.get(), chunk1.get());  // second time: deduplicated to chunk1
  EXPECT_NE(result2.get(), chunk2.get());
}

TEST(TimestampRegistry, DifferentChunksNotDeduplicated)
{
  TimestampRegistry registry;

  auto chunk1 = std::make_shared<TimestampChunk>();
  chunk1->count = 3;
  chunk1->values = { 1.0, 2.0, 3.0 };
  chunk1->min_x = 1.0;
  chunk1->max_x = 3.0;

  auto chunk2 = std::make_shared<TimestampChunk>();
  chunk2->count = 3;
  chunk2->values = { 4.0, 5.0, 6.0 };
  chunk2->min_x = 4.0;
  chunk2->max_x = 6.0;

  auto result1 = registry.deduplicate(chunk1);
  auto result2 = registry.deduplicate(chunk2);

  EXPECT_EQ(result1.get(), chunk1.get());
  EXPECT_EQ(result2.get(), chunk2.get());
}

TEST(TimestampRegistry, Clear)
{
  TimestampRegistry registry;

  auto chunk = std::make_shared<TimestampChunk>();
  chunk->count = 2;
  chunk->values = { 1.0, 2.0 };
  chunk->min_x = 1.0;
  chunk->max_x = 2.0;

  registry.deduplicate(chunk);
  registry.clear();

  // After clear, same content should not find a match
  auto chunk2 = std::make_shared<TimestampChunk>();
  chunk2->count = 2;
  chunk2->values = { 1.0, 2.0 };
  chunk2->min_x = 1.0;
  chunk2->max_x = 2.0;

  auto result = registry.deduplicate(chunk2);
  EXPECT_EQ(result.get(), chunk2.get());  // not chunk, since registry was cleared
}

TEST(TimestampRegistry, DeduplicateWithStartOffset)
{
  TimestampRegistry registry;

  // chunk1: values at [0,1,2] starting at offset 0
  auto chunk1 = std::make_shared<TimestampChunk>();
  chunk1->count = 3;
  chunk1->start_offset = 0;
  chunk1->values = { 1.0, 2.0, 3.0 };
  chunk1->min_x = 1.0;
  chunk1->max_x = 3.0;

  // chunk2: same active data but at different offset
  auto chunk2 = std::make_shared<TimestampChunk>();
  chunk2->count = 3;
  chunk2->start_offset = 2;
  chunk2->values = { 99.0, 99.0, 1.0, 2.0, 3.0 };
  chunk2->min_x = 1.0;
  chunk2->max_x = 3.0;

  auto result1 = registry.deduplicate(chunk1);
  auto result2 = registry.deduplicate(chunk2);

  // Same active data -> deduplicated
  EXPECT_EQ(result2.get(), chunk1.get());
}

// ===========================================================================
// PlotGroup registry integration
// ===========================================================================

TEST(PlotGroupRegistry, RegistryAccessible)
{
  auto group = std::make_shared<PlotGroup>("test_group");
  auto& registry = group->timestampRegistry();

  auto chunk = std::make_shared<TimestampChunk>();
  chunk->count = 2;
  chunk->values = { 1.0, 2.0 };
  chunk->min_x = 1.0;
  chunk->max_x = 2.0;

  auto result = registry.deduplicate(chunk);
  EXPECT_EQ(result.get(), chunk.get());
}

// ===========================================================================
// PlotDataBase integration tests (streaming data)
// ===========================================================================

using Timeseries = TimeseriesBase<double>;

TEST(PlotDataBaseIntegration, PushBackAndAccess)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  for (int i = 0; i < 100; i++)
  {
    series.pushBack({ static_cast<double>(i), static_cast<double>(i * 2) });
  }

  EXPECT_EQ(series.size(), 100u);
  for (size_t i = 0; i < 100; i++)
  {
    auto pt = series.at(i);
    EXPECT_DOUBLE_EQ(pt.x, static_cast<double>(i));
    EXPECT_DOUBLE_EQ(pt.y, static_cast<double>(i * 2));
  }
}

TEST(PlotDataBaseIntegration, StreamingPushPopPreservesData)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  // Simulate streaming with a sliding window
  const int window = 500;
  for (int i = 0; i < 3000; i++)
  {
    series.pushBack({ static_cast<double>(i), static_cast<double>(i) });
    if (series.size() > static_cast<size_t>(window))
    {
      series.popFront();
    }
  }

  EXPECT_EQ(series.size(), static_cast<size_t>(window));
  EXPECT_DOUBLE_EQ(series.front().x, 2500.0);
  EXPECT_DOUBLE_EQ(series.back().x, 2999.0);
}

TEST(PlotDataBaseIntegration, RangeXOptimized)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  for (int i = 0; i < 2048; i++)
  {
    series.pushBack({ static_cast<double>(i), 0.0 });
  }

  auto range = series.rangeX();
  ASSERT_TRUE(range.has_value());
  EXPECT_DOUBLE_EQ(range->min, 0.0);
  EXPECT_DOUBLE_EQ(range->max, 2047.0);
}

TEST(PlotDataBaseIntegration, RangeXAfterPopFront)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  for (int i = 0; i < 100; i++)
  {
    series.pushBack({ static_cast<double>(i), 0.0 });
  }

  // Pop first 50 elements
  for (int i = 0; i < 50; i++)
  {
    series.popFront();
  }

  auto range = series.rangeX();
  ASSERT_TRUE(range.has_value());
  EXPECT_DOUBLE_EQ(range->min, 50.0);
  EXPECT_DOUBLE_EQ(range->max, 99.0);
}

TEST(PlotDataBaseIntegration, RangeXEmpty)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  auto range = series.rangeX();
  EXPECT_FALSE(range.has_value());
}

// ===========================================================================
// DAG transforms (setPoint, insert)
// ===========================================================================

TEST(PlotDataBaseIntegration, SetPointUpdatesTimestamp)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  for (int i = 0; i < 10; i++)
  {
    series.pushBack({ static_cast<double>(i), static_cast<double>(i) });
  }

  series.setPoint(5, 50.0, 500.0);

  auto pt = series.at(5);
  EXPECT_DOUBLE_EQ(pt.x, 50.0);
  EXPECT_DOUBLE_EQ(pt.y, 500.0);

  // Other points unchanged
  EXPECT_DOUBLE_EQ(series.at(4).x, 4.0);
  EXPECT_DOUBLE_EQ(series.at(6).x, 6.0);
}

TEST(PlotDataBaseIntegration, InsertMidStream)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  // pushBack inserts in sorted order for Timeseries
  series.pushBack({ 1.0, 10.0 });
  series.pushBack({ 3.0, 30.0 });
  series.pushBack({ 5.0, 50.0 });

  // Push an out-of-order timestamp — Timeseries sorts it in
  series.pushBack({ 2.0, 20.0 });

  EXPECT_EQ(series.size(), 4u);
  EXPECT_DOUBLE_EQ(series.at(0).x, 1.0);
  EXPECT_DOUBLE_EQ(series.at(1).x, 2.0);
  EXPECT_DOUBLE_EQ(series.at(2).x, 3.0);
  EXPECT_DOUBLE_EQ(series.at(3).x, 5.0);
}

// ===========================================================================
// getIndexFromX
// ===========================================================================

TEST(PlotDataBaseIntegration, GetIndexFromXExact)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  for (int i = 0; i < 100; i++)
  {
    series.pushBack({ static_cast<double>(i * 10), static_cast<double>(i) });
  }

  EXPECT_EQ(series.getIndexFromX(0.0), 0);
  EXPECT_EQ(series.getIndexFromX(50.0), 5);
  EXPECT_EQ(series.getIndexFromX(990.0), 99);
}

TEST(PlotDataBaseIntegration, GetIndexFromXNearest)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  for (int i = 0; i < 100; i++)
  {
    series.pushBack({ static_cast<double>(i * 10), static_cast<double>(i) });
  }

  // 14 is closer to 10 than to 20
  EXPECT_EQ(series.getIndexFromX(14.0), 1);
  // 16 is closer to 20 than to 10
  EXPECT_EQ(series.getIndexFromX(16.0), 2);
}

TEST(PlotDataBaseIntegration, GetIndexFromXEmpty)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  EXPECT_EQ(series.getIndexFromX(42.0), -1);
}

TEST(PlotDataBaseIntegration, GetIndexFromXBeyondRange)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  for (int i = 0; i < 10; i++)
  {
    series.pushBack({ static_cast<double>(i), static_cast<double>(i) });
  }

  // Before range
  EXPECT_EQ(series.getIndexFromX(-10.0), 0);
  // After range
  EXPECT_EQ(series.getIndexFromX(100.0), 9);
}

TEST(PlotDataBaseIntegration, GetIndexFromXMultipleChunks)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  const uint32_t CAP = TimestampChunk::CAPACITY;
  for (size_t i = 0; i < 3 * CAP; i++)
  {
    series.pushBack({ static_cast<double>(i), static_cast<double>(i) });
  }

  // Test around chunk boundaries
  EXPECT_EQ(series.getIndexFromX(static_cast<double>(CAP - 1)), static_cast<int>(CAP - 1));
  EXPECT_EQ(series.getIndexFromX(static_cast<double>(CAP)), static_cast<int>(CAP));
  EXPECT_EQ(series.getIndexFromX(static_cast<double>(2 * CAP)), static_cast<int>(2 * CAP));
}

// ===========================================================================
// Dedup end-to-end integration
// ===========================================================================

TEST(DedupIntegration, SiblingSeriesShareTimestampChunks)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series_a("a", group);
  Timeseries series_b("b", group);

  const uint32_t CAP = TimestampChunk::CAPACITY;
  // Push identical timestamps into both series
  for (uint32_t i = 0; i < 2 * CAP; i++)
  {
    double t = static_cast<double>(i);
    series_a.pushBack({ t, 1.0 });
    series_b.pushBack({ t, 2.0 });
  }

  // The first (sealed) chunk should be deduplicated
  auto chunk_a = series_a.timestamps().chunks()[0];
  auto chunk_b = series_b.timestamps().chunks()[0];
  EXPECT_EQ(chunk_a.get(), chunk_b.get());
}

TEST(DedupIntegration, DifferentTimestampsNotDeduplicated)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series_a("a", group);
  Timeseries series_b("b", group);

  const uint32_t CAP = TimestampChunk::CAPACITY;
  for (uint32_t i = 0; i < 2 * CAP; i++)
  {
    series_a.pushBack({ static_cast<double>(i), 1.0 });
    series_b.pushBack({ static_cast<double>(i + 1000000), 2.0 });
  }

  auto chunk_a = series_a.timestamps().chunks()[0];
  auto chunk_b = series_b.timestamps().chunks()[0];
  EXPECT_NE(chunk_a.get(), chunk_b.get());
}

TEST(DedupIntegration, NoDedupWithoutGroup)
{
  Timeseries series("no_group", nullptr);

  const uint32_t CAP = TimestampChunk::CAPACITY;
  // This should not crash even without a group
  for (uint32_t i = 0; i < 2 * CAP; i++)
  {
    series.pushBack({ static_cast<double>(i), 1.0 });
  }

  EXPECT_EQ(series.size(), 2 * CAP);
}

TEST(DedupIntegration, CowAfterDedupPreservesData)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series_a("a", group);
  Timeseries series_b("b", group);

  const uint32_t CAP = TimestampChunk::CAPACITY;

  // Push identical timestamps, then push extras so the shared chunk is sealed
  for (uint32_t i = 0; i < CAP + 10; i++)
  {
    double t = static_cast<double>(i);
    series_a.pushBack({ t, 1.0 });
    series_b.pushBack({ t, 2.0 });
  }

  // They should share the first chunk
  EXPECT_EQ(series_a.timestamps().chunks()[0].get(), series_b.timestamps().chunks()[0].get());

  // Pop from series_a — should COW, not affect series_b
  for (int i = 0; i < 10; i++)
  {
    series_a.popFront();
  }

  EXPECT_DOUBLE_EQ(series_a.front().x, 10.0);
  EXPECT_DOUBLE_EQ(series_b.front().x, 0.0);

  // series_b data is fully intact
  for (uint32_t i = 0; i < CAP + 10; i++)
  {
    EXPECT_DOUBLE_EQ(series_b.at(i).x, static_cast<double>(i));
  }
}

TEST(DedupIntegration, DedupViaUnsortedPush)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series_a("a", group);
  Timeseries series_b("b", group);

  const uint32_t CAP = TimestampChunk::CAPACITY;

  // Use pushUnsorted for both series
  for (uint32_t i = 0; i < 2 * CAP; i++)
  {
    double t = static_cast<double>(i);
    series_a.pushUnsorted({ t, 1.0 });
    series_b.pushUnsorted({ t, 2.0 });
  }
  series_a.sort();
  series_b.sort();

  // After sort, dedup happens via push_back during rebuild
  // Note: sort clears and re-pushes, so dedup is triggered
  // The sealed first chunk should be shared
  auto chunk_a = series_a.timestamps().chunks()[0];
  auto chunk_b = series_b.timestamps().chunks()[0];
  EXPECT_EQ(chunk_a.get(), chunk_b.get());
}

// ===========================================================================
// ClonePoints
// ===========================================================================

TEST(PlotDataBaseIntegration, ClonePointsSharesChunks)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series_a("a", group);

  const uint32_t CAP = TimestampChunk::CAPACITY;
  for (uint32_t i = 0; i < 2 * CAP; i++)
  {
    series_a.pushBack({ static_cast<double>(i), static_cast<double>(i) });
  }

  Timeseries series_b("b", group);
  series_b.clonePoints(series_a);

  // Chunks should be shared (cheap copy)
  EXPECT_EQ(series_a.timestamps().chunks()[0].get(), series_b.timestamps().chunks()[0].get());

  // Data should be identical
  EXPECT_EQ(series_a.size(), series_b.size());
  for (size_t i = 0; i < series_a.size(); i++)
  {
    EXPECT_DOUBLE_EQ(series_a.at(i).x, series_b.at(i).x);
  }
}

TEST(PlotDataBaseIntegration, ClonePointsCowOnMutation)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series_a("a", group);

  for (int i = 0; i < 10; i++)
  {
    series_a.pushBack({ static_cast<double>(i), static_cast<double>(i) });
  }

  Timeseries series_b("b", group);
  series_b.clonePoints(series_a);

  // Mutate series_b
  series_b.setPoint(0, 999.0, 999.0);

  // series_a should be unaffected
  EXPECT_DOUBLE_EQ(series_a.at(0).x, 0.0);
  EXPECT_DOUBLE_EQ(series_b.at(0).x, 999.0);
}

// ===========================================================================
// MaxRangeX (trimRange) integration
// ===========================================================================

TEST(PlotDataBaseIntegration, MaxRangeXTrimsCorrectly)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);
  series.setMaximumRangeX(100.0);

  for (int i = 0; i < 200; i++)
  {
    series.pushBack({ static_cast<double>(i), static_cast<double>(i) });
  }

  // With max_range_x=100 and back=199, front should be >= 99
  EXPECT_GE(series.front().x, 99.0);
  EXPECT_DOUBLE_EQ(series.back().x, 199.0);
}

// ===========================================================================
// Sort
// ===========================================================================

TEST(PlotDataBaseIntegration, SortPreservesDataIntegrity)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  // Push unsorted data
  std::vector<double> times = { 5.0, 3.0, 8.0, 1.0, 7.0, 2.0, 6.0, 4.0 };
  for (double t : times)
  {
    series.pushUnsorted({ t, t * 10.0 });
  }
  series.sort();

  EXPECT_EQ(series.size(), 8u);
  for (size_t i = 1; i < series.size(); i++)
  {
    EXPECT_LT(series.at(i - 1).x, series.at(i).x);
  }

  // Check values correspond correctly
  for (size_t i = 0; i < series.size(); i++)
  {
    EXPECT_DOUBLE_EQ(series.at(i).y, series.at(i).x * 10.0);
  }
}

// ===========================================================================
// Edge case: NaN/Inf filtering
// ===========================================================================

TEST(PlotDataBaseIntegration, NanInfFiltered)
{
  auto group = std::make_shared<PlotGroup>("grp");
  Timeseries series("test", group);

  series.pushBack({ 1.0, 10.0 });
  series.pushBack({ std::numeric_limits<double>::infinity(), 20.0 });
  series.pushBack({ 2.0, std::numeric_limits<double>::quiet_NaN() });
  series.pushBack({ std::numeric_limits<double>::quiet_NaN(), 30.0 });
  series.pushBack({ 3.0, 30.0 });

  EXPECT_EQ(series.size(), 2u);
  EXPECT_DOUBLE_EQ(series.at(0).x, 1.0);
  EXPECT_DOUBLE_EQ(series.at(1).x, 3.0);
}

// ===========================================================================
// Large scale test (stress)
// ===========================================================================

TEST(PlotDataBaseIntegration, LargeScaleStreamingWithDedup)
{
  auto group = std::make_shared<PlotGroup>("grp");

  const int NUM_SERIES = 5;
  std::vector<std::unique_ptr<Timeseries>> series;
  for (int s = 0; s < NUM_SERIES; s++)
  {
    series.push_back(std::make_unique<Timeseries>("series_" + std::to_string(s), group));
  }

  const uint32_t CAP = TimestampChunk::CAPACITY;
  const size_t N = 5 * CAP;

  // Push identical timestamps into all series
  for (size_t i = 0; i < N; i++)
  {
    double t = static_cast<double>(i);
    for (int s = 0; s < NUM_SERIES; s++)
    {
      series[s]->pushBack({ t, static_cast<double>(s * 100 + i) });
    }
  }

  // All sealed chunks should be shared across all series
  size_t num_sealed = series[0]->timestamps().chunks().size() - 1;  // last is active
  for (size_t ci = 0; ci < num_sealed; ci++)
  {
    auto ref = series[0]->timestamps().chunks()[ci].get();
    for (int s = 1; s < NUM_SERIES; s++)
    {
      EXPECT_EQ(series[s]->timestamps().chunks()[ci].get(), ref)
          << "Chunk " << ci << " not shared between series 0 and " << s;
    }
  }

  // Verify data integrity
  for (int s = 0; s < NUM_SERIES; s++)
  {
    EXPECT_EQ(series[s]->size(), N);
    for (size_t i = 0; i < N; i++)
    {
      EXPECT_DOUBLE_EQ(series[s]->at(i).x, static_cast<double>(i));
    }
  }
}
