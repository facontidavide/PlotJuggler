#include "dataload_csv.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>
#include "selectlistdialog.h"

DataLoadCSV::DataLoadCSV()
{
  _extensions.push_back("csv");
}

const QRegExp csv_separator("(\\,|\\;|\\|)");

const std::vector<const char*>& DataLoadCSV::compatibleFileExtensions() const
{
  return _extensions;
}

QSize DataLoadCSV::parseHeader(QFile* file, std::vector<std::string>& ordered_names)
{
  QTextStream inA(file);

  QString first_line = inA.readLine();

  QStringList firstline_items = first_line.split(csv_separator);

  int linecount = 0;
  const int columncount = firstline_items.count();

  for (int i = 0; i < firstline_items.size(); i++)
  {
    // remove annoying prefix
    QString field_name(firstline_items[i]);

    if (field_name.isEmpty())
    {
      field_name = QString("_Column_%1").arg(i);
    }
    ordered_names.push_back(field_name.toStdString());
  }

  while (!inA.atEnd())
  {
    inA.readLine();
    linecount++;
  }

  QSize table_size;
  table_size.setWidth(columncount);
  table_size.setHeight(linecount);
  return table_size;
}

bool DataLoadCSV::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  bool use_provided_configuration = false;

  if (info->plugin_config.hasChildNodes())
  {
    use_provided_configuration = true;
    xmlLoadState(info->plugin_config.firstChildElement());
  }

  const int TIME_INDEX_NOT_DEFINED = -2;

  int time_index = TIME_INDEX_NOT_DEFINED;

  QFile file(info->filename);
  file.open(QFile::ReadOnly);

  std::vector<std::string> column_names;

  const QSize table_size = parseHeader(&file, column_names);
  const int tot_lines = table_size.height() - 1;
  const int columncount = table_size.width();

  file.close();
  file.open(QFile::ReadOnly);
  QTextStream inB(&file);

  std::vector<PlotData*> plots_vector;

  bool interrupted = false;

  int linecount = 0;

  QProgressDialog progress_dialog;
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, tot_lines - 1);
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  // remove first line (header)
  inB.readLine();

  //---- build plots_vector from header  ------
  std::deque<std::string> valid_field_names;

  for (unsigned i = 0; i < column_names.size(); i++)
  {
    const std::string& field_name = (column_names[i]);

    auto it = plot_data.addNumeric(field_name);

    valid_field_names.push_back(field_name);
    plots_vector.push_back(&(it->second));

    if (time_index == TIME_INDEX_NOT_DEFINED && use_provided_configuration)
    {
      if (_default_time_axis == field_name)
      {
        time_index = i;
      }
    }
  }

  if (time_index == TIME_INDEX_NOT_DEFINED && !use_provided_configuration)
  {
    valid_field_names.push_front("INDEX (auto-generated)");

    SelectFromListDialog* dialog = new SelectFromListDialog(valid_field_names);
    dialog->setWindowTitle("Select the time axis");
    int res = dialog->exec();

    if (res == QDialog::Rejected)
    {
      return false;
    }

    const int selected_item = dialog->getSelectedRowNumber().at(0);
    if (selected_item > 0)
    {
      for (unsigned i = 0; i < column_names.size(); i++)
      {
        if (column_names[i] == valid_field_names[selected_item])
        {
          _default_time_axis = column_names[i];
          time_index = selected_item - 1;
          break;
        }
      }
    }
  }

  //-----------------
  double prev_time = -std::numeric_limits<double>::max();
  bool monotonic_warning = false;
  bool to_string_parse = false;
  QString format_string = "";


  while (!inB.atEnd())
  {
    QString line = inB.readLine();

    QStringList string_items = line.split(csv_separator);
    if (string_items.size() != columncount)
    {
      auto err_msg = QString("The number of values at line %1 is %2,\n"
                             "but the expected number of columns is %3.\n"
                             "Aborting...")
          .arg(linecount+1)
          .arg(string_items.size())
          .arg(columncount);

      QMessageBox::warning(nullptr, "Error reading file", err_msg );
      return false;
    }
    double t = linecount;

    if (time_index >= 0)
    {
      bool is_number = false;
      if (!to_string_parse)
      {
          t = string_items[time_index].toDouble(&is_number);
          if (!is_number)
          {
              to_string_parse = true;
          }
      }

      if (!is_number || to_string_parse)
      {
          if (format_string.isEmpty())
          {
              bool ok;
              format_string = QInputDialog::getText(nullptr, tr("Error reading file"),
                      tr("One of the timestamps is not a valid number, please enter a Format String - see QDateTime::fromString"),
                      QLineEdit::Normal, "yyyy-MM-dd hh:mm:ss", &ok);

              if (!ok || format_string.isEmpty())
              {
                  return false;
              }
          }

          QDateTime ts = QDateTime::fromString(string_items[time_index], format_string);
          is_number = ts.isValid();
          if (!is_number)
          {
              QMessageBox::StandardButton reply;
              reply = QMessageBox::warning(0, tr("Error reading file"),
                      tr("Couldn't parse timestamp. Aborting.\n"));

              return false;
          }
          t = ts.toMSecsSinceEpoch()/1000.0;
      }


      if (t < prev_time)
      {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(0, tr("Error reading file"),
                                     tr("Selected time in not strictly monotonic. Loading will be aborted\n"));

        return false;
      }
      else if (t == prev_time)
      {
        monotonic_warning = true;
      }

      prev_time = t;
    }

    int index = 0;
    for (int i = 0; i < string_items.size(); i++)
    {
      bool is_number = false;
      double y = string_items[i].toDouble(&is_number);
      if (is_number)
      {
        PlotData::Point point(t, y);
        plots_vector[index]->pushBack(point);
      }
      index++;
    }

    if (linecount++ % 100 == 0)
    {
      progress_dialog.setValue(linecount);
      QApplication::processEvents();
      if (progress_dialog.wasCanceled())
      {
        interrupted = true;
        break;
      }
    }
  }
  file.close();

  if (interrupted)
  {
    progress_dialog.cancel();
    plot_data.numeric.clear();
  }

  if (monotonic_warning)
  {
    QString message = "Two consecutive samples had the same X value (i.e. time).\n"
                      "Since PlotJuggler makes the assumption that timeseries are strictly monotonic, you "
                      "might experience undefined behaviours.\n\n"
                      "You have been warned...";
    QMessageBox::warning(0, tr("Warning"), message);
  }

  return true;
}

DataLoadCSV::~DataLoadCSV()
{
}

bool DataLoadCSV::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("default");
  elem.setAttribute("time_axis", _default_time_axis.c_str());

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadCSV::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("default");
  if (!elem.isNull())
  {
    if (elem.hasAttribute("time_axis"))
    {
      _default_time_axis = elem.attribute("time_axis").toStdString();
      return true;
    }
  }
  return false;
}
