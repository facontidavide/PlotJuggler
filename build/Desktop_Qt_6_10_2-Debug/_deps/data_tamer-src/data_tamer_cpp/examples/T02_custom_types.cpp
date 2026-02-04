#include "data_tamer/data_tamer.hpp"
#include "data_tamer/sinks/dummy_sink.hpp"
#include <iostream>
#include <thread>

// check the custom type in the following file,  that defines:
//
// - Point3D
// - Quaternion
// - Pose

#include "geometry_types.hpp"

struct Bar
{
  int a;
};

int main()
{
  using namespace TestTypes;
  using namespace DataTamer;

  auto dummy_sink = std::make_shared<DummySink>();
  ChannelsRegistry::Global().addDefaultSink(dummy_sink);
  auto channel = ChannelsRegistry::Global().getChannel("my_channel");

  // Register as usual
  Point3D point;
  Pose pose;
  std::vector<Point3D> points_vect(5);
  std::array<int32_t, 3> value_array;

  channel->registerValue<Point3D>("point", &point);
  channel->registerValue("pose", &pose);
  channel->registerValue("points", &points_vect);
  channel->registerValue("value_array", &value_array);

  // Print the schema to understand how they are serialized
  std::cout << channel->getSchema() << std::endl;

  // Note has the size of the message is almost the same as the raw data.
  // The only overhead is the size of points_vect
  size_t expected_size = sizeof(double) * 3 +  // point
                         sizeof(double) * 7 +  // pose
                         sizeof(uint32_t) +
                         5 * (sizeof(double) * 3) +  // points_vect and its size
                         sizeof(int32_t) * 3;        // value_array

  channel->takeSnapshot();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::cout << "\nMessage size: " << dummy_sink->latest_snapshot.payload.size()
            << " expected: " << expected_size << std::endl;
}
