#ifndef AddCustomPlotDialog_H
#define AddCustomPlotDialog_H

#include <QDialog>
#include <QTimer>
#include <QListWidgetItem>
#include <unordered_map>
#include "PlotJuggler/plotdata.h"
#include "custom_function.h"
#include "qwt_plot_curve.h"
#include "ui_function_editor.h"
#include "plotwidget.h"
#include "PlotJuggler/util/delayed_callback.hpp"

#include "QLuaCompleter"
#include "QSyntaxStyle"

class FunctionEditorWidget : public QWidget
{
  Q_OBJECT

public:
  explicit FunctionEditorWidget(PlotDataMapRef& plotMapData,
                                const TransformsMap& mapped_custom_plots,
                                QWidget* parent);

  virtual ~FunctionEditorWidget() override;

  void setLinkedPlotName(const QString& linkedPlotName);

  enum EditorMode
  {
    CREATE,
    MODIFY
  };

  void clear();

  QString getLinkedData() const;

  const PlotData& getPlotData() const;

  void createNewPlot();

  void editExistingPlot(CustomPlotPtr data);

  bool eventFilter(QObject* obj, QEvent* event) override;

  void saveSettings();

public slots:
  void on_stylesheetChanged(QString theme);

private slots:

  void on_snippetsListSaved_currentRowChanged(int currentRow);

  void on_snippetsListSaved_doubleClicked(const QModelIndex& index);

  void on_nameLineEdit_textChanged(const QString& arg1);

  void savedContextMenu(const QPoint& pos);

  void on_buttonLoadFunctions_clicked();

  void on_buttonSaveFunctions_clicked();

  void on_buttonSaveCurrent_clicked();

  void onRenameSaved();

  void on_pushButtonCreate_clicked();

  void on_pushButtonCancel_pressed();

  void on_listAdditionalSources_itemSelectionChanged();

  void on_pushButtonDeleteCurves_clicked();

  void on_listSourcesChanged();

  void on_lineEditSource_textChanged(const QString& text);

  void onUpdatePreview();

  void onUpdatePreviewBatch();

  void on_pushButtonHelp_clicked();

  void onLineEditTab2FilterChanged();

  void on_pushButtonHelpTab2_clicked();

  void on_lineEditTab2Filter_textChanged(const QString& arg1);

  void on_functionTextBatch_textChanged();

  void on_suffixLineEdit_textChanged(const QString& arg1);

  void on_tabWidget_currentChanged(int index);

  void on_globalVarsTextBatch_textChanged();

  void on_globalVarsText_textChanged();

  void on_functionText_textChanged();

private:
  void importSnippets(const QByteArray& xml_text);

  QByteArray exportSnippets() const;

  bool addToSaved(const QString& name, const SnippetData& snippet);

  void updatePreview();

  //  QTimer _update_preview_timer;

  PlotDataMapRef& _plot_map_data;
  const TransformsMap& _transform_maps;
  Ui::FunctionEditor* ui;

  int _v_count;

  SnippetsMap _snipped_saved;

  QStringList _dragging_curves;

  PlotDataMapRef _local_plot_data;
  PlotWidget* _preview_widget;

  EditorMode _editor_mode;

  QLuaCompleter* lua_completer_;
  QLuaCompleter* lua_completer_batch_;

  DelayedCallback _tab2_filter;

  DelayedCallback _update_preview_tab1;
  DelayedCallback _update_preview_tab2;

  void setSemaphore(QLabel* semaphore, QString errors);

signals:
  void accept(std::vector<CustomPlotPtr> plot);
  void closed();
};

#endif  // AddCustomPlotDialog_H
