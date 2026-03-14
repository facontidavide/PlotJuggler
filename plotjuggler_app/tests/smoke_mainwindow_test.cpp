#include <QtTest/QtTest>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QDomDocument>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextStream>
#include <exception>
#include <vector>

#include "curvetree_view.h"
#include "mainwindow.h"

namespace
{
QString traceFilePath()
{
  return QDir(QStringLiteral(PJ_TEST_BINARY_DIR)).filePath("plotjuggler_smoke_trace.txt");
}

void appendTrace(const QString& message)
{
  QFile file(traceFilePath());
  if (file.open(QIODevice::Append | QIODevice::Text))
  {
    QTextStream stream(&file);
    stream << message << '\n';
  }
}

void traceMessageHandler(QtMsgType, const QMessageLogContext&, const QString& message)
{
  appendTrace(QStringLiteral("[qt] %1").arg(message));
}

void configureParser(QCommandLineParser& parser)
{
  parser.addOption(QCommandLineOption(QStringList() << "t" << "test"));
  parser.addOption(QCommandLineOption(QStringList() << "d" << "datafile", {}, "file_path"));
  parser.addOption(QCommandLineOption(QStringList() << "l" << "layout", {}, "file_path"));
  parser.addOption(QCommandLineOption(QStringList() << "p" << "publish"));
  parser.addOption(QCommandLineOption(QStringList() << "plugin_folders", {}, "directory_paths"));
  parser.addOption(QCommandLineOption(QStringList() << "buffer_size", {}, "seconds"));
  parser.addOption(QCommandLineOption(QStringList() << "enabled_plugins", {}, "name_list"));
  parser.addOption(QCommandLineOption(QStringList() << "disabled_plugins", {}, "name_list"));
  parser.addOption(QCommandLineOption(QStringList() << "skin_path", {}, "path"));
  parser.addOption(QCommandLineOption(QStringList() << "start_streamer", {}, "name"));
  parser.addOption(QCommandLineOption(QStringList() << "window_title", {}, "window_title"));
  const QString plugin_dir = QDir(QStringLiteral(PJ_TEST_BINARY_DIR)).filePath("bin");
  parser.process(QStringList{ QStringLiteral("plotjuggler_smoke_qttest"),
                              QStringLiteral("--plugin_folders"),
                              plugin_dir });
}

PJ::FileLoadInfo configuredCsvLoad(const QString& sample_file, const QString& time_axis)
{
  PJ::FileLoadInfo info;
  info.filename = sample_file;

  QDomElement plugin = info.plugin_config.createElement(QStringLiteral("plugin"));
  QDomElement parameters = info.plugin_config.createElement(QStringLiteral("parameters"));
  parameters.setAttribute(QStringLiteral("time_axis"), time_axis);
  parameters.setAttribute(QStringLiteral("delimiter"), QStringLiteral("0"));
  plugin.appendChild(parameters);
  info.plugin_config.appendChild(plugin);

  return info;
}
}  // namespace

class MainWindowSmokeTest : public QObject
{
  Q_OBJECT

private slots:
  void startsLoadsPlotsAndCloses();
};

void MainWindowSmokeTest::startsLoadsPlotsAndCloses()
{
  appendTrace("test:start");
  const QString sample_file =
      QDir(QStringLiteral(PJ_TEST_SOURCE_DIR)).filePath("datasamples/simple.csv");
  QVERIFY2(QFileInfo::exists(sample_file), qPrintable(sample_file));
  appendTrace(QStringLiteral("sample:%1").arg(sample_file));

  QCommandLineParser parser;
  configureParser(parser);
  appendTrace("parser:configured");
  std::unique_ptr<MainWindow> window_ptr;
  try
  {
    window_ptr = std::make_unique<MainWindow>(parser);
  }
  catch (const std::exception& ex)
  {
    appendTrace(QStringLiteral("mainwindow:exception:%1").arg(ex.what()));
    throw;
  }
  catch (...)
  {
    appendTrace("mainwindow:exception:unknown");
    throw;
  }
  MainWindow& window = *window_ptr;
  appendTrace("mainwindow:constructed");
  window.show();
  appendTrace("mainwindow:shown");

  QVERIFY(QTest::qWaitForWindowExposed(&window, 3000) || window.isVisible());
  appendTrace("mainwindow:exposed");

  const auto loaded_names = window.loadDataFromFile(
      configuredCsvLoad(sample_file, QStringLiteral("timestamp")), false);
  QVERIFY(!loaded_names.empty());
  appendTrace("data:loaded");
  for (const auto& name : loaded_names)
  {
    appendTrace(QStringLiteral("data:name=%1").arg(QString::fromStdString(name)));
  }

  auto* curve_tree = window.findChild<CurveTreeView*>("curveTreeView");
  QVERIFY(curve_tree != nullptr);
  QTRY_VERIFY(curve_tree->topLevelItemCount() > 0);
  appendTrace(QStringLiteral("curve_tree:items=%1").arg(curve_tree->topLevelItemCount()));

  const auto plots = window.findChildren<PlotWidget*>();
  QVERIFY(!plots.isEmpty());
  appendTrace(QStringLiteral("plots:count=%1").arg(plots.size()));

  std::vector<std::string> numeric_names;
  numeric_names.reserve(loaded_names.size());
  for (const auto& name : loaded_names)
  {
    if (name != "timestamp")
    {
      numeric_names.push_back(name);
    }
  }
  QVERIFY(numeric_names.size() >= 2);

  PlotWidget* plot_with_curves = nullptr;
  for (auto* plot : plots)
  {
    appendTrace(QStringLiteral("plot:try visible=%1").arg(plot->isVisible()));
    auto* first = plot->addCurve(numeric_names[0]);
    auto* second = plot->addCurve(numeric_names[1]);
    if (first != nullptr && second != nullptr)
    {
      plot->updateCurves(true);
      plot->replot();
      plot_with_curves = plot;
      appendTrace(QStringLiteral("plot:curves_added=%1,%2")
                      .arg(QString::fromStdString(numeric_names[0]),
                           QString::fromStdString(numeric_names[1])));
      break;
    }
  }

  QVERIFY(plot_with_curves != nullptr);
  QTRY_VERIFY(plot_with_curves->curveList().size() >= 2);
  appendTrace(QStringLiteral("plot:curve_count=%1").arg(plot_with_curves->curveList().size()));

  window.close();
  QTRY_VERIFY(!window.isVisible());
  appendTrace("mainwindow:closed");
}

int main(int argc, char* argv[])
{
  QFile::remove(traceFilePath());
  appendTrace("main:starting");
  qInstallMessageHandler(traceMessageHandler);

  QApplication app(argc, argv);
  appendTrace("main:qapplication");
  QCoreApplication::setOrganizationName("PlotJuggler");
  QCoreApplication::setApplicationName("io.plotjuggler.PlotJuggler.QtTest");
  QSettings::setDefaultFormat(QSettings::IniFormat);

  QTemporaryDir settings_dir;
  if (!settings_dir.isValid())
  {
    appendTrace("main:settings_dir_invalid");
    qCritical() << "Unable to create temporary settings directory";
    return 1;
  }
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings_dir.path());
  appendTrace(QStringLiteral("main:settings_dir=%1").arg(settings_dir.path()));

  MainWindowSmokeTest test;
  appendTrace("main:qexec");
  const int result = QTest::qExec(&test, argc, argv);
  appendTrace(QStringLiteral("main:result=%1").arg(result));
  return result;
}

#include "smoke_mainwindow_test.moc"
