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

namespace PJ {

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
  explicit MessageRef(uint8_t* first_ptr, size_t size) :
    _first_ptr(first_ptr), _size(size)
  { }

  explicit MessageRef(std::vector<uint8_t>& vect) :
    _first_ptr(vect.data()), _size(vect.size())
  { }

  const uint8_t* data() const {
    return _first_ptr;
  }

  uint8_t* data() {
    return _first_ptr;
  }

  size_t size() const {
    return _size;
  }

private:
  uint8_t* _first_ptr;
  size_t _size;
};

/**
 * @brief The MessageParser is the base class to create plugins that are able to parse one or
 * multiple Message types.
 * Each message type is uniquely identified by a MessageKey (128 bits, sufficiently large to
 * hold a MD5Sum identifier).
 *
 * You push one or more raw messages using the method pushMessageRef()
 * Once you have done, the result can be copied using plotData()
 */
class MessageParser
{
public:
  MessageParser(const std::string& topic_name,
                PlotDataMapRef& plot_data): _plot_data(plot_data), _topic_name(topic_name)
  { }
  virtual ~MessageParser() = default;

  virtual bool parseMessage(const MessageRef serialized_msg,
                            double& timestamp) = 0;
protected:

  PlotDataMapRef& _plot_data;
  std::string _topic_name;

  PlotData& getSeries(const std::string& key)
  {
    return _plot_data.getOrCreateNumberic( key );
  }
};

using MessageParserPtr = std::shared_ptr<MessageParser>;

//------------- This is the actual plugin interface --------------
class MessageParserCreator : public PlotJugglerPlugin
{
public:

  virtual MessageParserPtr createInstance(const std::string& topic_name, PlotDataMapRef& data) = 0;
};
//----------------------------------------------------------------

using MessageParserFactory = std::map<QString, std::shared_ptr<MessageParserCreator>>;

} // end namespace


QT_BEGIN_NAMESPACE
#define MessageParserCreator_iid "facontidavide.PlotJuggler3.MessageParserCreator"
Q_DECLARE_INTERFACE(PJ::MessageParserCreator, MessageParserCreator_iid)
QT_END_NAMESPACE
