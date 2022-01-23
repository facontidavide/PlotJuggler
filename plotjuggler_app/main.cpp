#include "mainwindow.h"
#include <iostream>
#include <QApplication>
#include <QSplashScreen>
#include <QThread>
#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QDir>

#include "PlotJuggler/transform_function.h"
#include "transforms/first_derivative.h"
#include "transforms/scale_transform.h"
#include "transforms/moving_average_filter.h"
#include "transforms/moving_rms.h"
#include "transforms/outlier_removal.h"
#include "transforms/integral_transform.h"

#include "nlohmann_parsers.h"
#include "new_release_dialog.h"

static QString VERSION_STRING =
    QString("%1.%2.%3").arg(PJ_MAJOR_VERSION).arg(PJ_MINOR_VERSION).arg(PJ_PATCH_VERSION);

inline int GetVersionNumber(QString str)
{
  QStringList online_version = str.split('.');
  if (online_version.size() != 3)
  {
    return 0;
  }
  int major = online_version[0].toInt();
  int minor = online_version[1].toInt();
  int patch = online_version[2].toInt();
  return major * 10000 + minor * 100 + patch;
}

void OpenNewReleaseDialog(QNetworkReply* reply)
{
  if (reply->error())
  {
    return;
  }
  QString answer = reply->readAll();
  QJsonDocument document = QJsonDocument::fromJson(answer.toUtf8());
  QJsonObject data = document.object();
  QString url = data["html_url"].toString();
  QString name = data["name"].toString();
  QString tag_name = data["tag_name"].toString();
  QSettings settings;
  int online_number = GetVersionNumber(tag_name);
  QString dont_show =
      settings.value("NewRelease/dontShowThisVersion", VERSION_STRING).toString();
  int dontshow_number = GetVersionNumber(dont_show);
  int current_number = GetVersionNumber(VERSION_STRING);

  if (online_number > current_number && online_number > dontshow_number)
  {
    NewReleaseDialog* dialog = new NewReleaseDialog(nullptr, tag_name, name, url);
    dialog->show();
  }
}

QPixmap getFunnySplashscreen()
{
  QSettings settings;
  srand(time(nullptr));

  auto getNum = []() {
    const int last_image_num = 71;
    int n = rand() % (last_image_num + 2);
    if (n > last_image_num)
    {
      n = 0;
    }
    return n;
  };
  std::set<int> previous_set;
  std::list<int> previous_nums;

  QStringList previous_list = settings.value("previousFunnyMemesList").toStringList();
  for(auto str: previous_list)
  {
    int num = str.toInt();
    previous_set.insert(num);
    previous_nums.push_back(num);
  }

  int n = getNum();
  while (previous_set.count(n) != 0)
  {
    n = getNum();
  }

  while(previous_nums.size() >= 10)
  {
    previous_nums.pop_front();
  }
  previous_nums.push_back(n);

  QStringList new_list;
  for(int num: previous_nums)
  {
    new_list.push_back( QString::number(num) );
  }

  settings.setValue("previousFunnyMemesList", new_list);
  auto filename = QString("://resources/memes/meme_%1.jpg").arg(n, 2, 10, QChar('0'));
  return QPixmap(filename);
}

std::pair<int, char**> MergeArguments(int argc, char* argv[])
{
#ifdef PJ_DEFAULT_ARGS
  auto default_cmdline_args =
      QString(PJ_DEFAULT_ARGS).split(" ", QString::SkipEmptyParts);
  int new_argc = argc + default_cmdline_args.size();
  static char* new_argv[100];

  // preserve arg[0] => executable path
  new_argv[0] = argv[0];

  // Add the remain arguments, replacing escaped characters if necessary.
  // Escaping needed because some chars cannot be entered easily in the -DPJ_DEFAULT_ARGS
  // preprocessor directive
  //   _0x20_   -->   ' '   (space)
  //   _0x3b_   -->   ';'   (semicolon)
  int index = 1;
  for (auto cmdline_arg : default_cmdline_args)
  {
    // replace(const QString &before, const QString &after, Qt::CaseSensitivity cs =
    // Qt::CaseSensitive)
    cmdline_arg = cmdline_arg.replace("_0x20_", " ", Qt::CaseSensitive);
    cmdline_arg = cmdline_arg.replace("_0x3b_", ";", Qt::CaseSensitive);
    new_argv[index++] = strdup(cmdline_arg.toLocal8Bit().data());
  }

  // If an argument appears repeated, the second value overrides previous one.
  // Do this after adding default_cmdline_args so the command-line overide default
  for (int i = 1; i < argc; ++i)
  {
    new_argv[index++] = argv[i];
  }

  return { new_argc, new_argv };

#else
  return { argc, argv };
#endif
}

