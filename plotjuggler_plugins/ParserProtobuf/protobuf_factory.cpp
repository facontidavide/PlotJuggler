#include "protobuf_factory.h"

#include <QComboBox>
#include <QCompleter>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QSettings>
#include <QStringListModel>
#include <QTableWidgetItem>

#include "fmt/format.h"
#include "PlotJuggler/svg_util.h"

namespace gp = google::protobuf;

static QString qualifiedName(const QString& basename, const QString& type_name)
{
  return QString("%1::%2").arg(basename, type_name);
}

ParserFactoryProtobuf::ParserFactoryProtobuf()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::ProtobufLoader;
  ui->setupUi(_widget);

  _source_tree.MapPath("", "");
  _source_tree.MapPath("/", "/");

  // Column sizing for the topic-mapping table.
  ui->tableTopicMapping->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->tableTopicMapping->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

  loadSettings();

  QSettings settings;
  QString theme = settings.value("Preferences::theme", "light").toString();
  ui->pushButtonRemove->setIcon(LoadSvg(":/resources/svg/trash.svg", theme));

  connect(ui->pushButtonInclude, &QPushButton::clicked, this,
          &ParserFactoryProtobuf::onIncludeDirectory);
  connect(ui->pushButtonLoad, &QPushButton::clicked, this, &ParserFactoryProtobuf::onLoadFile);
  connect(ui->pushButtonRemove, &QPushButton::clicked, this,
          &ParserFactoryProtobuf::onRemoveInclude);
  connect(ui->pushButtonRemoveFile, &QPushButton::clicked, this,
          &ParserFactoryProtobuf::onRemoveFile);
  connect(ui->listLoadedFiles, &QListWidget::itemSelectionChanged, this,
          &ParserFactoryProtobuf::onLoadedFilesSelectionChanged);

  connect(ui->pushButtonAddMapping, &QPushButton::clicked, this,
          &ParserFactoryProtobuf::onAddMapping);
  connect(ui->pushButtonRemoveMapping, &QPushButton::clicked, this,
          &ParserFactoryProtobuf::onRemoveMapping);

  QString last_type = settings.value("ProtobufParserCreator.lastType").toString();
  int combo_index = ui->comboBox->findText(last_type, Qt::MatchExactly);
  if (!last_type.isEmpty() && combo_index != -1)
  {
    ui->comboBox->setCurrentIndex(combo_index);
  }

  connect(ui->comboBox, qOverload<const QString&>(&QComboBox::currentIndexChanged), this,
          &ParserFactoryProtobuf::onComboChanged);
}

bool ParserFactoryProtobuf::importFile(const QString& filename)
{
  QFile file(filename);
  if (!file.exists())
  {
    QMessageBox::warning(nullptr, tr("Error loading file"),
                         tr("File %1 does not exist").arg(filename), QMessageBox::Cancel);
    return false;
  }
  file.open(QIODevice::ReadOnly);

  FileInfo info;
  QFileInfo fileinfo(filename);
  info.file_path = filename;
  info.file_basename = fileinfo.fileName();
  info.proto_text = file.readAll();

  _source_tree.MapPath("", filename.toStdString());
  _source_tree.MapPath("", info.file_basename.toStdString());
  _source_tree.MapPath("", fileinfo.absolutePath().toStdString());

  // Rebuild a fresh Importer holding every currently-loaded file plus this one;
  // DescriptorPool doesn't support removal, so we pay this cost on every add/remove.
  FileErrorCollector error_collector;
  _importer.reset(new gp::compiler::Importer(&_source_tree, &error_collector));

  for (auto& [basename, existing] : _loaded_files)
  {
    existing.file_descriptor = _importer->Import(existing.file_basename.toStdString());
    existing.descriptors.clear();
    if (existing.file_descriptor)
    {
      for (int i = 0; i < existing.file_descriptor->message_type_count(); i++)
      {
        const auto* d = existing.file_descriptor->message_type(i);
        existing.descriptors.insert({ QString::fromStdString(std::string(d->name())), d });
      }
    }
  }

  info.file_descriptor = _importer->Import(info.file_basename.toStdString());
  if (!info.file_descriptor)
  {
    if (!error_collector.errors().empty())
    {
      QMessageBox::warning(nullptr, "Error parsing Proto file", error_collector.errors().front(),
                           QMessageBox::Cancel);
    }
    return false;
  }
  for (int i = 0; i < info.file_descriptor->message_type_count(); i++)
  {
    const auto* d = info.file_descriptor->message_type(i);
    info.descriptors.insert({ QString::fromStdString(std::string(d->name())), d });
  }

  _loaded_files[info.file_basename] = std::move(info);

  // Refresh the list widget + the default combo + any per-row combos.
  ui->listLoadedFiles->blockSignals(true);
  ui->listLoadedFiles->clear();
  for (const auto& [basename, fi] : _loaded_files)
  {
    ui->listLoadedFiles->addItem(fi.file_basename);
  }
  ui->listLoadedFiles->blockSignals(false);

  rebuildTypeComboBox();
  return true;
}

