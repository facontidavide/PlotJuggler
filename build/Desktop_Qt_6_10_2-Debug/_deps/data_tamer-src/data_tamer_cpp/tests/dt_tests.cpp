#include "data_tamer/data_tamer.hpp"
#include "data_tamer/sinks/dummy_sink.hpp"

#include <gtest/gtest.h>

#include <variant>
#include <string>
#include <thread>

using namespace DataTamer;

TEST(DataTamerBasic, BasicTypes)
{
  for(size_t i = 0; i < TypesCount; i++)
  {
    auto type = static_cast<BasicType>(i);
    ASSERT_EQ(FromStr(ToStr(type)), type);
  }
}

TEST(DataTamerBasic, SinkAdd)
{
  auto dummy_sink_A = std::make_shared<DummySink>();
  auto dummy_sink_B = std::make_shared<DummySink>();

  ChannelsRegistry registry;
  registry.addDefaultSink(dummy_sink_A);

  auto channel = registry.getChannel("chan");
  channel->addDataSink(dummy_sink_B);

  double var = 3.14;
  int count = 49;
  [[maybe_unused]] auto id1 = channel->registerValue("var", &var);
  [[maybe_unused]] auto id2 = channel->registerValue("count", &count);

  const int shapshot_count = 10;
  for(int i = 0; i < shapshot_count; i++)
  {
    channel->takeSnapshot();
    std::this_thread::sleep_for(std::chrono::microseconds(50));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  const auto hash = channel->getSchema().hash;

  ASSERT_EQ(dummy_sink_A->schemas.size(), 1);
  ASSERT_EQ(dummy_sink_A->schemas.begin()->first, hash);
  ASSERT_EQ(dummy_sink_A->snapshots_count[hash], shapshot_count);

  ASSERT_EQ(dummy_sink_B->schemas.size(), 1);
  ASSERT_EQ(dummy_sink_B->schemas.begin()->first, hash);
  ASSERT_EQ(dummy_sink_B->snapshots_count[hash], shapshot_count);
}

TEST(DataTamerBasic, SerializeVariant)
{
  [[maybe_unused]] auto to_str = [](auto&& arg) -> void {
    std::cout << std::to_string(arg) << std::endl;
  };

  auto serializeAndBack = [&](auto const& value) {
    uint8_t buffer[8];
    ValuePtr ptr(&value);
    SerializeMe::SpanBytes buff(buffer, 8);
    ptr.serialize(buff);
    auto var = DeserializeAsVarType(ptr.type(), buffer);
    ;
    std::visit(to_str, var);
    return var;
  };

  double v1 = 69.0;
  auto n1 = serializeAndBack(v1);
  ASSERT_EQ(v1, std::get<double>(n1));

  int32_t v2 = 42;
  auto n2 = serializeAndBack(v2);
  ASSERT_EQ(v2, std::get<int32_t>(n2));

  uint8_t v3 = 200;
  auto n3 = serializeAndBack(v3);
  ASSERT_EQ(v3, std::get<uint8_t>(n3));
}

TEST(DataTamerBasic, TestRegistration)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  double v1 = 69.0;
  double v2 = 77.0;
  double v2_bis = 42.0;

  int32_t i1 = 55;
  int32_t i2 = 44;
  int32_t i2_bis = 33;
  int32_t i3 = 11;

  auto id_v1 = channel->registerValue("v1", &v1);
  auto id_v2 = channel->registerValue("v2", &v2);
  auto id_i1 = channel->registerValue("i1", &i1);
  auto id_i2 = channel->registerValue("i2", &i2);

  // changing the pointer to another one is valid
  // but only if we unregister first
  channel->unregister(id_v2);
  id_v2 = channel->registerValue("v2", &v2_bis);

  // changing type is never allowed, not even if we unregister
  ASSERT_ANY_THROW(channel->registerValue("v2", &i1));

  channel->takeSnapshot();
  // give time to the sink thread to do its work
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // changing the pointer after takeSnapshot is valid
  channel->unregister(id_i2);
  id_i2 = channel->registerValue("i2", &i2_bis);

  // adding a new value after takeSnapshot is not valid (would change the schema)
  ASSERT_ANY_THROW(channel->registerValue("i3", &i3););
  ASSERT_EQ(sink->latest_snapshot.active_mask.size(), 1);

  // payload should contain v1, v2, i1 and i2
  auto expected_size = sizeof(double) * 2 + sizeof(int32_t) * 2;
  ASSERT_EQ(sink->latest_snapshot.payload.size(), expected_size);

  //-----------------------------------------------------------------
  // Unregister or disable some values. This should reduce the size of the snapshot
  channel->unregister(id_v1);
  channel->setEnabled(id_i1, false);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // payload should contain v2, and i2
  auto reduced_size = sizeof(double) + sizeof(int32_t);
  ASSERT_EQ(sink->latest_snapshot.payload.size(), reduced_size);

  //-----------------------------------------------------------------
  // Register and enable again
  channel->registerValue("v1", &v1);
  channel->setEnabled(id_i1, true);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  ASSERT_EQ(sink->latest_snapshot.payload.size(), expected_size);
}

