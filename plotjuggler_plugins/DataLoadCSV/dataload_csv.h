#pragma once

#include <QStandardItemModel>
#include "PlotJuggler/dataloader_base.h"
#include "ui_dataload_csv.h"

#include <QMessageBox>
#include <QCSVHighlighter>
#include <QProgressDialog>

using namespace PJ;

class DateTimeHelp;

class DataLoadCSV : public DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadCSV();
  const std::vector<const char*>& compatibleFileExtensions() const override;

  bool readDataFromFile(FileLoadInfo* fileload_info, PlotDataMapRef& destination) override;

  ~DataLoadCSV() override;

  const char* name() const override
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

private:
  static void ShowWarningMessageBox(const QString& title, const QString& message,
                                    QMessageBox::StandardButton& result);

  static void ShowWarningMessageBoxSkippedLines(const QString& title, const QString& message,
                                                const QString& detailedText);

  static void ShowWarningMessageBoxNonMonotonicTime(const QString& title, const QString& message,
                                                    const QString& detailedText,
                                                    bool& sortButtonClicked);
  void SetupUI();

  void parseHeaderLogic(QFile& file, std::vector<std::string>& column_names);
  void parseHeaderUI(const std::vector<std::string>& column_names);

  // The UI counterpart is readDataFromFile
  bool readDataFromFileLogic(FileLoadInfo* info, PlotDataMapRef& plot_data);

  // UI callbacks
  using LaunchDialogCallback =
      std::function<int(QFile& file, std::vector<std::string>* column_names)>;
  LaunchDialogCallback _launchDialogCallback;
  int launchDialog(QFile& file, std::vector<std::string>* column_names);
  int LaunchDialogUI(QFile& file, std::vector<std::string>* column_names) const;

  using OpenProgressDialogCallback = std::function<void(QFile& file)>;
  OpenProgressDialogCallback _openProgressDialogCallback;
  void OpenProgressDialogUI(QFile& file) const;
  void openProgressDialog(QFile& file);

  using UpdateProgressDialogCallback = std::function<void(int progress)>;
  UpdateProgressDialogCallback _updateProgressDialogCallback;
  void UpdateProgressDialogUI(int progress) const;

  using IsProgressBarCancelled = std::function<bool()>;
  IsProgressBarCancelled _isProgressBarCancelled;
  bool WasProgressBarCancelledUI() const;

  using WarningMessageBoxCallback = std::function<QMessageBox::StandardButton(
      const QString& title, const QString& text, const QString& detailedText)>;
  WarningMessageBoxCallback _warningMessageBoxCallback;
  QMessageBox::StandardButton OpenWarningMessageBoxUI(const QString& title, const QString& text,
                                                      const QString& detailedText = QString());
  QMessageBox::StandardButton openWarningMessageBox(const QString& title, const QString& text,
                                                    const QString& detailedText = QString());

  using WarningMessageBoxNonMonotonicTime =
      std::function<bool(const QString& title, const QString& text, const QString& detailedText)>;
  WarningMessageBoxNonMonotonicTime _warningMessageBoxNonMonotonicTime;
  bool OpenWarningMessageBoxNonMonotonicTimeUI(const QString& title, const QString& text,
                                               const QString& detailedText);
  bool openWarningMessageBoxNonMonotonicTime(const QString& title, const QString& text,
                                             const QString& detailedText);

  //---------------------------------------------------------------------------------//

  std::vector<const char*> _extensions;
  std::string _default_time_axis;
  QChar _delimiter;
  FileLoadInfo* _fileInfo = nullptr;
  bool multiple_columns_warning_ = true;
  // Used when Saving state
  int _delimiterIndex = -1;  // TODO: check if we can remove this
  bool _isCustomTime = false;
  bool _triggerMultipleColumnsWarning = false;
  std::string _dateFormat;

  QDialog* _dialog;
  Ui::DialogCSV* _ui;
  DateTimeHelp* _dateTime_dialog;
  QCSVHighlighter _csvHighlighter;
  QStandardItemModel* _model;
  QStringList _lines = QStringList();
  QString _preview_lines = QString();
};