void ParserFactoryProtobuf::rebuildTypeComboBox()
{
  // Default combo
  QString prev_default = ui->comboBox->currentText();
  ui->comboBox->blockSignals(true);
  ui->comboBox->clear();
  for (const auto& [basename, fi] : _loaded_files)
  {
    for (const auto& [type_name, descr] : fi.descriptors)
    {
      ui->comboBox->addItem(qualifiedName(basename, type_name));
    }
  }
  int idx = ui->comboBox->findText(prev_default);
  if (idx >= 0)
  {
    ui->comboBox->setCurrentIndex(idx);
  }
  ui->comboBox->blockSignals(false);

  // Per-row combos in the mapping table
  for (int row = 0; row < ui->tableTopicMapping->rowCount(); row++)
  {
    auto* combo = qobject_cast<QComboBox*>(ui->tableTopicMapping->cellWidget(row, 1));
    if (!combo)
    {
      continue;
    }
    QString prev = combo->currentText();
    combo->blockSignals(true);
    combo->clear();
    for (const auto& [basename, fi] : _loaded_files)
    {
      for (const auto& [type_name, descr] : fi.descriptors)
      {
        combo->addItem(qualifiedName(basename, type_name));
      }
    }
    int pidx = combo->findText(prev);
    combo->setCurrentIndex(pidx >= 0 ? pidx : -1);
    combo->blockSignals(false);
  }
}

const gp::Descriptor* ParserFactoryProtobuf::findDescriptor(const QString& qualified_type) const
{
  if (qualified_type.isEmpty())
  {
    return nullptr;
  }
  // Qualified form: "<basename>::<type>"
  int sep = qualified_type.indexOf("::");
  if (sep >= 0)
  {
    QString basename = qualified_type.left(sep);
    QString type_name = qualified_type.mid(sep + 2);
    auto file_it = _loaded_files.find(basename);
    if (file_it == _loaded_files.end())
    {
      return nullptr;
    }
    auto descr_it = file_it->second.descriptors.find(type_name);
    return descr_it != file_it->second.descriptors.end() ? descr_it->second : nullptr;
  }
  // Unqualified: search every loaded file (first match wins).
  for (const auto& [basename, fi] : _loaded_files)
  {
    auto descr_it = fi.descriptors.find(qualified_type);
    if (descr_it != fi.descriptors.end())
    {
      return descr_it->second;
    }
  }
  return nullptr;
}

