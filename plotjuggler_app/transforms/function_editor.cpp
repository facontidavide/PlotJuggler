#include "function_editor.h"
#include "custom_function.h"
#include "plotwidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QFont>
#include <QDomDocument>
#include <QDomElement>
#include <QFontDatabase>
#include <QFile>
#include <QMenu>
#include <QAction>
#include <QDir>
#include <QToolTip>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QByteArray>
#include <QInputDialog>
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QMimeData>
#include <QTableWidgetItem>
#include <QTimer>
#include <QSyntaxHighlighter>

#include "lua_custom_function.h"
#include "PlotJuggler/svg_util.h"
#include "ui_function_editor_help.h"

void FunctionEditorWidget::on_stylesheetChanged(QString theme)
{
  ui->pushButtonDeleteCurves->setIcon(LoadSvg(":/resources/svg/trash.svg", theme));
  ui->buttonLoadFunctions->setIcon(LoadSvg(":/resources/svg/import.svg", theme));
  ui->buttonSaveFunctions->setIcon(LoadSvg(":/resources/svg/export.svg", theme));
  ui->buttonSaveCurrent->setIcon(LoadSvg(":/resources/svg/save.svg", theme));

  _global_highlighter->setTheme(theme);
  _function_highlighter->setTheme(theme);
}

FunctionEditorWidget::FunctionEditorWidget(PlotDataMapRef& plotMapData,
                                           const TransformsMap& mapped_custom_plots,
                                           QWidget* parent)
  : QWidget(parent)
  , _plot_map_data(plotMapData)
  , _transform_maps(mapped_custom_plots)
  , ui(new Ui::FunctionEditor)
  , _v_count(1)
  , _preview_widget(new PlotWidget(_local_plot_data, this))
{
  ui->setupUi(this);

  _global_highlighter = new LuaHighlighter( ui->globalVarsTextField->document() );
  _function_highlighter = new LuaHighlighter( ui->mathEquation->document() );

  QSettings settings;

  this->setWindowTitle("Create a custom timeseries");

  QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  fixedFont.setPointSize(10);

  ui->globalVarsTextField->setFont(fixedFont);
  ui->mathEquation->setFont(fixedFont);
  ui->snippetPreview->setFont(fixedFont);

  auto theme = settings.value("StyleSheet:theme", "ligth").toString();
  on_stylesheetChanged(theme);

  QPalette palette = ui->listAdditionalSources->palette();
  palette.setBrush(QPalette::Highlight, palette.brush(QPalette::Base));
  palette.setBrush(QPalette::HighlightedText, palette.brush(QPalette::Text));
  ui->listAdditionalSources->setPalette(palette);

  QStringList numericPlotNames;
  for (const auto& p : _plot_map_data.numeric)
  {
    QString name = QString::fromStdString(p.first);
    numericPlotNames.push_back(name);
  }
  numericPlotNames.sort(Qt::CaseInsensitive);

  QByteArray saved_xml =
      settings.value("AddCustomPlotDialog.recentSnippetsXML", QByteArray()).toByteArray();
  restoreGeometry(settings.value("AddCustomPlotDialog.geometry").toByteArray());

  if (saved_xml.isEmpty())
  {
    QFile file("://resources/default.snippets.xml");
    if (!file.open(QIODevice::ReadOnly))
    {
      throw std::runtime_error("problem with default.snippets.xml");
    }
    saved_xml = file.readAll();
  }

  importSnippets(saved_xml);

  ui->snippetsListSaved->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(ui->snippetsListSaved, &QListWidget::customContextMenuRequested, this,
          &FunctionEditorWidget::savedContextMenu);

  ui->globalVarsTextField->setPlainText(
      settings.value("AddCustomPlotDialog.previousGlobals", "").toString());

  ui->mathEquation->setPlainText(
      settings.value("AddCustomPlotDialog.previousFunction", "return value").toString());

  ui->lineEditSource->installEventFilter(this);
  ui->listAdditionalSources->installEventFilter(this);

  auto preview_layout = new QHBoxLayout(ui->framePlotPreview);
  preview_layout->setMargin(6);
  preview_layout->addWidget(_preview_widget->widget());

  _preview_widget->setContextMenuEnabled(false);

  _update_preview_timer.setSingleShot(true);

  connect(&_update_preview_timer, &QTimer::timeout, this,
          &FunctionEditorWidget::on_updatePreview);

  updatePreview();

}