TEST(DataTamerBasic, Vector)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  std::vector<float> vect = { 1, 2, 3, 4 };

  channel->registerValue("vect", &vect);

  const auto expected_size = 4 * sizeof(float) + sizeof(uint32_t);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_EQ(sink->latest_snapshot.payload.size(), expected_size);
}

TEST(DataTamerBasic, Disable)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  double v1 = 11;
  float v2 = 22;
  int32_t v3 = 33;
  uint16_t v4 = 44;
  bool v5 = true;
  std::array<double, 3> v6 = { 1, 2, 3 };
  std::vector<float> v7 = { 1, 2, 3, 4 };

  auto id_v1 = channel->registerValue("v1", &v1);
  auto id_v2 = channel->registerValue("v2", &v2);
  auto id_v3 = channel->registerValue("v3", &v3);
  auto id_v4 = channel->registerValue("v4", &v4);
  auto id_v5 = channel->registerValue("v5", &v5);
  auto id_v6 = channel->registerValue("v6", &v6);
  auto id_v7 = channel->registerValue("v7", &v7);

  size_t expected_size = sizeof(v1) + sizeof(v2) + sizeof(v3) + sizeof(v4) + sizeof(v5) +
                         3 * sizeof(double) + 4 * sizeof(float) + sizeof(uint32_t);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_EQ(sink->latest_snapshot.payload.size(), expected_size);
  ASSERT_EQ(sink->latest_snapshot.active_mask[0], 0b11111111);

  auto checkSize = [&](const auto& id, size_t size) {
    channel->setEnabled(id, false);
    channel->takeSnapshot();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    channel->setEnabled(id, true);

    size_t expected = expected_size - size;
    ASSERT_EQ(sink->latest_snapshot.payload.size(), expected);
  };

  checkSize(id_v1, sizeof(v1));
  ASSERT_EQ(sink->latest_snapshot.active_mask[0], 0b11111110);

  checkSize(id_v2, sizeof(v2));
  ASSERT_EQ(sink->latest_snapshot.active_mask[0], 0b11111101);

  checkSize(id_v3, sizeof(v3));
  ASSERT_EQ(sink->latest_snapshot.active_mask[0], 0b11111011);

  checkSize(id_v4, sizeof(v4));
  ASSERT_EQ(sink->latest_snapshot.active_mask[0], 0b11110111);

  checkSize(id_v5, sizeof(v5));
  ASSERT_EQ(sink->latest_snapshot.active_mask[0], 0b11101111);

  checkSize(id_v6, 3 * sizeof(double));
  ASSERT_EQ(sink->latest_snapshot.active_mask[0], 0b11011111);

  checkSize(id_v7, 4 * sizeof(float) + sizeof(uint32_t));
  ASSERT_EQ(sink->latest_snapshot.active_mask[0], 0b10111111);
}

TEST(DataTamerBasic, VectorWithChangingSize)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  std::vector<float> vect = { 1, 2, 3, 4 };
  channel->registerValue("vect", &vect);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_EQ(sink->latest_snapshot.payload.size(),
            vect.size() * sizeof(float) + sizeof(uint32_t));

  // if we push more elements into the vector, it should
  // work, even if there is a new allocation of memory
  for(int i = 0; i < 6; i++)
  {
    vect.push_back(float(i));
  }

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_EQ(sink->latest_snapshot.payload.size(),
            vect.size() * sizeof(float) + sizeof(uint32_t));

  // same is the vector size is reduced
  vect.resize(5);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_EQ(sink->latest_snapshot.payload.size(),
            vect.size() * sizeof(float) + sizeof(uint32_t));
}

TEST(DataTamerBasic, LockedPtr)
{
  auto channel = LogChannel::create("chan");
  auto logged_float = channel->createLoggedValue<float>("real");
  float val = 3.14f;
  float val2 = 2.72f;
  logged_float->set(val);
  EXPECT_EQ(logged_float->get(), val);

  {
    auto ptr = logged_float->getMutablePtr();

    // expect that we can get the pointer
    EXPECT_TRUE(ptr);
    EXPECT_EQ(*ptr, val);

    // assign a new value to ptr
    *ptr = val2;
  }

  // we should be able to get it again now that ptr is out of scope
  EXPECT_TRUE(logged_float->getMutablePtr());

  // now expect that our assignment to the locked pointer took place
  EXPECT_EQ(logged_float->get(), val2);
}
