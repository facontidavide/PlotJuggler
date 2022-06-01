#include "dataload_parquet.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>


DataLoadParquet::DataLoadParquet()
{
}

DataLoadParquet::~DataLoadParquet()
{
  delete _ui;
}

const std::vector<const char*>& DataLoadParquet::compatibleFileExtensions() const
{
  static std::vector<const char*> extensions = {"parquet"};
  return extensions;
}


bool DataLoadParquet::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  return true;
}

bool DataLoadParquet::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  return true;
}

bool DataLoadParquet::xmlLoadState(const QDomElement& parent_element)
{
  return true;
}
