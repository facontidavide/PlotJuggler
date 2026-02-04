#include "data_tamer/channel.hpp"
#include "data_tamer/sinks/dummy_sink.hpp"

#include "../examples/geometry_types.hpp"

#include <gtest/gtest.h>

#include <string>
#include <thread>

using namespace DataTamer;
using namespace TestTypes;

struct TestType
{
  double timestamp = 0;
  int count = 0;
  std::vector<Point3D> positions;
  std::array<Pose, 3> poses;

  enum Color : uint8_t
  {
    RED,
    GREEN,
    BLUE,
    UNDEFINED
  };
  Color color = UNDEFINED;
};

template <typename AddField>
std::string_view TypeDefinition(TestType& obj, AddField& add)
{
  add("timestamp", &obj.timestamp);
  add("count", &obj.count);
  add("positions", &obj.positions);
  add("poses", &obj.poses);
  add("color", &obj.color);
  return "TestType";
}

TEST(DataTamerCustom, Registration)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  Pose poseA;
  Pose poseB;
  auto id = channel->registerValue("value", &poseA);
  // unless we unregister it first, it should fail
  ASSERT_ANY_THROW(channel->registerValue("value", &poseB));

  channel->unregister(id);
  // now it should work
  id = channel->registerValue("value", &poseB);

  // changing type is never allowed, not even if we unregister
  Point3D point;
  channel->unregister(id);
  ASSERT_ANY_THROW(channel->registerValue("value", &point));
}

TEST(DataTamerCustom, CustomType1)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  Pose pose = { { 1, 2, 3 }, { 4, 5, 6, 7 } };
  channel->registerValue("pose", &pose);

  TestType my_test;
  my_test.positions.resize(4);
  channel->registerValue("test_value", &my_test);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto expected_size = sizeof(Pose) + sizeof(double) + sizeof(int32_t) +
                       sizeof(uint32_t) + 4 * sizeof(Point3D) + sizeof(TestType::Color) +
                       3 * sizeof(Pose);

  ASSERT_EQ(sink->latest_snapshot.payload.size(), expected_size);

  //-------------------------------------------------
  // check that the schema includes the Point3D definition
  const auto schema = channel->getSchema();
  const std::string schema_txt = ToStr(schema);

  std::cout << schema_txt << std::endl;

  ASSERT_EQ(schema.custom_types.count("Point3D"), 1);
  ASSERT_EQ(schema.custom_types.count("Quaternion"), 1);
  ASSERT_EQ(schema.custom_types.count("Pose"), 1);
  ASSERT_EQ(schema.custom_types.count("TestType"), 1);

  const auto posA = schema_txt.find("Pose pose\n");

  const auto posB = schema_txt.find("===============\n"
                                    "MSG: Point3D\n"
                                    "float64 x\n"
                                    "float64 y\n"
                                    "float64 z\n");

  const auto posC = schema_txt.find("===============\n"
                                    "MSG: Quaternion\n"
                                    "float64 w\n"
                                    "float64 x\n"
                                    "float64 y\n"
                                    "float64 z\n");

  const auto posD = schema_txt.find("===============\n"
                                    "MSG: Pose\n"
                                    "Point3D position\n"
                                    "Quaternion rotation\n");

  const auto posE = schema_txt.find("===============\n"
                                    "MSG: TestType\n"
                                    "float64 timestamp\n"
                                    "int32 count\n"
                                    "Point3D[] positions\n"
                                    "Pose[3] poses\n"
                                    "uint8 color\n");

  ASSERT_TRUE(std::string::npos != posA);
  ASSERT_TRUE(std::string::npos != posB);
  ASSERT_TRUE(std::string::npos != posC);
  ASSERT_TRUE(std::string::npos != posD);
  ASSERT_TRUE(std::string::npos != posE);
  ASSERT_LT(posA, posB);
  ASSERT_LT(posA, posC);
  ASSERT_LT(posA, posD);
  ASSERT_LT(posA, posE);
}

