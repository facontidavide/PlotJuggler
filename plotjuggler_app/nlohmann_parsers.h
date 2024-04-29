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
#include <QSettings>

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
  NlohmannParserCreator(const char* encoding)
  {
    _encoding = encoding;
    _checkbox_use_timestamp = new QCheckBoxClose("use field as timestamp if available");
    loadSettings();
  }

  template <typename ParserT>
  MessageParserPtr createParserImpl(const std::string& topic_name, PlotDataMapRef& data)
  {
    saveSettings();

    std::string timestamp_name = _checkbox_use_timestamp->lineedit->text().toStdString();
    return std::make_shared<ParserT>(
        topic_name, data, _checkbox_use_timestamp->isChecked(), timestamp_name);
  }

  virtual QWidget* optionsWidget()
  {
    loadSettings();

    return _checkbox_use_timestamp;
  }

  const char* encoding() const override
  {
    return _encoding;
  }

  virtual void loadSettings()
  {
    QSettings settings;
    QString prefix = QString("NlohmannParser.") + QString(encoding());
    bool checked = settings.value(prefix + ".timestampEnabled", false).toBool();
    QString field = settings.value(prefix + ".timestampFieldName", "").toString();

    _checkbox_use_timestamp->setChecked(checked);
    _checkbox_use_timestamp->lineedit->setText(field);
  }

  virtual void saveSettings()
  {
    QSettings settings;
    QString prefix = QString("NlohmannParser.") + QString(encoding());
    settings.setValue(prefix + ".timestampEnabled", _checkbox_use_timestamp->isChecked());
    settings.setValue(prefix + ".timestampFieldName",
                      _checkbox_use_timestamp->lineedit->text());
  }

protected:
  QCheckBoxClose* _checkbox_use_timestamp;
  const char* _encoding;
};

class JSON_ParserFactory : public NlohmannParserCreator
{
public:
  JSON_ParserFactory() : NlohmannParserCreator("json")
  {
  }

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
};

class CBOR_ParserFactory : public NlohmannParserCreator
{
public:
  CBOR_ParserFactory() : NlohmannParserCreator("cbor")
  {
  }

  MessageParserPtr createParser(const std::string& topic_name, const std::string&,
                                const std::string&, PlotDataMapRef& data) override
  {
    return createParserImpl<CBOR_Parser>(topic_name, data);
  }
  const char* name() const override
  {
    return "CBOR_ParserFactory";
  }
};

class BSON_ParserFactory : public NlohmannParserCreator
{
public:
  BSON_ParserFactory() : NlohmannParserCreator("bson")
  {
  }

  MessageParserPtr createParser(const std::string& topic_name, const std::string&,
                                const std::string&, PlotDataMapRef& data) override
  {
    return createParserImpl<BSON_Parser>(topic_name, data);
  }
  const char* name() const override
  {
    return "BSON_ParserFactory";
  }
};

class MessagePack_ParserFactory : public NlohmannParserCreator
{
public:
  MessagePack_ParserFactory() : NlohmannParserCreator("msgpack")
  {
  }

  MessageParserPtr createParser(const std::string& topic_name, const std::string&,
                                const std::string&, PlotDataMapRef& data) override
  {
    return createParserImpl<MessagePack_Parser>(topic_name, data);
  }
  const char* name() const override
  {
    return "MessagePack_ParserFactory";
  }
};

#endif  // NLOHMANN_PARSERS_H
