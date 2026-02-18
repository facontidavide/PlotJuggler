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

  PJ::PlotDataMapRef* _plot_data = nullptr;
  PJ::TransformsMap* _transforms = nullptr;

  QStringList _dragging_curves;

  bool eventFilter(QObject* obj, QEvent* ev) override;

  void updateTimeControlsEnabled();

  bool getTimeRange(double& tmin, double& tmax) const;
  void setTimeRange(double tmin, double tmax);

  void updateTimeRange();

  void saveAll();

  double _t0 = 0.0;

  // Common structure to facilitate future exporter implementations
  struct ExportTable
  {
    std::vector<std::string> names;
    std::vector<double> time;
    std::vector<std::vector<double>> cols;
  };

  static double estimateMinDt(const PJ::PlotData& plot, size_t start_idx, double t_end);
  ExportTable buildExportTable(const std::vector<std::string>& topics, double t_start,
                               double t_end) const;

  bool serializeCSV(const ToolboxCSV::ExportTable& t, const QString& path);
  void debugPrintTable(const ExportTable& t);
};
