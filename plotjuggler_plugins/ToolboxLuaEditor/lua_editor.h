#ifndef LUA_EDITOR_H
#define LUA_EDITOR_H

#include <QtPlugin>
#include <map>
#include "PlotJuggler/toolbox_base.h"
#include "PlotJuggler/plotwidget_base.h"
#include "PlotJuggler/reactive_function.h"
#include "PlotJuggler/lua_highlighter.h"

namespace Ui
{
class LuaEditor;
}

class ToolboxLuaEditor : public PJ::ToolboxPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.Toolbox")
  Q_INTERFACES(PJ::ToolboxPlugin)

public:
  ToolboxLuaEditor();

  ~ToolboxLuaEditor() override;

  const char* name() const override;

  void init(PJ::PlotDataMapRef& src_data,
            PJ::TransformsMap& transform_map) override;

  std::pair<QWidget*, WidgetType> providedWidget() const override;

public slots:

  bool onShowWidget() override;

  void onSave();

  void onDelete();

  void restoreRecent(const QModelIndex &index);

private:
  QWidget* _widget;
  Ui::LuaEditor* ui;

  PJ::PlotDataMapRef* _plot_data = nullptr;
  PJ::TransformsMap* _transforms = nullptr;

  std::map<QString, std::shared_ptr<PJ::ReactiveLuaFunction>> _lua_functions;

   bool eventFilter(QObject *obj, QEvent *event) override;
   QStringList _dragging_curves;

   LuaHighlighter* _global_highlighter;
   LuaHighlighter* _function_highlighter;
   LuaHighlighter* _helper_highlighter;
};

#endif // LUA_EDITOR_H
