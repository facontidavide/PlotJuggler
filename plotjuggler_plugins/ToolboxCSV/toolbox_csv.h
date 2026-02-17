#pragma once

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/toolbox_base.h"
#include "PlotJuggler/plotwidget_base.h"

namespace Ui
{
class toolBoxUI;
}

class ToolboxCSV : public PJ::ToolboxPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.Toolbox")
  Q_INTERFACES(PJ::ToolboxPlugin)

public:
  ToolboxCSV();

  ~ToolboxCSV() override;

  const char* name() const override
  {
    return "CSV data saver";
  }

  void init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map) override;

  std::pair<QWidget*, WidgetType> providedWidget() const override;

public slots:

  bool onShowWidget() override;

private slots:

  void onClosed();
  void on_toolButton_clicked();

private:
  QWidget* _widget;
  Ui::toolBoxUI* ui;

  PJ::PlotWidgetBase* _plot_widget = nullptr;
  PJ::PlotDataMapRef* _plot_data = nullptr;
  PJ::TransformsMap* _transforms = nullptr;

  QStringList _dragging_curves;

  bool eventFilter(QObject* obj, QEvent* ev) override;
  void updateTimeControlsEnabled();
};