FunctionEditorWidget::~FunctionEditorWidget()
{
  delete _preview_widget;

  QSettings settings;
  settings.setValue("AddCustomPlotDialog.recentSnippetsXML", exportSnippets());
  settings.setValue("AddCustomPlotDialog.geometry", saveGeometry());
  settings.setValue("AddCustomPlotDialog.previousGlobals",
                    ui->globalVarsTextField->toPlainText());
  settings.setValue("AddCustomPlotDialog.previousFunction",
                    ui->mathEquation->toPlainText());

  delete ui;
}

void FunctionEditorWidget::setLinkedPlotName(const QString& linkedPlotName)
{
  ui->lineEditSource->setText(linkedPlotName);
}

void FunctionEditorWidget::clear()
{
  ui->lineEditSource->setText("");
  ui->nameLineEdit->setText("");
  ui->listAdditionalSources->setRowCount(0);
}

QString FunctionEditorWidget::getLinkedData() const
{
  return ui->lineEditSource->text();
}

QString FunctionEditorWidget::getglobal_vars() const
{
  return ui->globalVarsTextField->toPlainText();
}

QString FunctionEditorWidget::getEquation() const
{
  return ui->mathEquation->toPlainText();
}

QString FunctionEditorWidget::getName() const
{
  return ui->nameLineEdit->text();
}

void FunctionEditorWidget::createNewPlot()
{
  ui->nameLineEdit->setEnabled(true);
  ui->lineEditSource->setEnabled(true);
  _editor_mode = CREATE;
}

void FunctionEditorWidget::editExistingPlot(CustomPlotPtr data)
{
  ui->globalVarsTextField->setPlainText(data->snippet().global_vars);
  ui->mathEquation->setPlainText(data->snippet().function);
  setLinkedPlotName(data->snippet().linked_source);
  ui->nameLineEdit->setText(data->aliasName());
  ui->nameLineEdit->setEnabled(false);

  _editor_mode = MODIFY;

  auto list_widget = ui->listAdditionalSources;
  list_widget->setRowCount(0);

  for (QString curve_name : data->snippet().additional_sources)
  {
    if (list_widget->findItems(curve_name, Qt::MatchExactly).isEmpty() &&
        curve_name != ui->lineEditSource->text())
    {
      int row = list_widget->rowCount();
      list_widget->setRowCount(row + 1);
      list_widget->setItem(row, 0, new QTableWidgetItem(QString("v%1").arg(row + 1)));
      list_widget->setItem(row, 1, new QTableWidgetItem(curve_name));
    }
  }
  on_listSourcesChanged();
}

// CustomPlotPtr FunctionEditorWidget::getCustomPlotData() const
//{
//  return _plot;
//}

bool FunctionEditorWidget::eventFilter(QObject* obj, QEvent* ev)
{
  if (ev->type() == QEvent::DragEnter)
  {
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

      _dragging_curves.clear();

      while (!stream.atEnd())
      {
        QString curve_name;
        stream >> curve_name;
        if (!curve_name.isEmpty())
        {
          _dragging_curves.push_back(curve_name);
        }
      }
      if ((obj == ui->lineEditSource && _dragging_curves.size() == 1) ||
          (obj == ui->listAdditionalSources && _dragging_curves.size() > 0))
      {
        event->acceptProposedAction();
        return true;
      }
    }
  }
  else if (ev->type() == QEvent::Drop)
  {
    if (obj == ui->lineEditSource)
    {
      ui->lineEditSource->setText(_dragging_curves.front());
    }
    else if (obj == ui->listAdditionalSources)
    {
      auto list_widget = ui->listAdditionalSources;
      for (QString curve_name : _dragging_curves)
      {
        if (list_widget->findItems(curve_name, Qt::MatchExactly).isEmpty() &&
            curve_name != ui->lineEditSource->text())
        {
          int row = list_widget->rowCount();
          list_widget->setRowCount(row + 1);
          list_widget->setItem(row, 0, new QTableWidgetItem(QString("v%1").arg(row + 1)));
          list_widget->setItem(row, 1, new QTableWidgetItem(curve_name));
        }
      }
      on_listSourcesChanged();
    }
  }

  return false;
}

