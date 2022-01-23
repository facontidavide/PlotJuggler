#include "lua_editor.h"
#include "ui_lua_editor.h"
#include <QSettings>
#include <QPushButton>
#include <QLineEdit>
#include <QSettings>
#include <memory>
#include <QWheelEvent>
#include <QMessageBox>

#include "PlotJuggler/reactive_function.h"
#include "PlotJuggler/svg_util.h"

ToolboxLuaEditor::ToolboxLuaEditor()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::LuaEditor;
  ui->setupUi(_widget);

  QString library_default_code = ui->textLibrary->toPlainText();

  ui->textGlobal->installEventFilter(this);
  ui->textFunction->installEventFilter(this);
  ui->textLibrary->installEventFilter(this);

  ui->labelSemaphore->setToolTipDuration(5000);

  ui->textGlobal->setAcceptDrops(true);
  ui->textFunction->setAcceptDrops(true);

  connect(ui->pushButtonSave, &QPushButton::clicked, this, &ToolboxLuaEditor::onSave);

  connect(ui->pushButtonDelete, &QPushButton::clicked, this, &ToolboxLuaEditor::onDelete);

  connect(ui->lineEditFunctionName, &QLineEdit::textChanged, this, [this]()
          {
            bool has_name = ui->lineEditFunctionName->text().isEmpty() == false;
            ui->pushButtonSave->setEnabled( has_name );
          } );

  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ToolboxPlugin::closed);

  connect(ui->listWidgetRecent, &QListWidget::doubleClicked, this, &ToolboxLuaEditor::restoreRecent);

  connect(ui->listWidgetFunctions, &QListWidget::doubleClicked, this, &ToolboxLuaEditor::restoreFunction);

  connect(ui->listWidgetFunctions, &QListWidget::itemSelectionChanged, this,
          [this]() {
            auto selected = ui->listWidgetFunctions->selectedItems();
            ui->pushButtonDelete->setEnabled(selected.size()>0);
          });

  _delay_library_check.connectCallback([this]() {
    onLibraryUpdated();
  } );

  connect(ui->textLibrary, &QPlainTextEdit::textChanged,
          [this]() {
            _delay_library_check.triggerSignal(250); }
          );

  connect(ui->pushButtonDefaultLibrary, &QPushButton::clicked,
          [=]() { ui->textLibrary->setPlainText( library_default_code ); } );

  connect(ui->pushButtonApplyLibrary, &QPushButton::clicked,
          this, &ToolboxLuaEditor::onReloadLibrary);

  _global_highlighter = new LuaHighlighter( ui->textGlobal->document() );
  _function_highlighter = new LuaHighlighter( ui->textFunction->document() );
  _helper_highlighter = new LuaHighlighter( ui->textLibrary->document() );

  // restore recent functions
  QSettings settings;
  auto previous_functions = settings.value("ToolboxLuaEditor/recent_functions", "").toString();
  if( previous_functions.isEmpty() == false)
  {
    QDomDocument xml_doc;
    if(xml_doc.setContent(previous_functions))
    {
      auto root = xml_doc.firstChild();
      for(auto elem = root.firstChildElement("function"); !elem.isNull();
          elem = elem.nextSiblingElement("function") )
      {
        auto name = elem.attribute("name");
        auto item = new QListWidgetItem(name);
        setItemData(item, name, elem.attribute("global"), elem.attribute("function"));
        ui->listWidgetRecent->addItem(item);
      }
    }
  }

  if( settings.contains("ToolboxLuaEditor/library"))
  {
    QString code = settings.value("ToolboxLuaEditor/library").toString();
    ui->textLibrary->setPlainText(code);
  }
  onLibraryUpdated();

  _previous_library = ui->textLibrary->toPlainText();
}

ToolboxLuaEditor::~ToolboxLuaEditor()
{
  delete ui;
}

const char *ToolboxLuaEditor::name() const
{
  return "Reactive Script Editor";
}

void ToolboxLuaEditor::init(PlotDataMapRef &src_data,
                            TransformsMap &transform_map)
{
  _plot_data = &src_data;
  _transforms = &transform_map;

}

std::pair<QWidget *, ToolboxPlugin::WidgetType>
ToolboxLuaEditor::providedWidget() const
{
  return { _widget, PJ::ToolboxPlugin::FIXED };
}

