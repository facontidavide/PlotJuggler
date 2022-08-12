/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef NLOHMANN_PARSERS_H
#define NLOHMANN_PARSERS_H

#include "nlohmann/json.hpp"
#include "PlotJuggler/messageparser_base.h"
#include <QDebug>

using namespace PJ;

class NlohmannParser : public MessageParser
{
public:
  NlohmannParser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp,
                 const std::string& stamp_fieldname)
    : MessageParser(topic_name, data)
    , _use_message_stamp(use_msg_stamp)
    , _stamp_fieldname(stamp_fieldname)
  {
  }

protected:
  bool parseMessageImpl(double& timestamp);

  nlohmann::json _json;
  bool _use_message_stamp;
  std::string _stamp_fieldname;
};

class JSON_Parser : public NlohmannParser
{
public:
  JSON_Parser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp,
              const std::string& stamp_fieldname)
    : NlohmannParser(topic_name, data, use_msg_stamp, stamp_fieldname)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class CBOR_Parser : public NlohmannParser
{
public:
  CBOR_Parser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp,
              const std::string& stamp_fieldname)
    : NlohmannParser(topic_name, data, use_msg_stamp, stamp_fieldname)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class BSON_Parser : public NlohmannParser
{
public:
  BSON_Parser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp,
              const std::string& stamp_fieldname)
    : NlohmannParser(topic_name, data, use_msg_stamp, stamp_fieldname)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class MessagePack_Parser : public NlohmannParser
{
public:
  MessagePack_Parser(const std::string& topic_name, PlotDataMapRef& data,
                     bool use_msg_stamp, const std::string& stamp_fieldname)
    : NlohmannParser(topic_name, data, use_msg_stamp, stamp_fieldname)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

//------------------------------------------

#include <QGroupBox>
#include <QVBoxLayout>
#include <QLineEdit>

class QCheckBoxClose : public QGroupBox
{
public:
  QLineEdit* lineedit;
  QGroupBox* groupbox;
  QCheckBoxClose(QString text) : QGroupBox(text)
  {
    QGroupBox::setCheckable(true);
    QGroupBox::setChecked(false);
    lineedit = new QLineEdit(this);
    QVBoxLayout* vbox = new QVBoxLayout;
    vbox->addSpacing(20);
    vbox->addWidget(lineedit);
    QGroupBox::setLayout(vbox);
  }
  ~QCheckBoxClose() override
  {
    qDebug() << "Destroying QCheckBoxClose";
  }
};

class NlohmannParserCreator : public ParserFactoryPlugin
{
public:
  NlohmannParserCreator()
  {
    _checkbox_use_timestamp = new QCheckBoxClose("use field as timestamp if available");
  }

  template <typename ParserT>
  MessageParserPtr createParserImpl(const std::string& topic_name,
                                    PlotDataMapRef& data)
  {
    std::string timestamp_name = _checkbox_use_timestamp->lineedit->text().toStdString();
    return std::make_shared<ParserT>(
      topic_name, data, _checkbox_use_timestamp->isChecked(), timestamp_name);
  }

  virtual QWidget* optionsWidget()
  {
    return _checkbox_use_timestamp;
  }

protected:
  QCheckBoxClose* _checkbox_use_timestamp;
};

class JSON_ParserFactory : public NlohmannParserCreator
{
public:
  MessageParserPtr createParser(const std::string& topic_name,
                                const std::string& /*type_name*/,
                                const std::string& /*schema*/,
                                PlotDataMapRef& data) override
  {
    return createParserImpl<JSON_Parser>(topic_name, data);
  }
  const char* name() const override
  {
    return "JSON_ParserFactory";
  }
  const char* encoding() const override
  {
    return "json";
  }
};

class CBOR_ParserFactory : public NlohmannParserCreator
{
public:
  MessageParserPtr createParser(const std::string& topic_name,
                                const std::string&,
                                const std::string&,
                                PlotDataMapRef& data) override
  {
    return createParserImpl<CBOR_Parser>(topic_name, data);
  }
  const char* name() const override
  {
    return "CBOR_ParserFactory";
  }
  const char* encoding() const override
  {
    return "cbor";
  }
};

class BSON_ParserFactory : public NlohmannParserCreator
{
public:
  MessageParserPtr createParser(const std::string& topic_name,
                                const std::string&,
                                const std::string&,
                                PlotDataMapRef& data) override
  {
    return createParserImpl<BSON_Parser>(topic_name, data);
  }
  const char* name() const override
  {
    return "BSON_ParserFactory";
  }
  const char* encoding() const override
  {
    return "bson";
  }
};

class MessagePack_ParserFactory : public NlohmannParserCreator
{
public:
  MessageParserPtr createParser(const std::string& topic_name,
                                const std::string& ,
                                const std::string& ,
                                PlotDataMapRef& data) override
  {
    return createParserImpl<MessagePack_Parser>(topic_name, data);
  }
  const char* name() const override
  {
    return "MessagePack_ParserFactory";
  }
  const char* encoding() const override
  {
    return "msgpack";
  }
};

#endif  // NLOHMANN_PARSERS_H
