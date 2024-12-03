#include "error_collectors.h"
#include <QMessageBox>
#include <QDebug>

void FileErrorCollector::RecordError(const absl::string_view filename, int line, int,
                                     const absl::string_view message)
{
  auto msg = QString("File: [%1] Line: [%2] Message: %3\n\n")
                 .arg(QString::fromStdString(std::string{ filename }))
                 .arg(line)
                 .arg(QString::fromStdString(std::string{ message }));

  _errors.push_back(msg);
}

void FileErrorCollector::RecordWarning(const absl::string_view filename, int line, int,
                                       const absl::string_view message)
{
  auto msg = QString("Warning [%1] line %2: %3")
                 .arg(QString::fromStdString(std::string{ filename }))
                 .arg(line)
                 .arg(QString::fromStdString(std::string{ message }));
  qDebug() << msg;
}

void IoErrorCollector::RecordError(int line, google::protobuf::io::ColumnNumber,
                                   const absl::string_view message)
{
  _errors.push_back(QString("Line: [%1] Message: %2\n")
                        .arg(line)
                        .arg(QString::fromStdString(std::string{ message })));
}

void IoErrorCollector::RecordWarning(int line, google::protobuf::io::ColumnNumber column,
                                     const absl::string_view message)
{
  qDebug() << QString("Line: [%1] Message: %2\n")
                  .arg(line)
                  .arg(QString::fromStdString(std::string{ message }));
}