bool ToolboxLuaEditor::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
  if( ui->listWidgetFunctions->count() > 0 )
  {
    QString msg = "Do you want to save the current active scripts?\n\n";

    for(int row = 0; row < ui->listWidgetFunctions->count(); row++)
    {
      auto item = ui->listWidgetFunctions->item(row);
      msg += QString("  - %1\n").arg(item->text());
    }

    auto ret = QMessageBox::question(nullptr, this->name(), msg);
    if( ret == QMessageBox::No )
    {
      return false;
    }
  }

  auto library_elem = doc.createElement("library");
  library_elem.setAttribute("code", ui->textLibrary->toPlainText());
  parent_element.appendChild(library_elem);

  auto scripts_elem = doc.createElement("scripts");

  for(int row = 0; row < ui->listWidgetFunctions->count(); row++)
  {
    auto item = ui->listWidgetFunctions->item(row);
    auto fields = getItemData(item);
    auto elem = doc.createElement("script");
    elem.setAttribute("name", fields.name);
    elem.setAttribute("function", fields.function_code);
    elem.setAttribute("global", fields.global_code);
    scripts_elem.appendChild(elem);
  }
  parent_element.appendChild(scripts_elem);

  return true;
}

bool ToolboxLuaEditor::xmlLoadState(const QDomElement &parent_element)
{
  auto library_elem = parent_element.firstChildElement("library");
  if(!library_elem.isNull())
  {
    ui->textLibrary->setPlainText( library_elem.attribute("code") );
  }

  auto scripts_elem = parent_element.firstChildElement("scripts");
  if(!scripts_elem.isNull())
  {
    for (auto elem = scripts_elem.firstChildElement("script");
         elem.isNull() == false; elem = elem.nextSiblingElement("script"))
    {
      ui->listWidgetFunctions->clear();
      QString name = elem.attribute("name");
      QString function = elem.attribute("function");
      QString global = elem.attribute("global");
      auto item = new QListWidgetItem(name);
      setItemData(item, name, global, function);
      ui->listWidgetFunctions->addItem(item);

      auto lua_function = std::make_shared<ReactiveLuaFunction>(
          _plot_data, global, function, ui->textLibrary->toPlainText() );

      (*_transforms)[name.toStdString()] = lua_function;
    }
    ui->listWidgetFunctions->sortItems();
  }

  return true;
}

bool ToolboxLuaEditor::onShowWidget()
{
  ui->listWidgetFunctions->clear();

  // check the already existing functions.
  for(auto it : *_transforms)
  {
    if( auto lua_function = std::dynamic_pointer_cast<ReactiveLuaFunction>( it.second ))
    {
      QString name = QString::fromStdString(it.first);
      auto item = new QListWidgetItem(name);
      setItemData(item, name, lua_function->getGlobalCode(), lua_function->getFunctionCode());
      ui->listWidgetFunctions->addItem(item);
    }
    ui->listWidgetFunctions->sortItems();
  }

  QSettings settings;
  QString theme = settings.value("StyleSheet::theme", "light").toString();

  ui->pushButtonDelete->setIcon(LoadSvg(":/resources/svg/clear.svg", theme));

  _font_size = settings.value("ToolboxLuaEditor/fonts_size", 12).toInt();

  QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  fixedFont.setPointSize(_font_size);

  ui->textGlobal->setFont( fixedFont );
  ui->textFunction->setFont( fixedFont );
  ui->textLibrary->setFont( fixedFont );

  return true;
}

