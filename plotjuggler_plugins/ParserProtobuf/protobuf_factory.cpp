#include "protobuf_factory.h"

#include <QSettings>
#include <QMessageBox>

#include "PlotJuggler/fmt/format.h"
#include "PlotJuggler/svg_util.h"

namespace gp = google::protobuf;


ParserFactoryProtobuf::ParserFactoryProtobuf()
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

  connect( ui->pushButtonInclude, &QPushButton::clicked, this, &ParserFactoryProtobuf::onIncludeDirectory);
  connect( ui->pushButtonLoad, &QPushButton::clicked, this, &ParserFactoryProtobuf::onLoadFile);
  connect( ui->pushButtonRemove, &QPushButton::clicked, this, &ParserFactoryProtobuf::onRemoveInclude);

  QString last_type = settings.value("ProtobufParserCreator.lastType").toString();
  int combo_index = ui->comboBox->findText(last_type, Qt::MatchExactly);
  if( !last_type.isEmpty() && combo_index != -1)
  {
    ui->comboBox->setCurrentIndex(combo_index);
    onComboChanged(last_type);
  }

  connect( ui->comboBox, qOverload<const QString&>(&QComboBox::currentIndexChanged),
          this, &ParserFactoryProtobuf::onComboChanged );
}

void ParserFactoryProtobuf::importFile(QString filename)
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
  FileInfo info;
  QFileInfo fileinfo(filename);
  QString file_basename = fileinfo.fileName();
  info.file_path = filename;
  info.proto_text = file.readAll();

  _source_tree.MapPath("", filename.toStdString());
  _source_tree.MapPath("", file_basename.toStdString());
  _source_tree.MapPath("", fileinfo.absolutePath().toStdString());

  FileErrorCollector error_collector;

  _importer.reset( new gp::compiler::Importer(&_source_tree, &error_collector) );
  info.file_descriptor = _importer->Import(file_basename.toStdString());

  if( !info.file_descriptor )
  {
    if(error_collector.errors().size() > 0)
    {
      QMessageBox::warning(nullptr, "Error parsing Proto file",
                           error_collector.errors().front(),
                           QMessageBox::Cancel);
    }
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

void ParserFactoryProtobuf::loadSettings()
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

void ParserFactoryProtobuf::saveSettings()
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

ParserFactoryProtobuf::~ParserFactoryProtobuf()
{
  delete ui;
}

MessageParserPtr ParserFactoryProtobuf::createParser(const std::string& topic_name,
                                                     const std::string& type_name,
                                                     const std::string& schema,
                                                     PlotDataMapRef& data)
{
  QString descriptor_name = type_name.empty() ?
                              ui->comboBox->currentText() :
                              QString::fromStdString(type_name);
  if( schema.empty() )
  {
    auto descr_it = _loaded_file.descriptors.find(descriptor_name);
    if( descr_it == _loaded_file.descriptors.end())
    {
      throw std::runtime_error("ParserFactoryProtobuf: can't find the descriptor");
    }
    auto selected_descriptor = descr_it->second;
    return std::make_shared<ProtobufParser>(topic_name, selected_descriptor, data );
  }
  else
  {
    gp::FileDescriptorSet field_set;
    if (!field_set.ParseFromArray(schema.data(), schema.size()))
    {
      throw std::runtime_error("failed to parse schema data");
    }

    return std::make_shared<ProtobufParser>(topic_name, type_name, field_set, data );
  }
}

void ParserFactoryProtobuf::onIncludeDirectory()
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

void ParserFactoryProtobuf::onLoadFile()
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

void ParserFactoryProtobuf::onRemoveInclude()
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


void ParserFactoryProtobuf::onComboChanged(const QString& text)
{
  auto descr_it = _loaded_file.descriptors.find(text);
  if( descr_it != _loaded_file.descriptors.end())
  {
    QSettings settings;
    settings.setValue("ProtobufParserCreator.lastType", text);
  }
}