void ParserFactoryProtobuf::loadSettings()
{
  ui->listWidget->clear();
  ui->listLoadedFiles->clear();
  ui->protoPreview->clear();

  QSettings settings;

  bool use_timestamp = settings.value("ProtobufParserCreator.useTimestamp", false).toBool();
  ui->checkBoxTimestamp->setChecked(use_timestamp);

  auto include_list = settings.value("ProtobufParserCreator.include_dirs").toStringList();
  for (const auto& include_dir : include_list)
  {
    QDir path_dir(include_dir);
    if (path_dir.exists())
    {
      ui->listWidget->addItem(include_dir);
      _source_tree.MapPath("", include_dir.toStdString());
    }
  }
  ui->listWidget->sortItems();

  // Preferred new key: plural list of proto files. Fall back to legacy single
  // "ProtobufParserCreator.protofile" so existing user settings keep working.
  QStringList proto_files = settings.value("ProtobufParserCreator.protofiles").toStringList();
  if (proto_files.isEmpty())
  {
    QString legacy = settings.value("ProtobufParserCreator.protofile").toString();
    if (!legacy.isEmpty())
    {
      proto_files << legacy;
    }
  }
  for (const auto& f : proto_files)
  {
    importFile(f);
  }

  // Restore topic mappings: QStringList where each entry is "topic||qualified_type".
  auto saved_mappings = settings.value("ProtobufParserCreator.topicMappings").toStringList();
  // Restore the set of previously-seen topic names.
  {
    std::lock_guard<std::mutex> lock(_seen_topics_mutex);
    _seen_topics.clear();
    for (const auto& t : settings.value("ProtobufParserCreator.seenTopics").toStringList())
    {
      _seen_topics.insert(t);
    }
  }

  ui->tableTopicMapping->blockSignals(true);
  ui->tableTopicMapping->setRowCount(0);
  for (const auto& entry : saved_mappings)
  {
    int sep = entry.indexOf("||");
    if (sep < 0)
    {
      continue;
    }
    addMappingRow(entry.left(sep), entry.mid(sep + 2));
  }
  ui->tableTopicMapping->blockSignals(false);
}

void ParserFactoryProtobuf::saveSettings()
{
  QSettings settings;

  QStringList include_list;
  for (int row = 0; row < ui->listWidget->count(); row++)
  {
    include_list += ui->listWidget->item(row)->text();
  }
  settings.setValue("ProtobufParserCreator.include_dirs", include_list);

  QStringList proto_files;
  for (const auto& [basename, fi] : _loaded_files)
  {
    proto_files << fi.file_path;
  }
  settings.setValue("ProtobufParserCreator.protofiles", proto_files);
  settings.remove("ProtobufParserCreator.protofile");  // legacy key

  settings.setValue("ProtobufParserCreator.useTimestamp", ui->checkBoxTimestamp->isChecked());

  QStringList mappings;
  for (int row = 0; row < ui->tableTopicMapping->rowCount(); row++)
  {
    auto* topic_edit = qobject_cast<QLineEdit*>(ui->tableTopicMapping->cellWidget(row, 0));
    auto* type_combo = qobject_cast<QComboBox*>(ui->tableTopicMapping->cellWidget(row, 1));
    if (!topic_edit || !type_combo)
    {
      continue;
    }
    QString topic = topic_edit->text();
    QString type = type_combo->currentText();
    if (topic.isEmpty() || type.isEmpty())
    {
      continue;
    }
    mappings << QString("%1||%2").arg(topic, type);
  }
  settings.setValue("ProtobufParserCreator.topicMappings", mappings);

  QStringList seen_list;
  {
    std::lock_guard<std::mutex> lock(_seen_topics_mutex);
    for (const auto& t : _seen_topics)
    {
      seen_list << t;
    }
  }
  settings.setValue("ProtobufParserCreator.seenTopics", seen_list);
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
  // Remember this topic so future Topic-Mapping ComboBox rows can offer it as
  // a pick. Persist once on first observation; QSettings writes are thread-safe.
  if (!topic_name.empty())
  {
    QString topic_q = QString::fromStdString(topic_name);
    bool added = false;
    QStringList snapshot;
    {
      std::lock_guard<std::mutex> lock(_seen_topics_mutex);
      added = _seen_topics.insert(topic_q).second;
      if (added)
      {
        for (const auto& t : _seen_topics)
        {
          snapshot << t;
        }
      }
    }
    if (added)
    {
      QSettings settings;
      settings.setValue("ProtobufParserCreator.seenTopics", snapshot);
    }
  }

  if (schema.empty())
  {
    QString chosen;
    // 1. explicit override from the streamer (e.g. MCAP/Foxglove)
    if (!type_name.empty())
    {
      chosen = QString::fromStdString(type_name);
    }
    // 2. topic-mapping table
    if (chosen.isEmpty() && !topic_name.empty())
    {
      QString topic_q = QString::fromStdString(topic_name);
      for (int row = 0; row < ui->tableTopicMapping->rowCount(); row++)
      {
        auto* topic_edit = qobject_cast<QLineEdit*>(ui->tableTopicMapping->cellWidget(row, 0));
        auto* type_combo = qobject_cast<QComboBox*>(ui->tableTopicMapping->cellWidget(row, 1));
        if (!topic_edit || !type_combo)
        {
          continue;
        }
        if (topic_edit->text() == topic_q)
        {
          chosen = type_combo->currentText();
          break;
        }
      }
    }
    // 3. fall back to the default combo-box selection
    if (chosen.isEmpty())
    {
      chosen = ui->comboBox->currentText();
    }

    auto descriptor = findDescriptor(chosen);
    if (!descriptor)
    {
      throw std::runtime_error(
          fmt::format("ParserFactoryProtobuf: can't find descriptor '{}'", chosen.toStdString()));
    }
    auto parser = std::make_shared<ProtobufParser>(topic_name, descriptor, data);
    parser->enableEmbeddedTimestamp(ui->checkBoxTimestamp->isChecked());
    return parser;
  }
  else
  {
    gp::FileDescriptorSet field_set;
    if (!field_set.ParseFromArray(schema.data(), schema.size()))
    {
      throw std::runtime_error("failed to parse schema data");
    }

    auto parser = std::make_shared<ProtobufParser>(topic_name, type_name, field_set, data);
    parser->enableEmbeddedTimestamp(ui->checkBoxTimestamp->isChecked());
    return parser;
  }
}