void ToolboxLuaEditor::onSave()
{
  auto name = ui->lineEditFunctionName->text();
  if(ui->listWidgetFunctions->findItems(name, Qt::MatchExactly).size()>0)
  {
    QMessageBox msgBox(_widget);
    msgBox.setWindowTitle("Warning");
    msgBox.setText(tr("A dfunction with the same name exists already.\n"
                      " Do you want to overwrite it?\n"));
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton* button = msgBox.addButton(tr("Overwrite"), QMessageBox::YesRole);
    msgBox.setDefaultButton(button);

    int res = msgBox.exec();
    if (res < 0 || res == QMessageBox::Cancel)
    {
      return;
    }
  }

  try {
    auto lua_function = std::make_shared<ReactiveLuaFunction>(
        _plot_data,
        ui->textGlobal->toPlainText(),
        ui->textFunction->toPlainText(),
        ui->textLibrary->toPlainText() );

    (*_transforms)[name.toStdString()] = lua_function;

    if( ui->listWidgetFunctions->findItems(name, Qt::MatchExactly).empty() )
    {
      ui->listWidgetFunctions->addItem(name);
      ui->listWidgetFunctions->sortItems();
    }

    auto item = ui->listWidgetFunctions->findItems(name, Qt::MatchExactly).first();
    setItemData(item, name, ui->textGlobal->toPlainText(), ui->textFunction->toPlainText());

    for( auto& new_name: lua_function->createdCurves() )
    {
      emit plotCreated(new_name);
    }
  }
  catch(std::runtime_error& err)
  {
    QMessageBox::warning(nullptr, "Error in Lua code",  QString(err.what()), QMessageBox::Cancel);
  }

  auto prev_items = ui->listWidgetRecent->findItems(name, Qt::MatchExactly);
  // new name, delete oldest and append
  if( prev_items.empty() )
  {
    while( ui->listWidgetRecent->count() >= 10 )
    {
      delete ui->listWidgetRecent->takeItem(0);
    }
  }
  else{
    // overwrite item with same name
    auto row = ui->listWidgetRecent->row( prev_items.first() );
    delete ui->listWidgetRecent->takeItem(row);
  }

  // save recent functions
  auto new_item = new QListWidgetItem(name);
  setItemData(new_item, name, ui->textGlobal->toPlainText(), ui->textFunction->toPlainText());
  ui->listWidgetRecent->addItem(new_item);

  QDomDocument xml_doc;
  auto root = xml_doc.createElement("functions");

  for(int row = 0; row < ui->listWidgetRecent->count(); row++)
  {
    auto item = ui->listWidgetRecent->item(row);
    auto fields = getItemData(item);

    auto elem = xml_doc.createElement("function");
    elem.setAttribute("name", fields.name);
    elem.setAttribute("global", fields.global_code);
    elem.setAttribute("function", fields.function_code);
    root.appendChild(elem);
  }
  xml_doc.appendChild(root);

  QSettings settings;
  settings.setValue("ToolboxLuaEditor/recent_functions", xml_doc.toString());
}

void ToolboxLuaEditor::onDelete()
{
  for(auto item: ui->listWidgetFunctions->selectedItems())
  {
    _transforms->erase(item->text().toStdString());

    int row = ui->listWidgetFunctions->row(item);
    delete ui->listWidgetFunctions->takeItem(row);
  }
}

void ToolboxLuaEditor::restoreRecent(const QModelIndex &index)
{
  auto item = ui->listWidgetRecent->item(index.row());
  auto fields = getItemData(item);
  ui->lineEditFunctionName->setText(fields.name);
  ui->textGlobal->setPlainText(fields.global_code);
  ui->textFunction->setPlainText(fields.function_code);
}

void ToolboxLuaEditor::restoreFunction(const QModelIndex &index)
{
  auto item = ui->listWidgetFunctions->item(index.row());
  auto fields = getItemData(item);
  ui->lineEditFunctionName->setText(fields.name);
  ui->textGlobal->setPlainText(fields.global_code);
  ui->textFunction->setPlainText(fields.function_code);
}

void ToolboxLuaEditor::onLibraryUpdated()
{
  if( ui->textLibrary->toPlainText() == _previous_library )
  {
    ui->pushButtonApplyLibrary->setEnabled(false);
    return;
  }

  QString svg_name = ":/resources/svg/green_circle.svg";
  try {
    ReactiveLuaFunction tmp(_plot_data, "", "", ui->textLibrary->toPlainText());
    ui->labelSemaphore->setToolTip("Everything is fine :)");
    int active_series = ui->listWidgetFunctions->count();
    ui->pushButtonApplyLibrary->setEnabled( active_series > 0 );
    _previous_library = ui->textLibrary->toPlainText();

    QSettings settings;
    settings.setValue("ToolboxLuaEditor/library", ui->textLibrary->toPlainText());
  }
  catch(std::runtime_error& ex)
  {
    QString error_msg = ex.what();
    ui->labelSemaphore->setToolTip(error_msg);
    svg_name = ":/resources/svg/red_circle.svg";
    ui->pushButtonApplyLibrary->setEnabled(false);
  }

  QFile file(svg_name);
  file.open(QFile::ReadOnly | QFile::Text);
  QByteArray content(file.readAll());
  QSvgRenderer rr(content);
  QImage image(ui->labelSemaphore->width(), ui->labelSemaphore->height(), QImage::Format_ARGB32);
  QPainter painter(&image);
  image.fill(Qt::transparent);
  rr.render(&painter);
  ui->labelSemaphore->setPixmap( QPixmap::fromImage(image) );
}

