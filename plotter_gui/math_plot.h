#pragma once

#include <memory>
#include <string>
#include <QWidget>
#include <QString>
#include <include/PlotJuggler/plotdata.h>

class MathPlot;
class QJSEngine;
typedef std::shared_ptr<MathPlot> MathPlotPtr;

class MathPlot
{
public:
    MathPlot(const std::string &linkedPlot, const std::string &plotName, const QString &globalVars, const QString &equation);

    void calc(PlotDataMapRef &plotData, PlotData &out);

    const std::string& getName();
    const std::string& getLinkedPlot();
    const QString& getGlobalVars();
    const QString& getEquation();

private:
    void addJavascriptDependencies(QJSEngine &engine);

    std::string _linkedPlot;
    std::string _plotName;
    QString _globalVars;
    QString _calcEquation;
};