void FunctionEditorWidget::importSnippets(const QByteArray& xml_text)
{
  ui->snippetsListSaved->clear();

  _snipped_saved = GetSnippetsFromXML(xml_text);

  for (const auto& it : _snipped_saved)
  {
    ui->snippetsListSaved->addItem(it.first);
  }

  for (const auto& custom_it : _transform_maps)
  {
    auto math_plot = dynamic_cast<LuaCustomFunction*>(custom_it.second.get());
    if (!math_plot)
    {
      continue;
    }
    SnippetData snippet;
    snippet.alias_name = math_plot->aliasName();

    if (_snipped_saved.count(snippet.alias_name) > 0)
    {
      continue;
    }

    snippet.global_vars = math_plot->snippet().global_vars;
    snippet.function = math_plot->snippet().function;
  }
  ui->snippetsListSaved->sortItems();
}

QByteArray FunctionEditorWidget::exportSnippets() const
{
  QDomDocument doc;
  auto root = ExportSnippets(_snipped_saved, doc);
  doc.appendChild(root);
  return doc.toByteArray(2);
}

void FunctionEditorWidget::on_snippetsListSaved_currentRowChanged(int current_row)
{
  if (current_row < 0)
  {
    ui->snippetPreview->setPlainText("");
    return;
  }
  const auto& name = ui->snippetsListSaved->currentItem()->text();
  const SnippetData& snippet = _snipped_saved.at(name);

  QString preview;

  if (!snippet.global_vars.isEmpty())
  {
    preview += snippet.global_vars + "\n\n";
  }
  preview += "function calc(time, value";

  for (int i = 1; i <= snippet.additional_sources.size(); i++)
  {
    preview += QString(", v%1").arg(i);
  }

  preview += ")\n";
  auto function_lines = snippet.function.split("\n");
  for (const auto& line : function_lines)
  {
    preview += "    " + line + "\n";
  }
  preview += "end";
  ui->snippetPreview->setPlainText(preview);
}

void FunctionEditorWidget::on_snippetsListSaved_doubleClicked(const QModelIndex& index)
{
  const auto& name = ui->snippetsListSaved->item(index.row())->text();
  const SnippetData& snippet = _snipped_saved.at(name);

  ui->globalVarsTextField->setPlainText(snippet.global_vars);
  ui->mathEquation->setPlainText(snippet.function);
}

void FunctionEditorWidget::savedContextMenu(const QPoint& pos)
{
  auto list_saved = ui->snippetsListSaved;

  if (list_saved->selectedItems().size() != 1)
  {
    return;
  }

  QMenu menu;

  QAction* rename_item = new QAction("Rename...", this);
  menu.addAction(rename_item);

  connect(rename_item, &QAction::triggered, this, &FunctionEditorWidget::onRenameSaved);

  QAction* remove_item = new QAction("Remove", this);
  menu.addAction(remove_item);

  connect(remove_item, &QAction::triggered, this, [list_saved, this]() {
    const auto& item = list_saved->selectedItems().first();
    _snipped_saved.erase(item->text());
    delete list_saved->takeItem(list_saved->row(item));
  });

  menu.exec(list_saved->mapToGlobal(pos));
}

void FunctionEditorWidget::on_nameLineEdit_textChanged(const QString& name)
{
  if (_plot_map_data.numeric.count(name.toStdString()) == 0)
  {
    ui->pushButtonCreate->setText("Create New Timeseries");
  }
  else
  {
    ui->pushButtonCreate->setText("Modify Timeseries");
  }
  updatePreview();
}

void FunctionEditorWidget::on_buttonLoadFunctions_clicked()
{
  QSettings settings;
  QString directory_path =
      settings.value("AddCustomPlotDialog.loadDirectory", QDir::currentPath()).toString();

  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Open Snippet Library"), directory_path, tr("Snippets (*.snippets.xml)"));
  if (fileName.isEmpty())
  {
    return;
  }

  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly))
  {
    QMessageBox::critical(this, "Error",
                          QString("Failed to open the file [%1]").arg(fileName));
    return;
  }

  directory_path = QFileInfo(fileName).absolutePath();
  settings.setValue("AddCustomPlotDialog.loadDirectory", directory_path);

  importSnippets(file.readAll());
}

void FunctionEditorWidget::on_buttonSaveFunctions_clicked()
{
  QSettings settings;
  QString directory_path =
      settings.value("AddCustomPlotDialog.loadDirectory", QDir::currentPath()).toString();

  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Open Snippet Library"), directory_path, tr("Snippets (*.snippets.xml)"));

  if (fileName.isEmpty())
  {
    return;
  }
  if (!fileName.endsWith(".snippets.xml"))
  {
    fileName.append(".snippets.xml");
  }

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly))
  {
    QMessageBox::critical(this, "Error",
                          QString("Failed to open the file [%1]").arg(fileName));
    return;
  }
  auto data = exportSnippets();

  file.write(data);
  file.close();

  directory_path = QFileInfo(fileName).absolutePath();
  settings.setValue("AddCustomPlotDialog.loadDirectory", directory_path);
}

