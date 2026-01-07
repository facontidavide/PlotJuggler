#pragma once

#include <QStandardItemModel>
#include "PlotJuggler/dataloader_base.h"
#include "ui_dataload_csv.h"

using namespace PJ;

class DateTimeHelp;

class DataLoadCSV : public DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadCSV();
  virtual const std::vector<const char*>& compatibleFileExtensions() const override;

  virtual bool readDataFromFile(PJ::FileLoadInfo* fileload_info,
                                PlotDataMapRef& destination) override;

  virtual ~DataLoadCSV() override = default;

  virtual const char* name() const override
  {
    return "DataLoad CSV";
  }

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

  void parseHeader(QFile& file, std::vector<std::string>& ordered_names);

  QChar GetDelimiter() const noexcept;

  void SetDelimiter(const char& delimiter) noexcept;

  /**
   * @brief Auto-detect the delimiter used in a CSV line.
   *
   * Analyzes the first line of a CSV file to determine the most likely delimiter.
   * Handles quoted strings correctly (delimiters inside quotes are ignored).
   *
   * @param first_line The first line of the CSV file
   * @return The detected delimiter character, or ',' as default
   */
  static char DetectDelimiter(const QString& first_line);

  static void SplitLine(const QString& line, QChar separator, QStringList& parts);

  void SetDelimiterIndex(int delimiterIndex);

  int GetDelimiterIndex() const noexcept;

  void SetIsCustomTime(bool isCustomTime);

  bool GetIsCustomTime() const noexcept;

  void SetDateFormat(const std::string& dateFormat);

  std::string GetDateFormat() const noexcept;

  static constexpr int TIME_INDEX_NOT_DEFINED = -2;
  static constexpr int TIME_INDEX_GENERATED = -1;
  static constexpr const char* INDEX_AS_TIME = "__TIME_INDEX_GENERATED__";

signals:
  void onWarningOccurred(const QString& title, const QString& message);

  void onParseHeader(const QStringList& lines, const QString& preview_lines,
                     const std::vector<std::string>& column_names);

  void onLaunchDialog(QFile& file, std::vector<std::string>* column_names, int& result);

private:
  std::vector<const char*> _extensions;

  std::string _default_time_axis;

  QChar _delimiter;

  FileLoadInfo* _fileInfo;

  bool multiple_columns_warning_ = true;

  // Used when Saving state
  int _delimiterIndex = -1;

  bool _isCustomTime = false;

  std::string _dateFormat;
};
