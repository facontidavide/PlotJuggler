#ifndef AddMathPlotDialog_H
#define AddMathPlotDialog_H

#include <QDialog>
#include <QListWidgetItem>
#include <qwt_plot_curve.h>
#include "PlotJuggler/plotdata.h"
#include "math_plot.h"

namespace Ui {
class AddMathPlotDialog;
}

class AddMathPlotDialog : public QDialog
{
    Q_OBJECT

    struct SnippetData{
        QString name;
        QString globalVars;
        QString equation;
    };

public:
    explicit AddMathPlotDialog(PlotDataMapRef &plotMapData, QWidget *parent);
    virtual ~AddMathPlotDialog() override;

    void setLinkedPlotName(const QString &linkedPlotName);
    void accept() override;

    QString getLinkedData() const;
    QString getGlobalVars() const;
    QString getEquation() const;
    QString getName() const;
    PlotDataPtr getNewPlotData() const;
    void editExistingPlot(MathPlotPtr data);
    MathPlotPtr getMathPlotData() const;

private slots:

    void on_curvesListWidget_doubleClicked(const QModelIndex &index);

    void on_snippetsListWidget_currentRowChanged(int currentRow);

    void on_snippetsListWidget_doubleClicked(const QModelIndex &index);

private:
    static const std::vector<SnippetData> getSnippets();

    PlotDataMapRef &_plotMapData;
    Ui::AddMathPlotDialog *ui;

    PlotDataPtr _newPlotData;
    MathPlotPtr _mathPlot;
    bool _isNewPlot = true;
};

#endif // AddMathPlotDialog_H
