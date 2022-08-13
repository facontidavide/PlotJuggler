/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QtPlugin>
#include <QApplication>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <map>
#include <set>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/pj_plugin.h"

namespace PJ
{
/*
 * A messgaeParser is a clas that is able to convert a message received by
 * a DataStreamer plugin into data in PlotDataMapRef.
 *
 * - Each data Source has its own instance of MessageParser
 * - MessageParser objects are created by MessageParserCreator.
 * - The actual plugin created here is the MessageParserCreator.
 * - Each DataStreamer plugin has its own set of MessageParserCreator
 *
 * */

class MessageRef
{
public:
  explicit MessageRef(const uint8_t* first_ptr, size_t size)
    : _ptr(first_ptr), _size(size)
  {
  }

  explicit MessageRef(const std::byte* first_ptr, size_t size)
    : _ptr(reinterpret_cast<const uint8_t*>(first_ptr)), _size(size)
  {
  }

  explicit MessageRef(const int8_t* first_ptr, size_t size)
    : _ptr(reinterpret_cast<const uint8_t*>(first_ptr)), _size(size)
  {
  }

  template <typename T>
  explicit MessageRef(const std::vector<T>& vect)
    : MessageRef(vect.data(), vect.size())
  {
  }

  const uint8_t* data() const
  {
    return _ptr;
  }

  uint8_t* data() // this is bad and will be removed
  {
    return const_cast<uint8_t*>(_ptr);
  }

  size_t size() const
  {
    return _size;
  }

private:
  const uint8_t* _ptr = nullptr;
  size_t _size = 0;
};

/**
 * @brief The MessageParser is the base class used to parse
 * a message with a specific encoding+schema.
 */
class MessageParser
{
public:
  MessageParser(const std::string& topic_name, PlotDataMapRef& plot_data)
    : _plot_data(plot_data), _topic_name(topic_name)
  {
  }
  virtual ~MessageParser() = default;

  virtual bool parseMessage(const MessageRef serialized_msg, double& timestamp) = 0;

  // Decide what to do if an array is particularly large (size > max_size):
  //
  // if clamp == true, then keep the first max_size elements,
  // otherwise, skip the entire array.
  virtual void setLargeArraysPolicy(bool clamp, unsigned max_size)
  {
    _clamp_large_arrays = clamp;
    _max_array_size = max_size;
  }

  unsigned maxArraySize() const
  {
    return _max_array_size;
  }

  bool clampLargeArray() const
  {
    return _clamp_large_arrays;
  }

protected:
  PlotDataMapRef& _plot_data;
  std::string _topic_name;

  PlotData& getSeries(const std::string& key)
  {
    return _plot_data.getOrCreateNumeric(key);
  }

  StringSeries& getStringSeries(const std::string& key)
  {
    return _plot_data.getOrCreateStringSeries(key);
  }
private:
  bool _clamp_large_arrays = false;
  unsigned _max_array_size = 10000;
};

using MessageParserPtr = std::shared_ptr<MessageParser>;

//------------- This is the actual plugin interface --------------
class ParserFactoryPlugin : public PlotJugglerPlugin
{
public:
  using Ptr = std::shared_ptr<ParserFactoryPlugin>;

  // provide an identifier of the provided encoding.
  // example "ros1", "ros2", "json", "protobuf", etc.
  virtual const char* encoding() const = 0;

  // create an instance of MessageParser, already configured to
  // decode a specific schema.
  virtual MessageParserPtr createParser(const std::string& topic_name,
                                        const std::string& type_name,
                                        const std::string& schema,
                                        PlotDataMapRef& data) = 0;
};

using ParserFactories = std::map<QString, std::shared_ptr<ParserFactoryPlugin>>;


}  // namespace PJ

QT_BEGIN_NAMESPACE
#define ParserFactoryPlugin_iid "facontidavide.PlotJuggler3.ParserFactoryPlugin"
Q_DECLARE_INTERFACE(PJ::ParserFactoryPlugin, ParserFactoryPlugin_iid)
QT_END_NAMESPACE