void FunctionEditorWidget::on_buttonSaveCurrent_clicked()
{
  QString name;

  auto selected_snippets = ui->snippetsListSaved->selectedItems();
  if (selected_snippets.size() >= 1)
  {
    name = selected_snippets.front()->text();
  }
  bool ok = false;
  name = QInputDialog::getText(this, tr("Name of the Function"), tr("Name:"),
                               QLineEdit::Normal, name, &ok);

  if (!ok || name.isEmpty())
  {
    return;
  }

  SnippetData snippet;
  snippet.alias_name = name;
  snippet.global_vars = ui->globalVarsTextField->toPlainText();
  snippet.function = ui->mathEquation->toPlainText();

  addToSaved(name, snippet);

  on_snippetsListSaved_currentRowChanged(ui->snippetsListSaved->currentRow());
}

bool FunctionEditorWidget::addToSaved(const QString& name, const SnippetData& snippet)
{
  if (_snipped_saved.count(name))
  {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Warning");
    msgBox.setText(tr("A function with the same name exists already in the list of saved "
                      "functions.\n"));
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton* button = msgBox.addButton(tr("Overwrite"), QMessageBox::YesRole);
    msgBox.setDefaultButton(button);

    int res = msgBox.exec();

    if (res < 0 || res == QMessageBox::Cancel)
    {
      return false;
    }
  }
  else
  {
    ui->snippetsListSaved->addItem(name);
    ui->snippetsListSaved->sortItems();
  }
  _snipped_saved[name] = snippet;
  return true;
}

void FunctionEditorWidget::onRenameSaved()
{
  auto list_saved = ui->snippetsListSaved;
  auto item = list_saved->selectedItems().first();
  const auto& name = item->text();

  bool ok;
  QString new_name = QInputDialog::getText(this, tr("Change the name of the function"),
                                           tr("New name:"), QLineEdit::Normal, name, &ok);

  if (!ok || new_name.isEmpty() || new_name == name)
  {
    return;
  }

  SnippetData snippet = _snipped_saved[name];
  _snipped_saved.erase(name);
  snippet.alias_name = new_name;

  _snipped_saved.insert({ new_name, snippet });
  item->setText(new_name);
  ui->snippetsListSaved->sortItems();
}

