#include "add_math_plot.h"
#include "ui_add_math_plot.h"
#include <QDebug>
#include <QMessageBox>
#include <QFont>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include "math_plot.h"
#include "plotwidget.h"

AddMathPlotDialog::AddMathPlotDialog(PlotDataMapRef &plotMapData, QWidget *parent) :
    QDialog(parent),
    _plotMapData(plotMapData),
    ui(new Ui::AddMathPlotDialog)
{
    ui->setupUi(this);

    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->globalVarsTextField->setFont(fixedFont);
    ui->mathEquation->setFont(fixedFont);
    ui->snippetTextEdit->setFont(fixedFont);

    QStringList numericPlotNames;
    for(const auto &p : plotMapData.numeric)
    {
        QString name = QString::fromStdString(p.first);
        numericPlotNames.push_back(name);
    }
    numericPlotNames.sort(Qt::CaseInsensitive);
    for(const QString& name : numericPlotNames)
    {
        ui->linkedChannelCombobox->addItem(name);
        ui->curvesListWidget->addItem(name);
    }

    const auto& snippets = getSnippets();
    for(const SnippetData &d : snippets)
    {
        ui->snippetsListWidget->addItem(d.name);
    }
}

AddMathPlotDialog::~AddMathPlotDialog()
{
    delete ui;
}

void AddMathPlotDialog::setLinkedPlotName(const QString &linkedPlotName)
{
    ui->linkedChannelCombobox->setCurrentText(linkedPlotName);
}

void AddMathPlotDialog::accept()
{
    try {
        std::string plotName = getName().toStdString();
        if(_isNewPlot)
        {
            // check if name is unique
            if(_plotMapData.numeric.count(plotName) != 0)
            {
                throw std::runtime_error("plot name already exists");
            }
        }
        MathPlotPtr mathPlot = std::make_shared<MathPlot>(getLinkedData().toStdString(), plotName, getGlobalVars(), getEquation());
        PlotDataPtr newData = std::make_shared<PlotData>(plotName);
        mathPlot->calc(_plotMapData, *newData);

        _mathPlot = mathPlot;
        _newPlotData = newData;
        QDialog::accept();
    }
    catch (std::runtime_error e) {
        _mathPlot.reset();
        _newPlotData.reset();

        QMessageBox::critical(this, "Error", "Failed to create math plot : " + QString::fromStdString(e.what()));
    }
}

QString AddMathPlotDialog::getLinkedData() const
{
    return ui->linkedChannelCombobox->currentText();
}

QString AddMathPlotDialog::getGlobalVars() const
{
    return ui->globalVarsTextField->toPlainText();
}

QString AddMathPlotDialog::getEquation() const
{
    return ui->mathEquation->toPlainText();
}

QString AddMathPlotDialog::getName() const
{
    return ui->nameLineEdit->text();
}

PlotDataPtr AddMathPlotDialog::getNewPlotData() const
{
    return _newPlotData;
}

void AddMathPlotDialog::editExistingPlot(MathPlotPtr data)
{
    ui->globalVarsTextField->setPlainText(data->getGlobalVars());
    ui->mathEquation->setPlainText(data->getEquation());
    setLinkedPlotName(QString::fromStdString(data->getLinkedPlot()));
    ui->pushButtonDone->setText("Update");
    ui->nameLineEdit->setText(QString::fromStdString(data->getName()));
    ui->nameLineEdit->setEnabled(false);

    _isNewPlot = false;
}

MathPlotPtr AddMathPlotDialog::getMathPlotData() const
{
    return _mathPlot;
}



void AddMathPlotDialog::on_curvesListWidget_doubleClicked(const QModelIndex &index)
{
    QString appendString = QString("$$%1$$").arg(ui->curvesListWidget->item(index.row())->text());
    if(ui->globalVarsTextField->hasFocus())
    {
        ui->globalVarsTextField->insertPlainText(appendString);
    }
    else if(ui->mathEquation->hasFocus())
    {
        ui->mathEquation->insertPlainText(appendString);
    }
}

const std::vector<AddMathPlotDialog::SnippetData> AddMathPlotDialog::getSnippets()
{
    static bool firstRun = true;
    static std::vector<SnippetData> snippets;

    if(firstRun)
    {
        QFile file(":/resources/snippets.xml");
        if(!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical(nullptr, "Error", QString("Failed to open snippets.xml file"));
        }
        QDomDocument doc;
        QString parseErrorMsg;
        int parseErrorLine;
        if(!doc.setContent(&file, false, &parseErrorMsg, &parseErrorLine))
        {
            QMessageBox::critical(nullptr, "Error", QString("Failed to parse snippets.xml, error %1 at line %2").arg(parseErrorMsg).arg(parseErrorLine));
        }
        else
        {
            QDomElement docElem = doc.documentElement();
            QDomNode n = docElem.firstChild();
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull() && e.tagName() == "snippet") {
                    SnippetData d;
                    d.name = e.attribute("name");
                    d.globalVars = e.firstChildElement("global").text().trimmed();
                    d.equation = e.firstChildElement("equation").text().trimmed();
                    snippets.emplace_back(d);
                }
                n = n.nextSibling();
            }
        }
        firstRun = false;
    }

    return snippets;
}

void AddMathPlotDialog::on_snippetsListWidget_currentRowChanged(int currentRow)
{
    SnippetData snippet = getSnippets().at(static_cast<size_t>(currentRow));
    QString desc = QString("%1\n\nfunction calc(x,y){\n%2\n}").arg(snippet.globalVars).arg(snippet.equation);
    ui->snippetTextEdit->setPlainText(desc);
}

void AddMathPlotDialog::on_snippetsListWidget_doubleClicked(const QModelIndex &index)
{
    SnippetData snippet = getSnippets().at(static_cast<size_t>(index.row()));
    ui->globalVarsTextField->setPlainText(snippet.globalVars);
    ui->mathEquation->setPlainText(snippet.equation);
}