void ToolboxLuaEditor::onReloadLibrary()
{
  for(int row = 0; row < ui->listWidgetFunctions->count(); row++)
  {
    auto item = ui->listWidgetFunctions->item(row);
    auto name = item->text();
    auto fields = getItemData(item);
    try{
      auto lua_function = std::make_shared<ReactiveLuaFunction>(
          _plot_data, fields.global_code, fields.function_code, ui->textLibrary->toPlainText() );

      (*_transforms)[fields.name.toStdString()] = lua_function;
    }
    catch(std::runtime_error& err)
    {
      QMessageBox::warning(nullptr, "Error in Lua code",  QString(err.what()), QMessageBox::Cancel);
    }
  }
  ui->pushButtonApplyLibrary->setEnabled(false);
}

bool ToolboxLuaEditor::eventFilter(QObject *obj, QEvent *ev)
{
  if(obj != ui->textGlobal && obj != ui->textFunction && obj != ui->textLibrary )
  {
    return false;
  }

  if (ev->type() == QEvent::DragEnter)
  {
    _dragging_curves.clear();
    auto event = static_cast<QDragEnterEvent*>(ev);
    const QMimeData* mimeData = event->mimeData();
    QStringList mimeFormats = mimeData->formats();
    for (const QString& format : mimeFormats)
    {
      QByteArray encoded = mimeData->data(format);
      QDataStream stream(&encoded, QIODevice::ReadOnly);

      if (format != "curveslist/add_curve")
      {
        return false;
      }

      while (!stream.atEnd())
      {
        QString curve_name;
        stream >> curve_name;
        if (!curve_name.isEmpty())
        {
          _dragging_curves.push_back(curve_name);
        }
      }
      if( !_dragging_curves.empty() )
      {
        event->acceptProposedAction();
      }
    }
    return true;
  }
  else if (ev->type() == QEvent::Drop)
  {
    auto text_edit = qobject_cast<QPlainTextEdit*>(obj);
    for(const auto& name: _dragging_curves)
    {
      text_edit->insertPlainText(QString("\"%1\"\n").arg(name));
    }
    _dragging_curves.clear();
    return true;
  }
  else if (ev->type() == QEvent::Wheel)
  {
    QWheelEvent* wheel_event = dynamic_cast<QWheelEvent*>(ev);
    bool ctrl_modifier_pressed =
        (QGuiApplication::keyboardModifiers() == Qt::ControlModifier);

    if (ctrl_modifier_pressed)
    {
      int prev_size = _font_size;
      if (wheel_event->delta() < 0)
      {
        _font_size = std::max(8, prev_size - 1);
      }
      else if (wheel_event->delta() > 0)
      {
        _font_size = std::min(14, prev_size + 1);
      }
      if (_font_size != prev_size)
      {
        auto font = ui->textGlobal->font();
        font.setPointSize(_font_size);
        ui->textGlobal->setFont( font );
        ui->textFunction->setFont( font );
        ui->textLibrary->setFont( font );

        QSettings settings;
        settings.setValue("ToolboxLuaEditor/fonts_size", _font_size);
      }
      return true;
    }
  }
  return false;
}

ToolboxLuaEditor::SavedData ToolboxLuaEditor::getItemData(const QListWidgetItem *item) const
{
  auto fields = item->data(Qt::UserRole).toStringList();
  SavedData data;
  data.name = fields[0];
  data.global_code = fields[1];
  data.function_code = fields[2];
  return data;
}

void ToolboxLuaEditor::setItemData(QListWidgetItem *item,
                                   QString name,
                                   QString global_code,
                                   QString function_code)
{
  QStringList save_fields;
  save_fields.push_back(name);
  save_fields.push_back(global_code);
  save_fields.push_back(function_code);
  item->setData(Qt::UserRole, save_fields);
}