void FunctionEditorWidget::on_pushButtonCreate_clicked()
{
  try
  {
    std::string new_plot_name = getName().toStdString();

    if (_editor_mode == CREATE && _transform_maps.count(new_plot_name) != 0)
    {
      QMessageBox msgBox(this);
      msgBox.setWindowTitle("Warning");
      msgBox.setText(tr("A custom time series with the same name exists already.\n"
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

    SnippetData snippet;
    snippet.function = getEquation();
    snippet.global_vars = getglobal_vars();
    snippet.alias_name = getName();
    snippet.linked_source = getLinkedData();
    for (int row = 0; row < ui->listAdditionalSources->rowCount(); row++)
    {
      snippet.additional_sources.push_back(
          ui->listAdditionalSources->item(row, 1)->text());
    }

    CustomPlotPtr plot = std::make_unique<LuaCustomFunction>(snippet);
    accept(plot);
  }
  catch (const std::runtime_error& e)
  {
    QMessageBox::critical(this, "Error",
                          "Failed to create math plot : " +
                              QString::fromStdString(e.what()));
  }
}

void FunctionEditorWidget::on_pushButtonCancel_pressed()
{
  if (_editor_mode == MODIFY)
  {
    clear();
  }
  closed();
}

void FunctionEditorWidget::on_listSourcesChanged()
{
  QString function_text("function( time, value");
  for (int row = 0; row < ui->listAdditionalSources->rowCount(); row++)
  {
    function_text += ", ";
    function_text += ui->listAdditionalSources->item(row, 0)->text();
  }
  function_text += " )";
  ui->labelFunction->setText(function_text);

  updatePreview();
}

void FunctionEditorWidget::on_listAdditionalSources_itemSelectionChanged()
{
  bool any_selected = !ui->listAdditionalSources->selectedItems().isEmpty();
  ui->pushButtonDeleteCurves->setEnabled(any_selected);
}

void FunctionEditorWidget::on_pushButtonDeleteCurves_clicked()
{
  auto list_sources = ui->listAdditionalSources;
  QModelIndexList selected = list_sources->selectionModel()->selectedRows();
  while (selected.size() > 0)
  {
    list_sources->removeRow(selected.first().row());
    selected = list_sources->selectionModel()->selectedRows();
  }
  for (int row = 0; row < list_sources->rowCount(); row++)
  {
    list_sources->item(row, 0)->setText(QString("v%1").arg(row + 1));
  }

  on_listAdditionalSources_itemSelectionChanged();
  on_listSourcesChanged();
}

void FunctionEditorWidget::on_lineEditSource_textChanged(const QString& text)
{
  updatePreview();
}

void FunctionEditorWidget::on_mathEquation_textChanged()
{
  updatePreview();
}

void FunctionEditorWidget::updatePreview()
{
  _update_preview_timer.start(250);
}

void FunctionEditorWidget::on_updatePreview()
{
  QString errors;
  std::string new_plot_name = ui->nameLineEdit->text().toStdString();

  if (_transform_maps.count(new_plot_name) != 0)
  {
    if (ui->lineEditSource->text().toStdString() == new_plot_name ||
        ui->listAdditionalSources->findItems(getName(), Qt::MatchExactly).isEmpty() ==
            false)
    {
      errors += "- The name of the new timeseries is the same of one of its "
                "dependencies.\n";
    }
  }

  if (new_plot_name.empty())
  {
    errors += "- Missing name of the new time series.\n";
  }
  else
  {
    // check if name is unique (except if is custom_plot)
    if (_plot_map_data.numeric.count(new_plot_name) != 0 &&
        _transform_maps.count(new_plot_name) == 0)
    {
      errors += "- Plot name already exists and can't be modified.\n";
    }
  }

  if (ui->lineEditSource->text().isEmpty())
  {
    errors += "- Missing source time series.\n";
  }

  SnippetData snippet;
  snippet.function = getEquation();
  snippet.global_vars = getglobal_vars();
  snippet.alias_name = getName();
  snippet.linked_source = getLinkedData();
  for (int row = 0; row < ui->listAdditionalSources->rowCount(); row++)
  {
    snippet.additional_sources.push_back(ui->listAdditionalSources->item(row, 1)->text());
  }

  CustomPlotPtr lua_function;
  try
  {
    lua_function = std::make_unique<LuaCustomFunction>(snippet);
    ui->buttonSaveCurrent->setEnabled(true);
  }
  catch (...)
  {
    errors += "- The Lua function is not valid.\n";
    ui->buttonSaveCurrent->setEnabled(false);
  }

  if (lua_function)
  {
    try
    {
      std::string name = new_plot_name.empty() ? "no_name" : new_plot_name;
      PlotData& out_data = _local_plot_data.getOrCreateNumeric(name);
      out_data.clear();

      std::vector<PlotData*> out_vector = { &out_data };
      lua_function->setData(&_plot_map_data, {}, out_vector);
      lua_function->calculate();

      _preview_widget->removeAllCurves();
      _preview_widget->addCurve(name, Qt::blue);
      _preview_widget->zoomOut(false);
    }
    catch (...)
    {
      errors += "- The Lua function can not compute the result.\n";
    }
  }
  //----------------------------------

  QFile file(":/resources/svg/red_circle.svg");

  if (errors.isEmpty())
  {
    errors = "Everything is fine :)";
    file.setFileName(":/resources/svg/green_circle.svg");
    ui->pushButtonCreate->setEnabled(true);
  }
  else
  {
    errors = errors.left(errors.size() - 1);
    ui->pushButtonCreate->setEnabled(false);
  }

  ui->labelSemaphore->setToolTip(errors);
  ui->labelSemaphore->setToolTipDuration(5000);

  file.open(QFile::ReadOnly | QFile::Text);
  auto svg_data = file.readAll();
  file.close();
  QByteArray content(svg_data);
  QSvgRenderer rr(content);
  QImage image(26, 26, QImage::Format_ARGB32);
  QPainter painter(&image);
  image.fill(Qt::transparent);
  rr.render(&painter);
  ui->labelSemaphore->setPixmap(QPixmap::fromImage(image));
}

void FunctionEditorWidget::on_globalVarsTextField_textChanged()
{
  updatePreview();
}

void FunctionEditorWidget::on_pushButtonHelp_clicked()
{
  QDialog* dialog = new QDialog(this);
  auto ui = new Ui_FunctionEditorHelp();
  ui->setupUi(dialog);

  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->exec();
}
