#include "dataload_can.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QFileDialog>
#include "fstream"
#include "selectlistdialog.h"

const QRegularExpression can_frame_regexp("\\((\\d*\\.\\d*)\\)\\s{1,2}([A-Za-z0-9]*)\\s([0-9a-fA-F]{3,})\\#([0-9a-fA-F]*)");
const QRegExp csv_separator("(\\,|\\;|\\|)");

DataLoadCAN::DataLoadCAN()
{
  _extensions.push_back("log");
}

const std::vector<const char*>& DataLoadCAN::compatibleFileExtensions() const
{
  return _extensions;
}

bool DataLoadCAN::loadCANDatabase(QString dbc_filename)
{
  // Get dbc file and add frames to dataMap()
  auto dbc_dialog = QFileDialog::getOpenFileUrl().toLocalFile();
  std::ifstream dbc_file{ dbc_dialog.toStdString() };
  can_network_ = dbcppp::Network::loadDBCFromIs(dbc_file);
}

QSize DataLoadCAN::inspectFile(QFile* file)
{
  QTextStream inA(file);
  int linecount = 0;

  while (!inA.atEnd())
  {
    inA.readLine();
    linecount++;
  }

  QSize table_size;
  table_size.setWidth(4);
  table_size.setHeight(linecount);
  auto dbc_dialog = QFileDialog::getOpenFileUrl().toLocalFile();
  std::ifstream dbc_file{ dbc_dialog.toStdString() };
  can_network_ = dbcppp::Network::loadDBCFromIs(dbc_file);

  return table_size;
}

bool DataLoadCAN::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  bool use_provided_configuration = false;

  if (info->plugin_config.hasChildNodes())
  {
    use_provided_configuration = true;
    xmlLoadState(info->plugin_config.firstChildElement());
  }

  const int TIME_INDEX_NOT_DEFINED = -2;

  int time_index = TIME_INDEX_NOT_DEFINED;

  QFile file(info->filename);
  file.open(QFile::ReadOnly);

  std::vector<std::string> column_names;

  const QSize table_size = inspectFile(&file);
  const int tot_lines = table_size.height() - 1;
  const int columncount = table_size.width();

  file.close();
  file.open(QFile::ReadOnly);
  QTextStream inB(&file);

  std::vector<PlotData*> plots_vector;

  bool interrupted = false;

  int linecount = 0;

  QProgressDialog progress_dialog;
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, tot_lines);
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  // Add all signals by name
  can_network_->forEachMessage([&](const dbcppp::Message& msg) 
  {
    msg.forEachSignal([&](const dbcppp::Signal& signal) {
        auto str = QString("can_frames/%1/").arg(msg.getId()).toStdString() + signal.getName();
        plot_data.addNumeric(str);
  });});
  //-----------------
  double prev_time = -std::numeric_limits<double>::max();
  bool monotonic_warning = false;

  while (!inB.atEnd())
  {
    QString line = inB.readLine();
    static QRegularExpressionMatchIterator rxIterator;
    rxIterator = can_frame_regexp.globalMatch(line);
    QRegularExpressionMatch canFrame = rxIterator.next();
    /*
    qCritical() << canFrame.captured(1);  // time
    qCritical() << canFrame.captured(2);  // channel
    qCritical() << canFrame.captured(3);  // FRAME_ID
    qCritical() << canFrame.captured(4);  // DATA
    */
    uint64_t frameId = std::stoul(canFrame.captured(3).toStdString(),0,16);
    uint64_t frameData = std::stoul(canFrame.captured(4).toStdString(),0,16);
    //uint8_t frameDataBytes[4];
    auto frameDataBytes = static_cast<uint8_t*>(static_cast<void*>(&frameData));
    double frameTime = std::stof(canFrame.captured(1).toStdString());
    const dbcppp::Message* msg = can_network_->getMessageById(frameId);
    if (msg)
    {
      msg->forEachSignal([&](const dbcppp::Signal& signal) {
        double decoded_val = signal.rawToPhys(signal.decode(frameDataBytes));
        auto str = QString("can_frames/%1/").arg(frameId).toStdString() + signal.getName();
        auto it = plot_data.numeric.find(str);
        if (it != plot_data.numeric.end())
        {
          auto& plot = it->second;
          plot.pushBack(PlotData::Point(frameTime, decoded_val));
        }
      });
    }
  }
  file.close();

  if (interrupted)
  {
    progress_dialog.cancel();
    plot_data.numeric.clear();
  }

  if (monotonic_warning)
  {
    QString message = "Two consecutive samples had the same X value (i.e. time).\n"
                      "Since PlotJuggler makes the assumption that timeseries are strictly monotonic, you "
                      "might experience undefined behaviours.\n\n"
                      "You have been warned...";
    QMessageBox::warning(0, tr("Warning"), message);
  }

  return true;
}

DataLoadCAN::~DataLoadCAN()
{
}

bool DataLoadCAN::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("default");
  elem.setAttribute("time_axis", _default_time_axis.c_str());

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadCAN::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("default");
  if (!elem.isNull())
  {
    if (elem.hasAttribute("time_axis"))
    {
      _default_time_axis = elem.attribute("time_axis").toStdString();
      return true;
    }
  }
  return false;
}
