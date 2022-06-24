#include "dataload_parquet.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>
#include <QListWidget>

DataLoadParquet::DataLoadParquet()
{
  ui = new Ui::DialogParquet();
}

DataLoadParquet::~DataLoadParquet()
{
  delete ui;
}

const std::vector<const char*>& DataLoadParquet::compatibleFileExtensions() const
{
  static std::vector<const char*> extensions = {"parquet"};
  return extensions;
}

void DataLoadParquet::setupDialog(QDialog* dialog)
{
  ui->setupUi(dialog);

  connect(ui->checkBoxDateFormat, &QCheckBox::toggled, this,
          [=](bool checked){
            ui->lineEditDateFormat->setEnabled(checked);
          });

  connect(ui->listWidgetSeries, &QListWidget::currentTextChanged, this,
          [=](QString text) {
            ui->buttonBox->setEnabled( !text.isEmpty() );
          });

  connect(ui->listWidgetSeries, &QListWidget::doubleClicked, this,
          [=](const QModelIndex &) {
            dialog->accept();
          });

  connect(ui->radioButtonIndex, &QRadioButton::toggled, this,
          [=](bool checked) {
            ui->buttonBox->setEnabled(checked  );
            ui->listWidgetSeries->setEnabled( !checked );
          });

  QSettings settings;

  bool radio_index_checked = settings.value("DataLoadParquet::radioIndexChecked", false).toBool();
  ui->radioButtonIndex->setChecked( radio_index_checked );

  bool parse_date_time = settings.value("DataLoadParquet::parseDateTime", false).toBool();
  ui->checkBoxDateFormat->setChecked(parse_date_time);

  QString date_format = settings.value("DataLoadParquet::dateFromat", false).toString();
  if (date_format.isEmpty() == false)
  {
    ui->lineEditDateFormat->setText(date_format);
  }
}

