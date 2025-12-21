#ifndef ERROR_COLLECTORS_H
#define ERROR_COLLECTORS_H

#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/port_def.inc>

#include <QStringList>

class IoErrorCollector : public google::protobuf::io::ErrorCollector
{
public:
#if GOOGLE_PROTOBUF_VERSION >= 4022000
  void RecordError(int line, google::protobuf::io::ColumnNumber column,
                   absl::string_view message) override;

  void RecordWarning(int line, google::protobuf::io::ColumnNumber column,
                     absl::string_view message) override;
#else
  void AddError(int line, google::protobuf::io::ColumnNumber column,
                const std::string& message) override;

  void AddWarning(int line, google::protobuf::io::ColumnNumber column,
                  const std::string& message) override;
#endif

  const QStringList& errors()
  {
    return _errors;
  }

private:
  QStringList _errors;
};

class FileErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector
{
public:
#if GOOGLE_PROTOBUF_VERSION >= 4022000
  void RecordError(absl::string_view filename, int line, int, absl::string_view message) override;

  void RecordWarning(absl::string_view filename, int line, int, absl::string_view message) override;
#else
  void AddError(const std::string& filename, int line, int, const std::string& message) override;

  void AddWarning(const std::string& filename, int line, int, const std::string& message) override;
#endif

  const QStringList& errors()
  {
    return _errors;
  }

private:
  QStringList _errors;
};

#endif  // ERROR_COLLECTORS_H
