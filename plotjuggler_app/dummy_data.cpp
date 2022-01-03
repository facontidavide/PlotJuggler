#include "dummy_data.h"
#include <QColor>

using namespace PJ;

void BuildDummyData(PlotDataMapRef& datamap)
{
  size_t SIZE = 1000;
  QStringList words_list;
  words_list << "world/siam"
             << "world/tre"
             << "walk/piccoli"
             << "walk/porcellin"
             << "fly/high/mai"
             << "fly/high/nessun"
             << "fly/low/ci"
             << "fly/low/dividera"
             << "data_1"
             << "data_2"
             << "data_3"
             << "data_10";

  for (int i = 0; i < 100; i++)
  {
    words_list.append(QString("data_vect/%1").arg(i));
  }

  for (const QString& name : words_list)
  {
    double A = 6 * ((double)rand() / (double)RAND_MAX) - 3;
    double B = 3 * ((double)rand() / (double)RAND_MAX);
    double C = 3 * ((double)rand() / (double)RAND_MAX);
    double D = 20 * ((double)rand() / (double)RAND_MAX);

    auto it = datamap.addNumeric(name.toStdString());
    PlotData& plot = it->second;

    double t = 0;
    for (unsigned indx = 0; indx < SIZE; indx++)
    {
      t += 0.01;
      plot.pushBack(PlotData::Point(t + 35, A * sin(B * t + C) + D * t * 0.02));
    }
  }

  //-------------------------------
  auto tcGroup = datamap.getOrCreateGroup("myGroup/subGroup");
  tcGroup->setAttribute(PJ::TEXT_COLOR, QColor(Qt::blue));
  tcGroup->setAttribute(PJ::TOOL_TIP, QString("This is a group"));

  auto& tc_default = datamap.addNumeric("color/default", tcGroup)->second;
  auto& tc_red = datamap.addNumeric("color/red", tcGroup)->second;

  tc_red.setAttribute(PJ::TEXT_COLOR, QColor(Qt::red));

  PlotData& sin_plot = datamap.addNumeric("_sin")->second;
  PlotData& cos_plot = datamap.addNumeric("_cos")->second;
  StringSeries& str_plot = datamap.addStringSeries("str_value")->second;

  sin_plot.setAttribute(PJ::TOOL_TIP, QString("sine"));
  cos_plot.setAttribute(PJ::TOOL_TIP, QString("cosine"));
  str_plot.setAttribute(PJ::TOOL_TIP, QString("this is a string"));

  sin_plot.setAttribute(PJ::ITALIC_FONTS, true);

  //--------------------------------
  double t = 0;
  for (unsigned indx = 0; indx < SIZE; indx++)
  {
    t += 0.01;
    sin_plot.pushBack(PlotData::Point(t + 20, sin(t * 0.4)));
    cos_plot.pushBack(PlotData::Point(t + 20, cos(t * 0.4)));
    tc_default.pushBack(PlotData::Point(t + 20, indx));
    tc_default.pushBack(PlotData::Point(t + 20, indx));

    switch (indx % 3)
    {
      case 0:
        str_plot.pushBack({ t + 20, "Blue" });
        break;
      case 1:
        str_plot.pushBack({ t + 20, "Red" });
        break;
      case 2:
        str_plot.pushBack({ t + 20, "Green" });
        break;
    }
  }

  PlotDataXY& octagon = datamap.addScatterXY("octagon_trajectory")->second;
  for (unsigned i = 0; i < 8; i++)
  {
    double angle = double(i) * M_PI / 4;
    octagon.pushBack(PlotData::Point(cos(angle), sin(angle)));
  }
}
