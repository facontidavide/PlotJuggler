#ifndef PROTOBUF_FACTORY_H
#define PROTOBUF_FACTORY_H

#include <mutex>
#include <set>

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

  MessageParserPtr createParser(const std::string& topic_name, const std::string& type_name,
                                const std::string& schema, PlotDataMapRef& data) override;

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
    QString file_basename;
    QByteArray proto_text;
    const google::protobuf::FileDescriptor* file_descriptor = nullptr;
    std::map<QString, const google::protobuf::Descriptor*> descriptors;
  };

  // Multiple proto files can be loaded simultaneously. Keyed by basename.
  std::map<QString, FileInfo> _loaded_files;

  // Topic names observed in createParser() calls. Used to populate the Topic
  // column's ComboBox so users can pick known topics instead of retyping them.
  // Written from the streamer thread, read from the UI thread.
  std::set<QString> _seen_topics;
  mutable std::mutex _seen_topics_mutex;

  bool importFile(const QString& filename);

  void loadSettings();

  void saveSettings();

  void rebuildTypeComboBox();

  // Parse a qualified type string "<basename>::<type>" or unqualified "<type>";
  // returns nullptr if not found.
  const google::protobuf::Descriptor* findDescriptor(const QString& qualified_type) const;

  // Add a row to the topic-mapping table with the given topic and (optional)
  // selected qualified type. Wires a QComboBox into column 1.
  void addMappingRow(const QString& topic = QString(), const QString& selected_type = QString());

private slots:

  void onIncludeDirectory();

  void onLoadFile();

  void onRemoveInclude();

  void onRemoveFile();

  void onLoadedFilesSelectionChanged();

  void onComboChanged(const QString& text);

  void onAddMapping();

  void onRemoveMapping();

  void onTopicMappingChanged();
};

#endif  // PROTOBUF_FACTORY_H