TEST(DataTamerCustom, CustomType2)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  std::array<Point3D, 2> points;
  std::vector<Quaternion> quats(3);

  channel->registerValue("points", &points);
  channel->registerValue("quats", &quats);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto expected_size = 2 * sizeof(Point3D) + 3 * sizeof(Quaternion) + sizeof(uint32_t);

  //-------------------------------------------------
  const auto schema = channel->getSchema();
  const std::string schema_txt = ToStr(schema);

  std::cout << schema_txt << std::endl;

  ASSERT_EQ(sink->latest_snapshot.payload.size(), expected_size);
  ASSERT_EQ(schema.custom_types.count("Point3D"), 1);
  ASSERT_EQ(schema.custom_types.count("Quaternion"), 1);

  const auto posA = schema_txt.find("Point3D[2] points\n"
                                    "Quaternion[] quats\n");

  const auto posB = schema_txt.find("===============\n"
                                    "MSG: Point3D\n"
                                    "float64 x\n"
                                    "float64 y\n"
                                    "float64 z\n");

  const auto posC = schema_txt.find("===============\n"
                                    "MSG: Quaternion\n"
                                    "float64 w\n"
                                    "float64 x\n"
                                    "float64 y\n"
                                    "float64 z\n");

  ASSERT_TRUE(std::string::npos != posA);
  ASSERT_TRUE(std::string::npos != posB);
  ASSERT_TRUE(std::string::npos != posC);
  ASSERT_LT(posA, posB);
  ASSERT_LT(posA, posC);
}

struct Pos2D
{
  int32_t x;
  int32_t y;
};

class Pos2D_Serializer : public CustomSerializer
{
public:
  ~Pos2D_Serializer() override = default;

  const std::string& typeName() const override
  {
    static std::string name = "Point2D";
    return name;
  }

  std::optional<CustomSchema> typeSchema() const override
  {
    return CustomSchema{ "ros1", "int32 x\nint32 y" };
  }

  size_t serializedSize(const void*) const override { return sizeof(Pos2D); }

  bool isFixedSize() const override { return true; }

  void serialize(const void* src, SerializeMe::SpanBytes& buffer) const override
  {
    const auto* obj = static_cast<const Pos2D*>(src);
    SerializeMe::SerializeIntoBuffer(buffer, obj->x);
    SerializeMe::SerializeIntoBuffer(buffer, obj->y);
  }
};

TEST(DataTamerCustom, CustomType3)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  auto serializer = std::make_shared<Pos2D_Serializer>();

  Pos2D v1 = { 1, 2 };
  std::vector<Pos2D> v2(2);
  v2[0] = { 3, 4 };
  v2[1] = { 5, 6 };
  std::array<Pos2D, 3> v3;
  v3[0] = { 7, 8 };
  v3[1] = { 9, 10 };
  v3[2] = { 11, 12 };

  channel->registerCustomValue("v1", &v1, serializer);
  channel->registerCustomValue("v2", &v2, serializer);
  channel->registerCustomValue("v3", &v3, serializer);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const auto expected_size = 6 * sizeof(Pos2D) + sizeof(uint32_t);

  //-------------------------------------------------
  const auto schema = channel->getSchema();
  const std::string schema_txt = ToStr(schema);

  std::cout << schema_txt << std::endl;

  ASSERT_EQ(sink->latest_snapshot.payload.size(), expected_size);
  ASSERT_EQ(schema.custom_types.size(), 0);
  ASSERT_EQ(schema.custom_schemas.size(), 1);

  const auto posA = schema_txt.find("Point2D v1\n"
                                    "Point2D[] v2\n"
                                    "Point2D[3] v3\n");

  const auto posB = schema_txt.find("===============\n"
                                    "MSG: Point2D\n"
                                    "ENCODING: ros1\n"
                                    "int32 x\n"
                                    "int32 y\n");

  ASSERT_TRUE(std::string::npos != posA);
  ASSERT_TRUE(std::string::npos != posB);
  ASSERT_LT(posA, posB);
}

TEST(DataTamerCustom, RegisterConstMethods)
{
  auto channel = LogChannel::create("chan");
  auto sink = std::make_shared<DummySink>();
  channel->addDataSink(sink);

  PseudoEigen::Vector2d vect = { 1, 2 };
  channel->registerValue("vect", &vect);

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  const auto expected_size = 2 * sizeof(double);

  // //-------------------------------------------------
  const auto schema = channel->getSchema();
  const std::string schema_txt = ToStr(schema);

  std::cout << schema_txt << std::endl;

  ASSERT_EQ(sink->latest_snapshot.payload.size(), expected_size);
  ASSERT_EQ(schema.custom_types.size(), 1);
  ASSERT_EQ(schema.custom_schemas.size(), 0);

  const auto posA = schema_txt.find("Vector2d vect\n");

  const auto posB = schema_txt.find("===============\n"
                                    "MSG: Vector2d\n"
                                    "float64 x\n"
                                    "float64 y\n");

  ASSERT_TRUE(std::string::npos != posA);
  ASSERT_TRUE(std::string::npos != posB);
  ASSERT_LT(posA, posB);
}
