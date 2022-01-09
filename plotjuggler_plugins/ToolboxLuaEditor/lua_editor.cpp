#include "lua_editor.h"
#include "ui_lua_editor.h"
#include <QSettings>
#include <QPushButton>
#include <QLineEdit>
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

  connect(ui->lineEditFunctionName, &QLineEdit::textChanged, this, [this]()
          {
            bool has_name = ui->lineEditFunctionName->text().isEmpty() == false;
            ui->pushButtonSave->setEnabled( has_name );
          } );

  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ToolboxPlugin::closed);

  _global_highlighter = new LuaHighlighter( ui->textGlobal->document() );
  _function_highlighter = new LuaHighlighter( ui->textFunction->document() );
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

  return true;
}

void ToolboxLuaEditor::onSave()
{
  try {
    auto lua_function = std::make_shared<ReactiveLuaFunction>(
        _plot_data, ui->textGlobal->toPlainText(), ui->textFunction->toPlainText() );

    auto name = ui->lineEditFunctionName->text();
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
