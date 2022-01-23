#ifndef LUA_EDITOR_H
#define LUA_EDITOR_H

#include <QtPlugin>
#include <QListWidgetItem>
#include <map>
#include "PlotJuggler/toolbox_base.h"
#include "PlotJuggler/plotwidget_base.h"
#include "PlotJuggler/reactive_function.h"
#include "PlotJuggler/lua_highlighter.h"
#include "PlotJuggler/util/delayed_callback.hpp"

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

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

public slots:

  bool onShowWidget() override;

  void onSave();

  void onDelete();

  void restoreRecent(const QModelIndex &index);

  void restoreFunction(const QModelIndex &index);

  void onLibraryUpdated();

  void onReloadLibrary();

private:
  QWidget* _widget;
  Ui::LuaEditor* ui;

  PJ::PlotDataMapRef* _plot_data = nullptr;
  PJ::TransformsMap* _transforms = nullptr;


  bool eventFilter(QObject *obj, QEvent *event) override;
  QStringList _dragging_curves;

  LuaHighlighter* _global_highlighter;
  LuaHighlighter* _function_highlighter;
  LuaHighlighter* _helper_highlighter;

  int _font_size;
  DelayedCallback _delay_library_check;

  QString _previous_library;

  struct SavedData
  {
    QString name;
    QString global_code;
    QString function_code;
  };

  SavedData getItemData(const QListWidgetItem* item) const;

  void setItemData(QListWidgetItem* item, QString name, QString global_code, QString function_code);
};

#endif // LUA_EDITOR_H
