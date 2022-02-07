#ifndef NLOHMANN_PARSERS_H
#define NLOHMANN_PARSERS_H

#include "nlohmann/json.hpp"
#include "PlotJuggler/messageparser_base.h"
#include <QDebug>

using namespace PJ;

class NlohmannParser : public MessageParser
{
public:
  NlohmannParser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp, std::string stamp_fieldname)
    : MessageParser(topic_name, data), _use_message_stamp(use_msg_stamp), _stamp_fieldname(stamp_fieldname)
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
  JSON_Parser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp, std::string stamp_fieldname)
    : NlohmannParser(topic_name, data, use_msg_stamp, stamp_fieldname)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class CBOR_Parser : public NlohmannParser
{
public:
  CBOR_Parser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp, std::string stamp_fieldname)
    : NlohmannParser(topic_name, data, use_msg_stamp, stamp_fieldname)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class BSON_Parser : public NlohmannParser
{
public:
  BSON_Parser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp, std::string stamp_fieldname)
    : NlohmannParser(topic_name, data, use_msg_stamp, stamp_fieldname)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override;
};

class MessagePack_Parser : public NlohmannParser
{
public:
  MessagePack_Parser(const std::string& topic_name, PlotDataMapRef& data, bool use_msg_stamp, std::string stamp_fieldname)
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
  QLineEdit *lineedit;
  QGroupBox *groupbox;
  QCheckBoxClose(QString text) : QGroupBox(text)
  {
      QGroupBox::setCheckable(true);
      QGroupBox::setChecked(false);
      lineedit = new QLineEdit(this);
      QVBoxLayout *vbox = new QVBoxLayout;
      vbox->addSpacing(20);
      vbox->addWidget(lineedit);
      QGroupBox::setLayout(vbox);
  }
  ~QCheckBoxClose() override
  {
    qDebug() << "Destroying QCheckBoxClose";
  }

};

class NlohmannParserCreator : public MessageParserCreator
{
public:
  NlohmannParserCreator()
  {
    _checkbox_use_timestamp = new QCheckBoxClose("use field as timestamp if available");

  }

  virtual QWidget* optionsWidget()
  {
    return _checkbox_use_timestamp;
  }

protected:
  QCheckBoxClose* _checkbox_use_timestamp;
};

class JSON_ParserCreator : public NlohmannParserCreator
{
public:
  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override
  {
    return std::make_shared<JSON_Parser>(topic_name, data,
                                         _checkbox_use_timestamp->isChecked(), _checkbox_use_timestamp->lineedit->text().toStdString());
  }
  const char* name() const override
  {
    return "JSON";
  }
};

class CBOR_ParserCreator : public NlohmannParserCreator
{
public:
  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override
  {
    return std::make_shared<CBOR_Parser>(topic_name, data,
                                         _checkbox_use_timestamp->isChecked(), _checkbox_use_timestamp->lineedit->text().toStdString());
  }
  const char* name() const override
  {
    return "CBOR";
  }
};

class BSON_ParserCreator : public NlohmannParserCreator
{
public:
  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override
  {
    return std::make_shared<BSON_Parser>(topic_name, data,
                                         _checkbox_use_timestamp->isChecked(), _checkbox_use_timestamp->lineedit->text().toStdString());
  }
  const char* name() const override
  {
    return "BSON";
  }
};

class MessagePack_ParserCreator : public NlohmannParserCreator
{
public:
  MessageParserPtr createInstance(const std::string& topic_name,
                                  PlotDataMapRef& data) override
  {
    return std::make_shared<MessagePack_Parser>(topic_name, data,
                                                _checkbox_use_timestamp->isChecked(), _checkbox_use_timestamp->lineedit->text().toStdString());
  }
  const char* name() const override
  {
    return "MessagePack";
  }
};

#endif  // NLOHMANN_PARSERS_H