void ParserFactoryProtobuf::onIncludeDirectory()
{
  QSettings settings;
  QString directory_path =
      settings.value("ProtobufParserCreator.loadDirectory", QDir::currentPath()).toString();

  QString dirname =
      QFileDialog::getExistingDirectory(_widget, tr("Add Include Folder"), directory_path);
  if (dirname.isEmpty())
  {
    return;
  }
  settings.setValue("ProtobufParserCreator.loadDirectory", dirname);
  if (ui->listWidget->findItems(dirname, Qt::MatchExactly).empty())
  {
    ui->listWidget->addItem(dirname);
    ui->listWidget->sortItems();
    saveSettings();
  }
}

void ParserFactoryProtobuf::onLoadFile()
{
  QSettings settings;

  QString directory_path =
      settings.value("ProtobufParserCreator.loadDirectory", QDir::currentPath()).toString();

  QString filename = QFileDialog::getOpenFileName(_widget, tr("Load .proto file"), directory_path,
                                                  tr("(*.proto)"));
  if (filename.isEmpty())
  {
    return;
  }

  if (!importFile(filename))
  {
    return;
  }

  directory_path = QFileInfo(filename).absolutePath();
  settings.setValue("ProtobufParserCreator.loadDirectory", directory_path);

  saveSettings();
}

void ParserFactoryProtobuf::onRemoveInclude()
{
  auto selected = ui->listWidget->selectedItems();

  while (!selected.isEmpty())
  {
    auto item = selected.front();
    delete ui->listWidget->takeItem(ui->listWidget->row(item));
    selected.pop_front();
  }
  saveSettings();
}

void ParserFactoryProtobuf::onRemoveFile()
{
  auto selected = ui->listLoadedFiles->selectedItems();
  if (selected.isEmpty())
  {
    return;
  }

  for (auto* item : selected)
  {
    _loaded_files.erase(item->text());
  }

  // Re-import the remaining files into a fresh Importer.
  FileErrorCollector error_collector;
  _importer.reset(new gp::compiler::Importer(&_source_tree, &error_collector));
  for (auto& [basename, fi] : _loaded_files)
  {
    fi.file_descriptor = _importer->Import(fi.file_basename.toStdString());
    fi.descriptors.clear();
    if (fi.file_descriptor)
    {
      for (int i = 0; i < fi.file_descriptor->message_type_count(); i++)
      {
        const auto* d = fi.file_descriptor->message_type(i);
        fi.descriptors.insert({ QString::fromStdString(std::string(d->name())), d });
      }
    }
  }

  ui->listLoadedFiles->blockSignals(true);
  ui->listLoadedFiles->clear();
  for (const auto& [basename, fi] : _loaded_files)
  {
    ui->listLoadedFiles->addItem(fi.file_basename);
  }
  ui->listLoadedFiles->blockSignals(false);

  ui->protoPreview->clear();
  rebuildTypeComboBox();
  saveSettings();
}

