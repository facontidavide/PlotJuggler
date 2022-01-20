#include "lua_editor.h"
#include "ui_lua_editor.h"
#include <QSettings>
#include <QPushButton>
#include <QLineEdit>
#include <QSettings>
#include <memory>

#include "PlotJuggler/reactive_function.h"
#include "PlotJuggler/svg_util.h"

ToolboxLuaEditor::ToolboxLuaEditor()
{
  _widget = new QWidget(nullptr);
  ui = new Ui::LuaEditor;
  ui->setupUi(_widget);

  ui->textGlobal->installEventFilter(this);
  ui->textFunction->installEventFilter(this);

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

  connect(ui->listWidgetFunctions, &QListWidget::itemSelectionChanged, this,
          [this]() {
            auto selected = ui->listWidgetFunctions->selectedItems();
            ui->pushButtonDelete->setEnabled(selected.size()>0);
          });

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
        QStringList fields;
        fields.push_back(name);
        fields.push_back(elem.attribute("global"));
        fields.push_back(elem.attribute("function"));

        auto item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, fields);
        ui->listWidgetRecent->addItem(item);
      }
    }
  }
}

ToolboxLuaEditor::~ToolboxLuaEditor()
{
  delete ui;
}

const char *ToolboxLuaEditor::name() const
{
  return "Advanced Lua Functions";
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

bool ToolboxLuaEditor::onShowWidget()
{
  ui->listWidgetFunctions->clear();
  _lua_functions.clear();

  // check the already existing functions.
  for(auto it : *_transforms)
  {
    if( auto lua_function = std::dynamic_pointer_cast<ReactiveLuaFunction>( it.second ))
    {
      QString name = QString::fromStdString(it.first);
      _lua_functions.insert( {name, lua_function} );
      ui->listWidgetFunctions->addItem(name);
    }
  }
  ui->listWidgetFunctions->sortItems();

  QSettings settings;
  QString theme = settings.value("StyleSheet::theme", "light").toString();

  ui->pushButtonDelete->setIcon(LoadSvg(":/resources/svg/clear.svg", theme));

  auto font = ui->textGlobal->font();
  font.setPointSize(12);
  ui->textGlobal->setFont( font );
  ui->textFunction->setFont( font );
  ui->textLibrary->setFont( font );

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
        _plot_data, ui->textGlobal->toPlainText(), ui->textFunction->toPlainText() );

    (*_transforms)[name.toStdString()] = lua_function;

    _lua_functions.insert( {name, lua_function} );
    if( ui->listWidgetFunctions->findItems(name, Qt::MatchExactly).empty() )
    {
      ui->listWidgetFunctions->addItem(name);
    }

    for( auto& new_name: lua_function->createdCuves() )
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
  QStringList save_fields;
  save_fields.push_back(name);
  save_fields.push_back(ui->textGlobal->toPlainText());
  save_fields.push_back(ui->textFunction->toPlainText());
  auto new_item = new QListWidgetItem(name);
  new_item->setData(Qt::UserRole, save_fields);
  ui->listWidgetRecent->addItem(new_item);

  QDomDocument xml_doc;
  auto root = xml_doc.createElement("functions");

  for(int row = 0; row < ui->listWidgetRecent->count(); row++)
  {
    auto item = ui->listWidgetRecent->item(row);
    auto fields = item->data(Qt::UserRole).toStringList();

    auto elem = xml_doc.createElement("function");
    elem.setAttribute("name", fields[0]);
    elem.setAttribute("global", fields[1]);
    elem.setAttribute("function", fields[2]);
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
  auto fields = item->data(Qt::UserRole).toStringList();
  ui->lineEditFunctionName->setText(fields[0]);
  ui->textGlobal->setPlainText(fields[1]);
  ui->textFunction->setPlainText(fields[2]);
}

bool ToolboxLuaEditor::eventFilter(QObject *obj, QEvent *ev)
{
  if(obj != ui->textGlobal && obj != ui->textFunction )
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
  return false;
}
