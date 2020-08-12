/*
 *  Created on: Apr 16, 2018
 *      Author: Poom Pianpak
 */

#include "rosbridge_ws_client.hpp"

#include <future>

RosbridgeWsClient rbc("10.10.20.138/sda/rosbridge");

void advertiseServiceCallback(std::shared_ptr<WsClient::Connection> /*connection*/, std::shared_ptr<WsClient::InMessage> in_message)
{
  // message->string() is destructive, so we have to buffer it first
  std::string messagebuf = in_message->string();
  std::cout << "advertiseServiceCallback(): Message Received: " << messagebuf << std::endl;

  rapidjson::Document document;
  if (document.Parse(messagebuf.c_str()).HasParseError())
  {
    std::cerr << "advertiseServiceCallback(): Error in parsing service request message: " << messagebuf << std::endl;
    return;
  }

  rapidjson::Document values(rapidjson::kObjectType);
  rapidjson::Document::AllocatorType& allocator = values.GetAllocator();
  values.AddMember("success", document["args"]["data"].GetBool(), allocator);
  values.AddMember("message", "from advertiseServiceCallback", allocator);

  rbc.serviceResponse(document["service"].GetString(), document["id"].GetString(), true, values);
}

void callServiceCallback(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::InMessage> in_message)
{
  std::cout << "serviceResponseCallback(): Message Received: " << in_message->string() << std::endl;
  connection->send_close(1000);
}

void publisherThread(RosbridgeWsClient& rbc, const std::future<void>& futureObj)
{
  rbc.addClient("topic_publisher");

  rapidjson::Document d;
  d.SetObject();
  d.AddMember("data", "Test message from /ztopic", d.GetAllocator());

  while (futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
  {
    rbc.publish("/ztopic", d);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  std::cout << "publisherThread stops()" << std::endl;
}

void subscriberCallback(std::shared_ptr<WsClient::Connection> /*connection*/, std::shared_ptr<WsClient::InMessage> in_message)
{
  std::cout << "subscriberCallback(): Message Received: " << in_message->string() << std::endl;
}

void topicsCallback(std::shared_ptr<WsClient::Connection> /*connection*/, std::shared_ptr<WsClient::InMessage> in_message)
{
    std::cout << "subscriberCallback(): Message Received: " << in_message->string() << std::endl;
}

int main() {
//  rbc.addClient("service_advertiser");
//  rbc.callService("/rosapi/topics", &topicsCallback);
//  rbc.advertiseService("service_advertiser", "/zservice", "std_srvs/SetBool", advertiseServiceCallback);
//
//  rbc.addClient("topic_advertiser");
//  rbc.advertise("topic_advertiser", "/ztopic", "std_msgs/String");
//
  rbc.addClient("topic_subscriber");
  rbc.subscribe("topic_subscriber", "/tevel/perception/state", subscriberCallback);
//
//  // Test calling a service
//  rapidjson::Document document(rapidjson::kObjectType);
//  document.AddMember("data", true, document.GetAllocator());
//  rbc.callService("/zservice", callServiceCallback, document);
//
  // Test creating and stopping a publisher
//  {
//    // Create a std::promise object
//    std::promise<void> exitSignal;
//
//    // Fetch std::future object associated with promise
//    std::future<void> futureObj = exitSignal.get_future();
//
//    // Starting Thread & move the future object in lambda function by reference
//    std::thread th(&publisherThread, std::ref(rbc), std::cref(futureObj));
//
//    // Wait for 10 sec
//    std::this_thread::sleep_for(std::chrono::seconds(10));
//
//    std::cout << "Asking publisherThread to Stop" << std::endl;
//
//    // Set the value in promise
//    exitSignal.set_value();
//
//    // Wait for thread to join
//    th.join();
//  }
while (1){
    sleep(1000);
}
//
//  // Test removing clients
//  rbc.removeClient("service_advertiser");
//  rbc.removeClient("topic_advertiser");
//  rbc.removeClient("topic_subscriber");

  std::cout << "Program terminated" << std::endl;
}