void ParserFactoryProtobuf::onLoadedFilesSelectionChanged()
{
  auto selected = ui->listLoadedFiles->selectedItems();
  if (selected.isEmpty())
  {
    ui->protoPreview->clear();
    return;
  }
  auto it = _loaded_files.find(selected.front()->text());
  if (it != _loaded_files.end())
  {
    ui->protoPreview->setText(QString::fromUtf8(it->second.proto_text));
  }
}

void ParserFactoryProtobuf::onComboChanged(const QString& text)
{
  QSettings settings;
  settings.setValue("ProtobufParserCreator.lastType", text);
}

void ParserFactoryProtobuf::addMappingRow(const QString& topic, const QString& selected_type)
{
  int row = ui->tableTopicMapping->rowCount();
  ui->tableTopicMapping->insertRow(row);

  // Column 0: QLineEdit with a QCompleter. Suggestions come from two sources:
  //   1. Topics observed via createParser() in past sessions (persisted).
  //   2. Items in any sibling QListWidget of the parent dialog — the MQTT/ZMQ
  //      connection dialog typically shows its discovered topic list there,
  //      so this also catches topics in the CURRENT (not-yet-OK'd) session.
  auto* topic_edit = new QLineEdit();
  topic_edit->setPlaceholderText(tr("Type a topic (autocomplete from connection / past sessions)"));
  QStringList seen_list;
  {
    std::lock_guard<std::mutex> lock(_seen_topics_mutex);
    for (const auto& t : _seen_topics)
    {
      seen_list << t;
    }
  }
  // Walk up to the top-level window and scan every QListWidget we're not
  // responsible for. Items become suggestion candidates.
  QWidget* top = _widget;
  while (top->parentWidget())
  {
    top = top->parentWidget();
  }
  for (QListWidget* lw : top->findChildren<QListWidget*>())
  {
    if (lw == ui->listLoadedFiles || lw == ui->listWidget)
    {
      continue;
    }
    for (int i = 0; i < lw->count(); i++)
    {
      QString item_text = lw->item(i)->text();
      if (!item_text.isEmpty() && !seen_list.contains(item_text))
      {
        seen_list << item_text;
      }
    }
  }
  auto* completer = new QCompleter(seen_list, topic_edit);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  completer->setFilterMode(Qt::MatchContains);
  completer->setCompletionMode(QCompleter::PopupCompletion);
  topic_edit->setCompleter(completer);
  if (!topic.isEmpty())
  {
    topic_edit->setText(topic);
  }
  connect(topic_edit, &QLineEdit::textChanged, this, &ParserFactoryProtobuf::onTopicMappingChanged);
  ui->tableTopicMapping->setCellWidget(row, 0, topic_edit);

  // Column 1: non-editable ComboBox of all qualified types from loaded files.
  auto* type_combo = new QComboBox();
  for (const auto& [basename, fi] : _loaded_files)
  {
    for (const auto& [type_name, descr] : fi.descriptors)
    {
      type_combo->addItem(qualifiedName(basename, type_name));
    }
  }
  if (!selected_type.isEmpty())
  {
    int idx = type_combo->findText(selected_type);
    if (idx >= 0)
    {
      type_combo->setCurrentIndex(idx);
    }
  }
  connect(type_combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &ParserFactoryProtobuf::onTopicMappingChanged);
  ui->tableTopicMapping->setCellWidget(row, 1, type_combo);
}

void ParserFactoryProtobuf::onAddMapping()
{
  addMappingRow();
  saveSettings();
}

void ParserFactoryProtobuf::onRemoveMapping()
{
  auto selected_rows = ui->tableTopicMapping->selectionModel()->selectedRows();
  std::vector<int> rows;
  rows.reserve(selected_rows.size());
  for (const auto& idx : selected_rows)
  {
    rows.push_back(idx.row());
  }
  std::sort(rows.begin(), rows.end(), std::greater<int>());
  for (int r : rows)
  {
    ui->tableTopicMapping->removeRow(r);
  }
  saveSettings();
}

void ParserFactoryProtobuf::onTopicMappingChanged()
{
  saveSettings();
}
