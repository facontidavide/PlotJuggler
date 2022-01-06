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

bool ToolboxLuaEditor::eventFilter(QObject *obj, QEvent *event)
{
  return false;
}
