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
#include "ui_functions_library.h"
#include "plotwidget.h"
#include "PlotJuggler/util/delayed_callback.hpp"

#include "QLuaCompleter"
#include "QSyntaxStyle"

class FunctionEditorWidget : public QWidget
{
  Q_OBJECT

public:
  explicit FunctionEditorWidget(PlotDataMapRef& plotMapData,
                                const TransformsMap& mapped_custom_plots, QWidget* parent);

  virtual ~FunctionEditorWidget() override;

  void setLinkedPlotName(const QString& linkedPlotName);

  enum EditorMode
  {
    CREATE,
    MODIFY
  };

  enum class ScriptLang
  {
    Lua,
    Python
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

  void on_nameLineEdit_textChanged(const QString& arg1);

  void on_buttonLoadFunctions_clicked();

  void on_buttonSaveFunctions_clicked();

  void on_buttonSaveCurrent_clicked();

  void on_pushButtonCreate_clicked();

  void on_pushButtonCancel_pressed();

  void on_listAdditionalSources_itemSelectionChanged();

  void on_pushButtonDeleteCurves_clicked();

  void on_listSourcesChanged();

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

  void onScriptLangChanged();

private:
  void importSnippets(const QByteArray& xml_text);

  QByteArray exportSnippets() const;

  bool addToSaved(const QString& name, const SnippetData& snippet);

  void updatePreview();

  //  QTimer _update_preview_timer;

  PlotDataMapRef& _plot_map_data;
  const TransformsMap& _transform_maps;
  Ui::FunctionEditor* ui;
  Ui::FunctionsLibrary* _functions_library_ui;

  QDialog* _functions_library_dialog;
  QWidget* _functions_library_overlay;

  QString _selected_library_name;

  void reloadFunctionsLibraryTable();
  void updateFunctionsLibraryPreview();

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

  void setupFunctionAppsButton();

  void syncSourceFromRadio();

  ScriptLang currentLang() const;
  ScriptLang currentLangBatch() const;

  CustomPlotPtr createCustomFunction(const SnippetData& snippet, ScriptLang lang) const;

  void reassignRadioRows();

  QButtonGroup* _source_group = nullptr;
  QString _linked_source;
  int _linked_source_row = -1;

signals:
  void accept(std::vector<CustomPlotPtr> plot);
  void closed();
};

#endif  // AddCustomPlotDialog_H
