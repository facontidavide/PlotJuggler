#include "math_plot.h"
#include <QJSEngine>
#include <QFile>
#include <QMessageBox>

MathPlot::MathPlot(const std::string &linkedPlot, const std::string &plotName, const QString &globalVars, const QString &equation):
    _linkedPlot(linkedPlot),
    _plotName(plotName),
    _globalVars(globalVars),
    _calcEquation(equation)
{

}

void MathPlot::calc(PlotDataMapRef &plotData, PlotData &out)
{
    QString qLinkedPlot = QString::fromStdString(_linkedPlot);
    QJSEngine jsEngine;

    addJavascriptDependencies(jsEngine);

    QList<PlotData*> usedChannels;
    QString mathEquationReplaced = _calcEquation;
    while(true)
    {
        int pos1=mathEquationReplaced.indexOf("$$");
        if(pos1 == -1)
            break;

        int pos2 = mathEquationReplaced.indexOf("$$", pos1+2);
        if(pos2 == -1)
        {
            throw std::runtime_error("syntax error : invalid use of $$ macro");
        }

        QString channelName = mathEquationReplaced.mid(pos1+2, pos2-pos1-2);

        if(channelName == qLinkedPlot)
        {
            // special case : user entered linkedPlot ; no need to add another channel
            mathEquationReplaced.replace(QStringLiteral("$$%1$$").arg(channelName), QStringLiteral("y"));
        }
        else
        {
            QString jsExpression = QString("CHANNEL_VALUES[%1]").arg(usedChannels.size());
            mathEquationReplaced.replace(QStringLiteral("$$%1$$").arg(channelName), jsExpression);

            auto plotDataIt = plotData.numeric.find(channelName.toStdString());
            if(plotDataIt == plotData.numeric.end())
            {
                throw std::runtime_error("invalid channel");
            }

            usedChannels.push_back(&plotDataIt->second);
        }
    }
    //qDebug() << "final equation string : " << mathEquationReplaced;
    QJSValue globalVarResult = jsEngine.evaluate(_globalVars);
    if(globalVarResult.isError())
    {
        throw std::runtime_error("JS Engine : " + globalVarResult.toString().toStdString());
    }

    QString calcMethodStr = QString("function calc(x, y, CHANNEL_VALUES){with (Math){\n%1\n}}").arg(mathEquationReplaced);
    jsEngine.evaluate(calcMethodStr);
    QJSValue calcFct = jsEngine.evaluate("calc");
    if(calcFct.isError())
    {
        throw std::runtime_error("JS Engine : " + calcFct.toString().toStdString());
    }

    PlotData *srcPlotData;
    auto srcPlotDataIt = plotData.numeric.find(_linkedPlot);
    if(srcPlotDataIt == plotData.numeric.end())
    {
        throw std::runtime_error("invalid linked data channel");
    }
    srcPlotData = &srcPlotDataIt->second;

    out.clear();
    for(size_t i=0;i<srcPlotData->size();++i)
    {
        const PlotData::Point &oldPoint = srcPlotData->at(i);

        QJSValue channelValues = jsEngine.newArray(static_cast<quint32>(usedChannels.size()));
        for(int channelIndex = 0; channelIndex<usedChannels.size(); ++channelIndex)
        {
            PlotData *channelData = usedChannels[channelIndex];
            double value;
            int index = channelData->getIndexFromX(oldPoint.x);
            if(index != -1)
                value = channelData->at(static_cast<size_t>(index)).y;
            else
                value = std::numeric_limits<double>::quiet_NaN();

            channelValues.setProperty(static_cast<quint32>(channelIndex), QJSValue(value));
        }

        PlotData::Point newPoint;
        newPoint.x = oldPoint.x;
        //jsEngine.globalObject().setProperty("CHANNEL_VALUES", channelValues); // this would be another method to share the array
        QJSValue jsData = calcFct.call({QJSValue(oldPoint.x), QJSValue(oldPoint.y), channelValues});
        if(jsData.isError())
        {
            throw std::runtime_error("JS Engine : " + jsData.toString().toStdString());
        }
        newPoint.y = jsData.toNumber();

        out.pushBack(newPoint);
    }
}

const std::string &MathPlot::getName()
{
    return _plotName;
}

const std::string &MathPlot::getLinkedPlot()
{
    return _linkedPlot;
}

const QString &MathPlot::getGlobalVars()
{
    return _globalVars;
}

const QString &MathPlot::getEquation()
{
    return _calcEquation;
}

void MathPlot::addJavascriptDependencies(QJSEngine &engine)
{
    static QStringList files{":/js/resources/common.js", ":/js/resources/geographiclib.min.js" };
    for(QString fileName : files)
    {
        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly))
        {
            QString commonData = QString::fromUtf8(file.readAll());
            QJSValue out = engine.evaluate(commonData);
            if(out.isError())
            {
                qWarning() << "JS Engine : " << out.toString();
            }
        }
    }
}


