#include <iostream>
#include "data_tamer/data_tamer.hpp"
#include "data_tamer/sinks/dummy_sink.hpp"

int main()
{
  using namespace DataTamer;

  // start defining one or more Sinks that must be added by default.
  // Do addDefaultSink BEFORE creating a channel.
  auto dummy_sink = std::make_shared<DummySink>();
  ChannelsRegistry::Global().addDefaultSink(dummy_sink);

  // Create (or get) a channel using the global registry (singleton)
  auto channel = ChannelsRegistry::Global().getChannel("chan");

  // If you don't want to use addDefaultSink, you can do:
  // channel->addDataSink(std::make_shared<DummySink>())

  // You can register any arithmetic value. You are responsible for their lifetime
  double value_real = 3.14;
  int value_int = 42;
  [[maybe_unused]] auto id1 = channel->registerValue("value_real", &value_real);
  [[maybe_unused]] auto id2 = channel->registerValue("value_int", &value_int);

  // If you prefer to use RAII, use this method instead
  // logged_float will disable itself when it goes out of scope.
  auto logged_float = channel->createLoggedValue<float>("real");

  // this is the way you store the current snapshot of the values
  channel->takeSnapshot();

  // you can modify logged_float like this
  logged_float->set(6.28f);

  // if you want to modify it in a thread-safe manner, you can modify it like this
  // while ptr exists, its mutex will be locked, so make sure you destruct it as soon as you're done!
  if(auto ptr = logged_float->getMutablePtr())
  {
    *ptr += 1.1f;
  }

  // If you want to access logged_float by reference, but you are not planning to modify its value,
  // you should use getConstPtr(), instead. In this way, you will reduce mutex contention.
  if(auto ptr = logged_float->getConstPtr())
  {
    std::cout << "logged_float = " << *ptr << "\n";
  }

  // You can disable a value like this
  channel->setEnabled(id1, false);
  // or
  logged_float->setEnabled(false);

  // The serialized data of the next snapshot will contain
  // only [value_int], i.e. [id2], since the other two are disabled
  channel->takeSnapshot();
}