int main(int argc, char* argv[])
{
  auto arg = MergeArguments(argc, argv);
  QApplication app(arg.first, arg.second);

  QCoreApplication::setOrganizationName("PlotJuggler");
  QCoreApplication::setApplicationName("PlotJuggler-3");
  QSettings::setDefaultFormat(QSettings::IniFormat);

  QSettings settings;

  if (!settings.isWritable())
  {
    qDebug() << "ERROR: the file [" << settings.fileName()
             << "] is not writable. This may happen when you run PlotJuggler with sudo. "
                "Change the permissions of the file (\"sudo chmod 666 <file_name>\"on "
                "linux)";
  }

  app.setApplicationVersion(VERSION_STRING);

  //---------------------------
  TransformFactory::registerTransform<FirstDerivative>();
  TransformFactory::registerTransform<ScaleTransform>();
  TransformFactory::registerTransform<MovingAverageFilter>();
  TransformFactory::registerTransform<MovingRMS>();
  TransformFactory::registerTransform<OutlierRemovalFilter>();
  TransformFactory::registerTransform<IntegralTransform>();
  //---------------------------

  QCommandLineParser parser;
  parser.setApplicationDescription("PlotJuggler: the time series visualization tool that "
                                   "you deserve ");
  parser.addVersionOption();
  parser.addHelpOption();

  QCommandLineOption nosplash_option(QStringList() << "n"
                                                   << "nosplash",
                                     "Don't display the splashscreen");
  parser.addOption(nosplash_option);

  QCommandLineOption test_option(QStringList() << "t"
                                               << "test",
                                 "Generate test curves at startup");
  parser.addOption(test_option);

  QCommandLineOption loadfile_option(QStringList() << "d"
                                                   << "datafile",
                                     "Load a file containing data", "file_path");
  parser.addOption(loadfile_option);

  QCommandLineOption layout_option(QStringList() << "l"
                                                 << "layout",
                                   "Load a file containing the layout configuration",
                                   "file_path");
  parser.addOption(layout_option);

  QCommandLineOption publish_option(QStringList() << "p"
                                                  << "publish",
                                    "Automatically start publisher when loading the "
                                    "layout file");
  parser.addOption(publish_option);

  QCommandLineOption folder_option(QStringList() << "plugin_folders",
                                   "Add semicolon-separated list of folders where you "
                                   "should look "
                                   "for additional plugins.",
                                   "directory_paths");
  parser.addOption(folder_option);

  QCommandLineOption buffersize_option(QStringList() << "buffer_size",
                                       QCoreApplication::translate("main", "Change the "
                                                                           "maximum size "
                                                                           "of the "
                                                                           "streaming "
                                                                           "buffer "
                                                                           "(minimum: 10 "
                                                                           "default: "
                                                                           "60)"),
                                       QCoreApplication::translate("main", "seconds"));

  parser.addOption(buffersize_option);

  QCommandLineOption nogl_option(QStringList() << "disable_opengl", "Disable OpenGL "
                                                                    "display before "
                                                                    "starting the "
                                                                    "application. "
                                                                    "You can enable it "
                                                                    "again in the "
                                                                    "'Preferences' "
                                                                    "menu.");

  parser.addOption(nogl_option);

  QCommandLineOption enabled_plugins_option(QStringList() << "enabled_plugins",
                                            "Limit the loaded plugins to ones in the "
                                            "semicolon-separated list",
                                            "name_list");
  parser.addOption(enabled_plugins_option);

  QCommandLineOption disabled_plugins_option(QStringList() << "disabled_plugins",
                                             "Do not load any of the plugins in the "
                                             "semicolon separated list",
                                             "name_list");
  parser.addOption(disabled_plugins_option);

  QCommandLineOption skin_path_option(QStringList() << "skin_path",
                                      "New \"skin\". Refer to the sample in "
                                      "[plotjuggler_app/resources/skin]",
                                      "path to folder");
  parser.addOption(skin_path_option);

  QCommandLineOption start_streamer(QStringList() << "start_streamer",
                                    "Automatically start a Streaming Plugin with the "
                                    "give filename",
                                    "file_name (no extension)");
  parser.addOption(start_streamer);

  parser.process(*qApp);

  if (parser.isSet(publish_option) && !parser.isSet(layout_option))
  {
    std::cerr << "Option [ -p / --publish ] is invalid unless [ -l / --layout ] is used "
                 "too."
              << std::endl;
    return -1;
  }

  if (parser.isSet(enabled_plugins_option) && parser.isSet(disabled_plugins_option))
  {
    std::cerr << "Option [ --enabled_plugins ] and [ --disabled_plugins ] can't be used "
                 "together."
              << std::endl;
    return -1;
  }

  if (parser.isSet(nogl_option))
  {
    settings.setValue("Preferences::use_opengl", false);
  }

  if (parser.isSet(skin_path_option))
  {
    QDir path(parser.value(skin_path_option));
    if (!path.exists())
    {
      qDebug() << "Skin path [" << parser.value(skin_path_option) << "] not found";
      return -1;
    }
  }

  QIcon app_icon("://resources/plotjuggler.svg");
  QApplication::setWindowIcon(app_icon);

  QNetworkAccessManager manager;
  QObject::connect(&manager, &QNetworkAccessManager::finished, OpenNewReleaseDialog);

  QNetworkRequest request;
  request.setUrl(QUrl("https://api.github.com/repos/facontidavide/PlotJuggler/releases/"
                      "latest"));
  manager.get(request);

  MainWindow* w = nullptr;

  /*
   * You, fearless code reviewer, decided to start a journey into my source code.
   * For your bravery, you deserve to know the truth.
   * The splashscreen is useless; not only it is useless, it will make your start-up
   * time slower by few seconds for absolutely no reason.
   * But what are two seconds compared with the time that PlotJuggler will save you?
   * The splashscreen is the connection between me and my users, the glue that keeps
   * together our invisible relationship.
   * Now, it is up to you to decide: you can block the splashscreen forever or not,
   * reject a message that brings a little of happiness into your day, spent analyzing
   * data. Please don't do it.
   */

  if (!parser.isSet(nosplash_option) &&
      !(parser.isSet(loadfile_option) || parser.isSet(layout_option)))
  // if(false) // if you uncomment this line, a kitten will die somewhere in the world.
  {
    QPixmap main_pixmap;

    if (parser.isSet(skin_path_option))
    {
      QDir path(parser.value(skin_path_option));
      QFile splash = path.filePath("pj_splashscreen.png");
      if (splash.exists())
      {
        main_pixmap = QPixmap(splash.fileName());
      }
    }

    if (main_pixmap.isNull())
    {
      main_pixmap = getFunnySplashscreen();
    }
    QSplashScreen splash(main_pixmap, Qt::WindowStaysOnTopHint);
    QDesktopWidget* desktop = QApplication::desktop();
    const int scrn = desktop->screenNumber();
    const QPoint currentDesktopsCenter = desktop->availableGeometry(scrn).center();
    splash.move(currentDesktopsCenter - splash.rect().center());

    splash.show();
    app.processEvents();

    auto deadline = QDateTime::currentDateTime().addMSecs(500);
    while (QDateTime::currentDateTime() < deadline)
    {
      app.processEvents();
    }

    w = new MainWindow(parser);

    deadline = QDateTime::currentDateTime().addMSecs(3000);
    while (QDateTime::currentDateTime() < deadline && !splash.isHidden())
    {
      app.processEvents();
    }
  }
  else
  {
    w = new MainWindow(parser);
  }

  w->show();

  if (parser.isSet(start_streamer))
  {
    w->on_buttonStreamingStart_clicked();
  }

  return app.exec();
}
