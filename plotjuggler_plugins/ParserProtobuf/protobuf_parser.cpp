#include <QSettings>
#include <QMessageBox>
#include "protobuf_parser.h"
#include "PlotJuggler/fmt/format.h"
#include "PlotJuggler/svg_util.h"


using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

bool ProtobufParser::parseMessage(const MessageRef serialized_msg,
                                  double &timestamp)
{
  const google::protobuf::Message* prototype_msg =
      _msg_factory.GetPrototype(_msg_descriptor);

  google::protobuf::Message* mutable_msg = prototype_msg->New();
  if (!mutable_msg->ParseFromArray(serialized_msg.data(),
                                   serialized_msg.size()))
  {
    return false;
  }

  std::function<void(const google::protobuf::Message&, const std::string&)> ParseImpl;

  ParseImpl = [&](const google::protobuf::Message& msg, const std::string& prefix)
  {
    const Reflection* reflection = msg.GetReflection();
    const Descriptor* descriptor = msg.GetDescriptor();
//    std::vector<const FieldDescriptor*> reflection_fields;
//    reflection->ListFields(msg, &reflection_fields);

    for (int index=0; index < descriptor->field_count(); index++)
    {
      auto field = descriptor->field(index);

      std::string key = prefix.empty() ?
                            field->name():
                            fmt::format("{}/{}", prefix, field->name() );
      std::string suffix;

      if (!field)
      {
        continue;
      }

      int count = 1;
      bool repeated = false;
      if (field->is_repeated())
      {
        count = reflection->FieldSize(msg, field);
        repeated = true;
      }

      for(int index = 0; index < count ; index++)
      {
        if(repeated)
        {
          suffix = fmt::format("[{}]", index);
        }

        bool is_double = true;
        double value = 0;
        switch(field->cpp_type())
        {
          case FieldDescriptor::CPPTYPE_DOUBLE:{
            value = !repeated ? reflection->GetDouble(msg, field) :
                                reflection->GetRepeatedDouble(msg, field, index);
          }break;
          case FieldDescriptor::CPPTYPE_FLOAT:{
            auto tmp = !repeated ? reflection->GetFloat(msg, field) :
                                   reflection->GetRepeatedFloat(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_UINT32:{
            auto tmp = !repeated ? reflection->GetUInt32(msg, field) :
                                   reflection->GetRepeatedUInt32(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_UINT64:{
            auto tmp = !repeated ? reflection->GetUInt64(msg, field) :
                                   reflection->GetRepeatedUInt64(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_BOOL:{
            auto tmp = !repeated ? reflection->GetBool(msg, field) :
                                   reflection->GetRepeatedBool(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_INT32:{
            auto tmp = !repeated ? reflection->GetInt32(msg, field) :
                                   reflection->GetRepeatedInt32(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_INT64:{
            auto tmp = !repeated ? reflection->GetInt64(msg, field) :
                                   reflection->GetRepeatedInt64(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_ENUM:{
            auto tmp = !repeated ? reflection->GetEnum(msg, field) :
                                   reflection->GetRepeatedEnum(msg, field, index);

            auto& series = this->getStringSeries(key + suffix);
            series.pushBack({timestamp, tmp->name()});
            is_double = false;
          }break;
          case FieldDescriptor::CPPTYPE_STRING:{
            auto tmp = !repeated ? reflection->GetString(msg, field) :
                                   reflection->GetRepeatedString(msg, field, index);

            auto& series = this->getStringSeries(key + suffix);
            series.pushBack({timestamp, tmp});
            is_double = false;
          }break;
          case FieldDescriptor::CPPTYPE_MESSAGE:
          {
// Fix macro issue in Windows
#pragma push_macro("GetMessage")
#undef GetMessage
            const auto& new_msg = reflection->GetMessage(msg, field, nullptr);
#pragma pop_macro("GetMessage")

            ParseImpl(new_msg, key + suffix);
            is_double = false;
          }break;
        }

        if( is_double )
        {
          auto& series = this->getSeries(key + suffix);
          series.pushBack({timestamp, value});
        }
      }
    }
  };

  // start recursion
  ParseImpl(*mutable_msg, _topic_name);

  return true;
}

ProtobufParserCreator::ProtobufParserCreator()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::ProtobufLoader;
  ui->setupUi(_widget);

  _source_tree.MapPath("", "");
  _source_tree.MapPath("/", "/");

  loadSettings();

  QSettings settings;
  QString theme = settings.value("Preferences::theme", "light").toString();
  ui->pushButtonRemove->setIcon(LoadSvg(":/resources/svg/trash.svg", theme));

  connect( ui->pushButtonInclude, &QPushButton::clicked, this, &ProtobufParserCreator::onIncludeDirectory);
  connect( ui->pushButtonLoad, &QPushButton::clicked, this, &ProtobufParserCreator::onLoadFile);
  connect( ui->pushButtonRemove, &QPushButton::clicked, this, &ProtobufParserCreator::onRemoveInclude);

  QString last_type = settings.value("ProtobufParserCreator.lastType").toString();
  int combo_index = ui->comboBox->findText(last_type, Qt::MatchExactly);
  if( !last_type.isEmpty() && combo_index != -1)
  {
    ui->comboBox->setCurrentIndex(combo_index);
    onComboChanged(last_type);
  }

  connect( ui->comboBox, qOverload<const QString&>(&QComboBox::currentIndexChanged),
          this, &ProtobufParserCreator::onComboChanged );
}

void ProtobufParserCreator::importFile(QString filename)
{
  QFile file(filename);
  if( !file.exists() )
  {
    QMessageBox::warning(nullptr, tr("Error loading file"),
                         tr("File %1 does not exist").arg(filename),
                         QMessageBox::Cancel);
    return;
  }
  file.open(QIODevice::ReadOnly);
  Info info;
  QFileInfo fileinfo(filename);
  QString file_basename = fileinfo.fileName();
  info.file_path = filename;
  info.proto_text = file.readAll();

  _source_tree.MapPath("", filename.toStdString());
  _source_tree.MapPath("", file_basename.toStdString());
  _source_tree.MapPath("", fileinfo.absolutePath().toStdString());

  _importer.reset( new Importer(&_source_tree, &_error_collector) );
  info.file_descriptor = _importer->Import(file_basename.toStdString());

  if( !info.file_descriptor )
  {
    _error_collector.showErrors();
    return;
  }

  ui->loadedProto->setText(file_basename);
  ui->protoPreview->setText(info.proto_text);
  ui->comboBox->clear();

  for(int i=0; i < info.file_descriptor->message_type_count(); i++)
  {
    const std::string& type_name = info.file_descriptor->message_type(i)->name();
    auto descriptor = info.file_descriptor->FindMessageTypeByName(type_name);
    QString type_qname = QString::fromStdString(type_name);
    info.descriptors.insert({type_qname, descriptor });
    ui->comboBox->addItem(type_qname);
  }
  _loaded_file = std::move(info);
}

void ProtobufParserCreator::loadSettings()
{
  ui->listWidget->clear();
  ui->protoPreview->clear();

  QSettings settings;

  auto include_list = settings.value("ProtobufParserCreator.include_dirs").toStringList();
  for (const auto& include_dir: include_list)
  {
    QDir path_dir(include_dir);
    if (path_dir.exists())
    {
      ui->listWidget->addItem(include_dir);
      _source_tree.MapPath("", include_dir.toStdString());
    }
  }
  ui->listWidget->sortItems();

  auto filename = settings.value("ProtobufParserCreator.protofile").toString();
  if( !filename.isEmpty() ){
    importFile(filename);
  }
}

void ProtobufParserCreator::saveSettings()
{
  QSettings settings;
  QStringList include_list;
  for (int row=0; row<ui->listWidget->count(); row++)
  {
    include_list += ui->listWidget->item(row)->text();
  }
  settings.setValue("ProtobufParserCreator.include_dirs", include_list);
  settings.setValue("ProtobufParserCreator.protofile", _loaded_file.file_path);
}

ProtobufParserCreator::~ProtobufParserCreator()
{
  delete ui;
}

MessageParserPtr ProtobufParserCreator::createInstance(
    const std::string &topic_name, PlotDataMapRef &data)
{
  onComboChanged(ui->comboBox->currentText());
  return std::make_shared<ProtobufParser>(topic_name, data, _selected_descriptor);
}

void ProtobufParserCreator::onIncludeDirectory()
{
  QSettings settings;
  QString directory_path =
      settings.value("ProtobufParserCreator.loadDirectory", QDir::currentPath()).toString();

  QString dirname = QFileDialog::getExistingDirectory(_widget, tr("Load StyleSheet"), directory_path);
  if (dirname.isEmpty())
  {
    return;
  }
  settings.setValue("ProtobufParserCreator.loadDirectory", dirname);
  if( ui->listWidget->findItems(dirname, Qt::MatchExactly).empty() )
  {
    ui->listWidget->addItem( dirname );
    ui->listWidget->sortItems();
  }
}

void ProtobufParserCreator::onLoadFile()
{
  QSettings settings;

  QString directory_path =
      settings.value("ProtobufParserCreator.loadDirectory", QDir::currentPath()).toString();

  QString filename = QFileDialog::getOpenFileName(_widget, tr("Load StyleSheet"),
                                                   directory_path, tr("(*.proto)"));
  if (filename.isEmpty())
  {
    return;
  }

  importFile(filename);

  directory_path = QFileInfo(filename).absolutePath();
  settings.setValue("ProtobufParserCreator.loadDirectory", directory_path);

  saveSettings();
}

void ProtobufParserCreator::onRemoveInclude()
{
  auto selected = ui->listWidget->selectedItems();

  while(!selected.isEmpty())
  {
    auto item = selected.front();
    delete ui->listWidget->takeItem(ui->listWidget->row(item));
    selected.pop_front();
  }
  saveSettings();
  loadSettings();
}


void ProtobufParserCreator::onComboChanged(const QString& text)
{
  auto descr_it = _loaded_file.descriptors.find(text);
  if( descr_it != _loaded_file.descriptors.end())
  {
    _selected_descriptor = descr_it->second;
    QSettings settings;
    settings.setValue("ProtobufParserCreator.lastType", text);
  }
}

void ProtoErrorCollector::AddError(const std::string &filename, int line, int, const std::string &message)
{
  auto msg = QString("Error parsing file: \"%1\"\n"
                     "Line: %2\n"
                     "Message: %3\n\n")
      .arg(QString::fromStdString(filename))
      .arg(line)
      .arg(QString::fromStdString(message));

  _error_msg.append(msg);
}

void ProtoErrorCollector::AddWarning(const std::string &filename, int line, int, const std::string &message)
{
  auto msg = QString("Warning [%1] line %2: %3")
      .arg(QString::fromStdString(filename))
      .arg(line)
      .arg(QString::fromStdString(message));
  qDebug() << msg;
}

void ProtoErrorCollector::showErrors()
{
  QMessageBox::warning(nullptr, "Error parsing Proto file", _error_msg, QMessageBox::Cancel);
  _error_msg.clear();
}
