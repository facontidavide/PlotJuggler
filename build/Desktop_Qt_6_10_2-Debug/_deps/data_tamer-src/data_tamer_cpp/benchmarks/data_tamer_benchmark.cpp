#include <benchmark/benchmark.h>
#include "data_tamer/data_sink.hpp"
#include "data_tamer/data_tamer.hpp"
#include "../examples/geometry_types.hpp"

using namespace DataTamer;

class NullSink : public DataSinkBase
{
public:
  ~NullSink() override { stopThread(); }
  void addChannel(std::string const&, Schema const&) override {}
  bool storeSnapshot(const Snapshot&) override { return true; }
};

static void DT_Doubles(benchmark::State& state)
{
  std::vector<double> values(size_t(state.range(0)));

  auto registry = ChannelsRegistry();
  auto channel = registry.getChannel("channel");
  channel->addDataSink(std::make_shared<NullSink>());

  channel->registerValue("values", &values);

  for(auto _ : state)
  {
    channel->takeSnapshot();
  }
}

static void DT_PoseType(benchmark::State& state)
{
  std::vector<TestTypes::Pose> poses(size_t(state.range(0)));

  auto registry = ChannelsRegistry();
  auto channel = registry.getChannel("channel");
  channel->addDataSink(std::make_shared<NullSink>());

  channel->registerValue("values", &poses);

  for(auto _ : state)
  {
    channel->takeSnapshot();
  }
}

BENCHMARK(DT_Doubles)->Arg(125)->Arg(250)->Arg(500)->Arg(1000)->Arg(2000);
BENCHMARK(DT_PoseType)->Arg(125)->Arg(250)->Arg(500)->Arg(1000);

BENCHMARK_MAIN();
