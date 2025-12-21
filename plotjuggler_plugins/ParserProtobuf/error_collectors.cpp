#include "error_collectors.h"
#include <QMessageBox>
#include <QDebug>

namespace
{
// Helper function to convert string parameter to QString, handling both std::string and
// absl::string_view
#if GOOGLE_PROTOBUF_VERSION >= 4022000
QString protobufStringToQString(absl::string_view str)
{
  return QString::fromStdString(std::string(str));
}
#else
QString protobufStringToQString(const std::string& str)
{
  return QString::fromStdString(str);
}
#endif
}  // anonymous namespace

#if GOOGLE_PROTOBUF_VERSION >= 4022000
void FileErrorCollector::RecordError(absl::string_view filename, int line, int,
                                     absl::string_view message)
#else
void FileErrorCollector::AddError(const std::string& filename, int line, int,
                                  const std::string& message)
#endif
{
  auto msg = QString("File: [%1] Line: [%2] Message: %3\n\n")
                 .arg(protobufStringToQString(filename))
                 .arg(line)
                 .arg(protobufStringToQString(message));

  _errors.push_back(msg);
}

#if GOOGLE_PROTOBUF_VERSION >= 4022000
void FileErrorCollector::RecordWarning(absl::string_view filename, int line, int,
                                       absl::string_view message)
#else
void FileErrorCollector::AddWarning(const std::string& filename, int line, int,
                                    const std::string& message)
#endif
{
  auto msg = QString("Warning [%1] line %2: %3")
                 .arg(protobufStringToQString(filename))
                 .arg(line)
                 .arg(protobufStringToQString(message));
  qDebug() << msg;
}

#if GOOGLE_PROTOBUF_VERSION >= 4022000
void IoErrorCollector::RecordError(int line, google::protobuf::io::ColumnNumber,
                                   absl::string_view message)
#else
void IoErrorCollector::AddError(int line, google::protobuf::io::ColumnNumber,
                                const std::string& message)
#endif
{
  _errors.push_back(
      QString("Line: [%1] Message: %2\n").arg(line).arg(protobufStringToQString(message)));
}

#if GOOGLE_PROTOBUF_VERSION >= 4022000
void IoErrorCollector::RecordWarning(int line, google::protobuf::io::ColumnNumber column,
                                     absl::string_view message)
#else
void IoErrorCollector::AddWarning(int line, google::protobuf::io::ColumnNumber column,
                                  const std::string& message)
#endif
{
  qDebug() << QString("Line: [%1] Message: %2\n").arg(line).arg(protobufStringToQString(message));
}
