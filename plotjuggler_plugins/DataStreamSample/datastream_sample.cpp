#include "datastream_sample.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <thread>
#include <mutex>
#include <chrono>
#include <thread>
#include <math.h>

using namespace PJ;

DataStreamSample::DataStreamSample()
{
  _dummy_notification = new QAction(this);

  connect(_dummy_notification, &QAction::triggered, this, [this]() {
    QMessageBox::warning(nullptr, "Dummy Notifications",
                         QString("%1 notifications").arg(_notifications_count),
                         QMessageBox::Ok);

    if (_notifications_count > 0)
    {
      _notifications_count = 0;
      emit notificationsChanged(_notifications_count);
    }
  });

  _notifications_count = 0;
  for (int i = 0; i < 150; i++)
  {
    auto str = QString("data_vect/%1").arg(i).toStdString();
    DataStreamSample::Parameters param;
    param.A = 6 * ((double)rand() / (double)RAND_MAX) - 3;
    param.B = 3 * ((double)rand() / (double)RAND_MAX);
    param.C = 3 * ((double)rand() / (double)RAND_MAX);
    param.D = 20 * ((double)rand() / (double)RAND_MAX);
    _parameters.insert({ str, param });
    auto& plotdata = dataMap().addNumeric(str)->second;
  }
  //------------
  dataMap().addStringSeries("color");

  //------------
  auto tcGroup = std::make_shared<PJ::PlotGroup>("tc");
  tcGroup->setAttribute(TEXT_COLOR, QColor(Qt::blue));

  auto& tc_default = dataMap().addNumeric("tc/default")->second;
  auto& tc_red = dataMap().addNumeric("tc/red")->second;

  tc_red.setAttribute(TEXT_COLOR, QColor(Qt::red));
}

bool DataStreamSample::start(QStringList*)
{
  _running = true;
  pushSingleCycle();
  _thread = std::thread([this]() { this->loop(); });
  return true;
}

void DataStreamSample::shutdown()
{
  _running = false;
  if (_thread.joinable())
  {
    _thread.join();
  }
}

bool DataStreamSample::isRunning() const
{
  return _running;
}

DataStreamSample::~DataStreamSample()
{
  shutdown();
}

bool DataStreamSample::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  return true;
}

bool DataStreamSample::xmlLoadState(const QDomElement& parent_element)
{
  return true;
}

void DataStreamSample::pushSingleCycle()
{
  static int count = 0;
  std::lock_guard<std::mutex> lock(mutex());

  using namespace std::chrono;
  static auto initial_time = high_resolution_clock::now();
  const double offset =
      duration_cast<duration<double>>(initial_time.time_since_epoch()).count();

  auto now = high_resolution_clock::now();
  std::string colors[] = { "RED", "BLUE", "GREEN" };

  const double stamp =
      duration_cast<duration<double>>(now - initial_time).count() + offset;

  for (auto& it : _parameters)
  {
    auto& plot = dataMap().numeric.find(it.first)->second;
    const DataStreamSample::Parameters& param = it.second;

    double val = param.A * sin(param.B * stamp + param.C) + param.D;
    plot.pushBack(PlotData::Point(stamp, val));
  }

  auto& col_series = dataMap().strings.find("color")->second;
  col_series.pushBack({ stamp, colors[(count / 10) % 3] });

  auto& tc_default = dataMap().numeric.find("tc/default")->second;
  tc_default.pushBack({ stamp, double(count) });

  auto& tc_red = dataMap().numeric.find("tc/red")->second;
  tc_red.pushBack({ stamp, double(count) });

  count++;
}

void DataStreamSample::loop()
{
  _running = true;
  size_t count = 1;
  while (_running)
  {
    auto prev = std::chrono::high_resolution_clock::now();
    pushSingleCycle();
    emit dataReceived();
    if (count++ % 200 == 0)
    {
      _notifications_count++;
      emit notificationsChanged(_notifications_count);
    }
    std::this_thread::sleep_until(prev + std::chrono::milliseconds(20));  // 50 Hz
  }
}