bool DataLoadParquet::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  using parquet::Type;
  using parquet::ConvertedType;

  parquet_reader_ = parquet::ParquetFileReader::OpenFile(info->filename.toStdString());

  if( !parquet_reader_ )
  {
    return false;
  }

  std::shared_ptr<parquet::FileMetaData> file_metadata = parquet_reader_->metadata();
  const auto schema = file_metadata->schema();
  const size_t num_columns = file_metadata->num_columns();

  auto dialog = new QDialog();
  setupDialog(dialog);

  std::vector<parquet::Type::type> column_type;
  std::vector<parquet::ConvertedType::type> converted_column_type;
  std::vector<bool> valid_column( num_columns, true );

  for( size_t col=0; col<num_columns; col++ )
  {
    auto column =  schema->Column(col);
    auto type = column->physical_type();
    auto converted_type = column->converted_type();

    column_type.push_back( type );
    converted_column_type.push_back( converted_type );

    valid_column[col] = (type == Type::BOOLEAN ||
                         type == Type::INT32 ||
                         type == Type::INT64 ||
                         type == Type::FLOAT ||
                         type == Type::DOUBLE);

    ui->listWidgetSeries->addItem( QString::fromStdString(column->name()) );
  }

  {
    QSettings settings;
    if( _default_time_axis.isEmpty() )
    {
      _default_time_axis = settings.value("DataLoadParquet::prevTimestamp", {}).toString();
    }

    if (_default_time_axis.isEmpty() == false)
    {
      auto items = ui->listWidgetSeries->findItems(_default_time_axis, Qt::MatchExactly);
      if( items.size() > 0 )
      {
        ui->listWidgetSeries->setCurrentItem(items.front());
      }
    }
  }

  int ret = dialog->exec();
  if (ret != QDialog::Accepted)
  {
    return false;
  }

  QString selected_stamp;

  if( ui->radioButtonSelect->isChecked() )
  {
    auto selected = ui->listWidgetSeries->selectedItems();
    if( selected.size() == 1)
    {
      selected_stamp = selected.front()->text();
    }
  }

  QSettings settings;
  settings.setValue("DataLoadParquet::prevTimestamp", selected_stamp);
  settings.setValue("DataLoadParquet::radioIndexChecked", ui->radioButtonIndex->isChecked());
  settings.setValue("DataLoadParquet::parseDateTime", ui->checkBoxDateFormat->isChecked());
  settings.setValue("DataLoadParquet::dateFromat", ui->lineEditDateFormat->text());

  dialog->deleteLater();

  //-----------------------------
  // Time to parse
  int timestamp_column = -1;

  std::vector<PlotData*> series(num_columns, nullptr);

  for(size_t col=0; col<num_columns; col++)
  {
    auto column =  schema->Column(col);
    const std::string& name = column->name();
    if( name == selected_stamp.toStdString() )
    {
      timestamp_column = col;
    }
    if( valid_column[col] )
    {
      series[col] = &(plot_data.addNumeric(name)->second);
    }
  }

  parquet::StreamReader os{std::move(parquet_reader_)};

  std::vector<double> row_values( num_columns, 0.0 );

  int row = 0;
  while ( !os.eof() )
  {
    // extract an entire row
    for(size_t col=0; col<num_columns; col++)
    {
      if( !valid_column[col] )
      {
        continue;
      }
      auto type = column_type[col];
      auto converted_type = converted_column_type[col];

      switch(type)
      {
        case Type::BOOLEAN: {
          bool tmp;
          os >> tmp;
          row_values[col] = static_cast<double>(tmp);
          break;
        }
        case Type::INT32: 
        case Type::INT64: {
          switch(converted_type)
          {
            case ConvertedType::INT_8:
            {
              int8_t tmp;
              os >> tmp;
              row_values[col] = static_cast<double>(tmp);
              break;
            }
            case ConvertedType::INT_16:
            {
              int16_t tmp;
              os >> tmp;
              row_values[col] = static_cast<double>(tmp);
              break;
            }
            case ConvertedType::INT_32:
            {
              int32_t tmp;
              os >> tmp;
              row_values[col] = static_cast<double>(tmp);
              break;
            }
            case ConvertedType::INT_64:
            {
              int64_t tmp;
              os >> tmp;
              row_values[col] = static_cast<double>(tmp);
              break;
            }
            case ConvertedType::UINT_8:
            {
              uint8_t tmp;
              os >> tmp;
              row_values[col] = static_cast<double>(tmp);
              break;
            }
            case ConvertedType::UINT_16:
            {
              uint16_t tmp;
              os >> tmp;
              row_values[col] = static_cast<double>(tmp);
              break;
            }
            case ConvertedType::UINT_32:
            {
              uint32_t tmp;
              os >> tmp;
              row_values[col] = static_cast<double>(tmp);
              break;
            }
            case ConvertedType::UINT_64:
            {
              uint64_t tmp;
              os >> tmp;
              row_values[col] = static_cast<double>(tmp);
              break;
            }
            default: {
              //Fallback in case no converted type is provided
              switch(type)
              {
                case Type::INT32: 
                {
                  int32_t tmp;
                  os >> tmp;
                  row_values[col] = static_cast<double>(tmp);
                  break;
                }
                case Type::INT64:
                {
                  int64_t tmp;
                  os >> tmp;
                  row_values[col] = static_cast<double>(tmp);
                  break;
                }
              }
            }
          }
          break;
        }
        case Type::FLOAT: {
          float tmp;
          os >> tmp;
          row_values[col] = static_cast<double>(tmp);
          break;
        }
        case Type::DOUBLE: {
          os >> row_values[col];
          break;
        }
        default: {
        }
        } // end switch
    } // end for column

    os >> parquet::EndRow;

    double timestamp = timestamp_column >= 0 ? row_values[timestamp_column] : row;
    row++;

    for(int col=0; col<num_columns; col++)
    {
      if( !valid_column[col] )
      {
        continue;
      }
      auto column =  schema->Column(col);
      const std::string& name = column->name();

      if( valid_column[col] )
      {
        series[col]->pushBack( {timestamp, row_values[col] });
      }
    }
  }
  return true;
}

bool DataLoadParquet::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("default");
  elem.setAttribute("prevTimestamp", _default_time_axis);
  elem.setAttribute("radioIndexChecked", ui->radioButtonIndex->isChecked());
  elem.setAttribute("parseDateTime", ui->checkBoxDateFormat->isChecked());
  elem.setAttribute("dateFromat", ui->lineEditDateFormat->text());

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadParquet::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("default");
  if (!elem.isNull())
  {
    if (elem.hasAttribute("prevTimestamp"))
    {
      _default_time_axis = elem.attribute("prevTimestamp");
    }
    if (elem.hasAttribute("radioIndexChecked"))
    {
      bool checked = elem.attribute("radioIndexChecked").toInt();
      ui->radioButtonIndex->setChecked(checked);
    }
    if (elem.hasAttribute("parseDateTime"))
    {
      bool checked = elem.attribute("parseDateTime").toInt();
      ui->checkBoxDateFormat->setChecked(checked);
    }
    if (elem.hasAttribute("dateFromat"))
    {
      ui->lineEditDateFormat->setText( elem.attribute("dateFromat") );
    }
  }

  return true;
}
