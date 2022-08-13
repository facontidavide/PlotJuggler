#ifndef PROTOBUF_FACTORY_H
#define PROTOBUF_FACTORY_H

#include "PlotJuggler/messageparser_base.h"

#include "protobuf_parser.h"
#include "ui_protobuf_parser.h"

class ParserFactoryProtobuf : public PJ::ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  ParserFactoryProtobuf();

  ~ParserFactoryProtobuf() override;

  const char* name() const override
  {
    return "ParserFactoryProtobuf";
  }
  const char* encoding() const override
  {
    return "protobuf";
  }

  MessageParserPtr createParser(const std::string& topic_name,
                                const std::string& type_name,
                                const std::string& schema,
                                PlotDataMapRef& data) override;

  QWidget* optionsWidget() override
  {
    return _widget;
  }

protected:

  Ui::ProtobufLoader* ui;
  QWidget* _widget;

  google::protobuf::compiler::DiskSourceTree _source_tree;
  std::unique_ptr<google::protobuf::compiler::Importer> _importer;

  struct FileInfo
  {
    QString file_path;
    QByteArray proto_text;
    const google::protobuf::FileDescriptor* file_descriptor = nullptr;
    std::map<QString,const google::protobuf::Descriptor*> descriptors;
  };
  FileInfo _loaded_file;

  bool updateUI();

  void loadSettings();

  void saveSettings();

  void importFile(QString filename);

private slots:

  void onIncludeDirectory();

  void onLoadFile();

  void onRemoveInclude();

  void onComboChanged(const QString &text);
};


#endif // PROTOBUF_FACTORY_H
