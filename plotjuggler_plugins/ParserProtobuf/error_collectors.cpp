#include "error_collectors.h"
#include <QMessageBox>
#include <QDebug>

void FileErrorCollector::RecordError(const absl::string_view filename, int line, int,
                                  const absl::string_view message)
{
  auto msg = QString("File: [%1] Line: [%2] Message: %3\n\n")
                 .arg(QString::fromStdString(std::string(filename)))
                 .arg(line)
                 .arg(QString::fromStdString(std::string(message)));

  _errors.push_back(msg);
}

void FileErrorCollector::RecordWarning(const absl::string_view filename, int line, int,
                                    const absl::string_view message)
{
  auto msg = QString("Warning [%1] line %2: %3")
                 .arg(QString::fromStdString(std::string(filename)))
                 .arg(line)
                 .arg(QString::fromStdString(std::string(message)));
  qDebug() << msg;
}

void IoErrorCollector::AddError(int line, google::protobuf::io::ColumnNumber column,
                                const std::string& message)
{
  _errors.push_back(
      QString("Line: [%1] Message: %2\n").arg(line).arg(QString::fromStdString(message)));
}

void IoErrorCollector::AddWarning(int line, google::protobuf::io::ColumnNumber column,
                                  const std::string& message)
{
  qDebug() << QString("Line: [%1] Message: %2\n")
                  .arg(line)
                  .arg(QString::fromStdString(message));
}
