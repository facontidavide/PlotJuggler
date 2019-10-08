#include "edit_curves_dialog.h"

EditCurvesDialog::EditCurvesDialog(const PlotDataMapRef& plot_map_data,
                                   bool isXYCurve,
                                   PlotWidget *parent) :
    QDialog(parent),
    _isXYCurve(isXYCurve),
    _parent(parent),
    _disable_undo_logging(false),
    _undo_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Z), this),
    _redo_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this),
    ui(new Ui::EditCurvesDialog)
{
    ui->setupUi(this);

    setCurveType();

    
    for(const auto &p : plot_map_data.numeric)
    {
        QString name = QString::fromStdString(p.first);
        _numericPlotNames.push_back(name);
    }
    _numericPlotNames.sort(Qt::CaseInsensitive);
    ui->comboButtonBrowseX->addItem("");
    ui->comboButtonBrowseY->addItem("");
    ui->comboButtonBrowseX->addItems(_numericPlotNames);
    ui->comboButtonBrowseX->setEditable(true);
    ui->comboButtonBrowseY->addItems(_numericPlotNames);
    ui->comboButtonBrowseY->setEditable(true);

    _completerX = new QCompleter(_numericPlotNames, this);
    _completerX->setCaseSensitivity(Qt::CaseInsensitive);
    _completerX->setFilterMode(Qt::MatchContains);
    _completerX->setCompletionMode(QCompleter::PopupCompletion);

    _completerY = new QCompleter(_numericPlotNames, this);
    _completerY->setCaseSensitivity(Qt::CaseInsensitive);
    _completerY->setFilterMode(Qt::MatchContains);
    _completerY->setCompletionMode(QCompleter::PopupCompletion);

    ui->comboButtonBrowseX->setCompleter(_completerX);
    ui->comboButtonBrowseY->setCompleter(_completerY);

    connect( &_undo_shortcut, &QShortcut::activated, this, &EditCurvesDialog::onUndoInvoked);
    connect( &_redo_shortcut, &QShortcut::activated, this, &EditCurvesDialog::onRedoInvoked);

    _undo_timer.start();

    // save initial state
    // Edit: this must happen after curve names are initially added
    // in plotwidget 
    //onUndoableChange();
}

EditCurvesDialog::~EditCurvesDialog()
{
    delete ui;
}

void EditCurvesDialog::addCurveName(const QString &curve_name, 
                                    const QColor &color )
{
    QListWidgetItem* item = new QListWidgetItem(curve_name);
    item->setForeground(color);
    ui->listCurveWidget->addItem(item);
}

void EditCurvesDialog::on_listCurveWidget_itemSelectionChanged()
{
    // Using currentItem() is not strictly correct. Should find the
    // right way of doing this.

    auto current_item = ui->listCurveWidget->currentItem();
    if (current_item == NULL)
    {
        return;
    }

    if (_isXYCurve)
    {
        std::string orig = current_item->text().toStdString();
        std::string str = orig;
        std::string delimiter = "[";
        std::string xtopic, ytopic;
        size_t pos = 0;
        while ((pos = str.find(delimiter)) != std::string::npos) 
        {
            xtopic = ytopic = str.substr(0, pos);
            str.erase(0, pos + delimiter.length());
        }
        delimiter = ";";
        while ((pos = str.find(delimiter)) != std::string::npos) 
        {
            xtopic += str.substr(0, pos);
            str.erase(0, pos + delimiter.length());
            ytopic += str.substr(0, str.length()-1);
        }
        ui->comboButtonBrowseX->setCurrentText(QString::fromStdString(xtopic));
        ui->comboButtonBrowseY->setCurrentText(QString::fromStdString(ytopic));
    }
    else
    {
        ui->comboButtonBrowseY->setCurrentText(current_item->text());
    }
}

void EditCurvesDialog::on_pushButtonClose_pressed()
{
    this->accept();
}

void EditCurvesDialog::on_pushButtonApply_pressed()
{
    auto current_item = ui->listCurveWidget->currentItem();
    if (current_item == NULL)
    {
        return;
    }

    if (_isXYCurve)
    {
        auto xtopic = ui->comboButtonBrowseX->currentText().toStdString();
        auto ytopic = ui->comboButtonBrowseY->currentText().toStdString();

        size_t pos = 0;
        while (xtopic[pos] == ytopic[pos])
        {
            pos++;
        }
        auto base = xtopic.substr(0, pos);
        auto x_name = xtopic;
        auto y_name = ytopic;
        x_name = x_name.erase(0, pos);
        y_name = y_name.erase(0, pos);
        auto new_name = QString::fromStdString(base + "[" + x_name + ";" + y_name + "]");

        if (!ui->listCurveWidget->findItems(new_name, 0).empty()
            || !_numericPlotNames.contains(QString::fromStdString(xtopic))
            || !_numericPlotNames.contains(QString::fromStdString(ytopic))
            || xtopic == ytopic
            || xtopic == ""
            || ytopic == "")
        {
            QMessageBox::warning(this, "Invalid Curve",
                                 "The proposed curve is either invalid or already exists.",
                                 QMessageBox::Ok);
            return;
        }

        _parent->removeCurve(current_item->text().toStdString());
        _parent->addCurveXY(xtopic, ytopic, new_name, current_item->foreground().color());

        current_item->setText(new_name);
    }
    else
    {
        auto ytopic = ui->comboButtonBrowseY->currentText();

        if (!ui->listCurveWidget->findItems(ytopic, 0).empty()
            || !_numericPlotNames.contains(ytopic)
            || ytopic == "")
        {
            QMessageBox::warning(this, "Invalid Curve",
                                 "The proposed curve is either invalid or already exists.",
                                 QMessageBox::Ok);
            return;
        }

        _parent->removeCurve(current_item->text().toStdString());
        _parent->addCurve(ytopic.toStdString(), current_item->foreground().color());

        current_item->setText(ytopic);
    }
    _parent->replot();
    onUndoableChange();
}

void EditCurvesDialog::on_pushButtonAddNew_pressed()
{
    if (_isXYCurve)
    {
        auto xtopic = ui->comboButtonBrowseX->currentText().toStdString();
        auto ytopic = ui->comboButtonBrowseY->currentText().toStdString();

        size_t pos = 0;
        while (xtopic[pos] == ytopic[pos])
        {
            pos++;
        }
        auto base = xtopic.substr(0, pos);
        auto x_name = xtopic;
        auto y_name = ytopic;
        x_name = x_name.erase(0, pos);
        y_name = y_name.erase(0, pos);
        auto new_name = QString::fromStdString(base + "[" + x_name + ";" + y_name + "]");

        if (!ui->listCurveWidget->findItems(new_name, 0).empty()
            || !_numericPlotNames.contains(QString::fromStdString(xtopic))
            || !_numericPlotNames.contains(QString::fromStdString(ytopic))
            || xtopic == ytopic
            || xtopic == ""
            || ytopic == "")
        {
            QMessageBox::warning(this, "Invalid Curve",
                                 "The proposed curve is either invalid or already exists.",
                                 QMessageBox::Ok);
            return;
        }

        auto color = randomColorHint();
        _parent->addCurveXY(xtopic, ytopic, new_name, color);
        addCurveName(new_name, color);
    }
    else
    {
        auto ytopic = ui->comboButtonBrowseY->currentText();

        if (!ui->listCurveWidget->findItems(ytopic, 0).empty()
            || !_numericPlotNames.contains(ytopic)
            || ytopic == "")
        {
            QMessageBox::warning(this, "Invalid Curve",
                                 "The proposed curve is either invalid or already exists.",
                                 QMessageBox::Ok);
            return;
        }

        auto color = randomColorHint();
        _parent->addCurve(ytopic.toStdString(), color);
        addCurveName(ytopic, color);
    }
    _parent->replot();
    onUndoableChange();
}

void EditCurvesDialog::on_pushButtonRemoveSelected_pressed()
{
    if (ui->listCurveWidget->count() == 0)
    {
        return;
    }

    auto current_item = ui->listCurveWidget->takeItem(ui->listCurveWidget->currentRow());
    _parent->removeCurve(current_item->text().toStdString());
    delete current_item;
    _parent->replot();
    onUndoableChange();
}

void EditCurvesDialog::on_pushButtonToggleCurveType_pressed()
{
    int count = ui->listCurveWidget->count();
    if (count != 0)
    {
        auto ret = QMessageBox::warning(this, "Warning",
          "Toggling plot types will remove all existing curves. Proceed?",
          QMessageBox::Ok | QMessageBox::Cancel);
        if (ret != QMessageBox::Ok)
        {
            return;
        }
        for (int i=0; i < count; i++)
        {
            auto item = ui->listCurveWidget->takeItem(0);
            _parent->removeCurve(item->text().toStdString());
            delete item;
        }
    }

    _isXYCurve = !_isXYCurve;
    setCurveType();
    onUndoableChange();
}

void EditCurvesDialog::setCurveType()
{
    int count = ui->listCurveWidget->count();
    if (_isXYCurve)
    {
        if (!_parent->isXYPlot())
        {
            _parent->convertToXY();
        }
        ui->labelY->setText("Y:");
        ui->labelX->setHidden(false);
        ui->comboButtonBrowseX->setEnabled(true);
        ui->comboButtonBrowseX->setHidden(false);
    }
    else if (!_isXYCurve)
    {
        if (_parent->isXYPlot())
        {
            _parent->convertToTimeseries();
        }
        ui->labelY->setText("Timeseries:");
        ui->labelX->setHidden(true);
        ui->comboButtonBrowseX->setEnabled(false);
        ui->comboButtonBrowseX->setHidden(true);
    }

    ui->comboButtonBrowseX->setEnabled(_isXYCurve);
    ui->comboButtonBrowseX->setCurrentText("");
    ui->comboButtonBrowseY->setCurrentText("");
}

QDomDocument EditCurvesDialog::xmlSaveState() const
{
    QDomDocument doc;
    QDomProcessingInstruction instr =
            doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");

    doc.appendChild(instr);

    QDomElement root = doc.createElement( "root" );

    doc.appendChild(root);

    QDomElement isXYCurve_el = doc.createElement( "plot_type" );
    isXYCurve_el.setAttribute("is_xy", _isXYCurve);
    root.appendChild(isXYCurve_el);

    int count = ui->listCurveWidget->count();
    if (count != 0)
    {
        for (int i=0; i < count; i++)
        {
            auto item = ui->listCurveWidget->item(i);
            auto color = item->foreground().color();

            QDomElement curve_el = doc.createElement("curve");
            curve_el.setAttribute( "name", item->text());
            curve_el.setAttribute( "R", color.red());
            curve_el.setAttribute( "G", color.green());
            curve_el.setAttribute( "B", color.blue());

            root.appendChild(curve_el);
        }
    }
    for (QDomElement curve_el = root.firstChildElement("curve");
         !curve_el.isNull();
         curve_el = curve_el.nextSiblingElement("curve") )
    {
        QString curve_name = curve_el.attribute("name");
    }

    return doc;
}

bool EditCurvesDialog::xmlLoadState(QDomDocument state)
{      
    QDomElement root = state.namedItem("root").toElement();
    if (root.isNull()) 
    {
        return false;
    }

    QDomElement isXYCurve_el = root.firstChildElement("plot_type");
    if (isXYCurve_el.isNull())
    {
        return false;
    }
    _isXYCurve = isXYCurve_el.attribute("is_xy")==QString("1");

    int count = ui->listCurveWidget->count();
    for (int i=0; i < count; i++)
    {
       auto item = ui->listCurveWidget->takeItem(0);
       delete item;
    }

    setCurveType();

    for (QDomElement curve_el = root.firstChildElement("curve");
         !curve_el.isNull();
         curve_el = curve_el.nextSiblingElement("curve") )
    {
        QString curve_name = curve_el.attribute("name");
        int R = curve_el.attribute("R").toInt();
        int G = curve_el.attribute("G").toInt();
        int B = curve_el.attribute("B").toInt();
        QColor color(R,G,B);

        addCurveName(curve_name, color);
    }

    return true;
}

void EditCurvesDialog::onUndoableChange()
{
    if (_disable_undo_logging) return;

    int elapsed_ms = _undo_timer.restart();

    // overwrite the previous
    if (elapsed_ms < 100)
    {
        if (_undo_states.empty() == false)
            _undo_states.pop_back();
    }

    while (_undo_states.size() >= 100) _undo_states.pop_front();
    _undo_states.push_back(xmlSaveState());
    _redo_states.clear();
    emit _parent->undoableChange();
}

void EditCurvesDialog::onUndoInvoked()
{
    _disable_undo_logging = true;
    if( _undo_states.size() > 1)
    {
        QDomDocument state_document = _undo_states.back();
        while (_redo_states.size() >= 100) _redo_states.pop_front();
        _redo_states.push_back(state_document);
        _undo_states.pop_back();
        state_document = _undo_states.back();

        xmlLoadState(state_document);
        emit _parent->undoInvoked();
    }
    _disable_undo_logging = false;
}

void EditCurvesDialog::onRedoInvoked()
{
    _disable_undo_logging = true;
    if( _redo_states.size() > 0)
    {
        QDomDocument state_document = _redo_states.back();
        while (_undo_states.size() >= 100) _undo_states.pop_front();
        _undo_states.push_back(state_document);
        _redo_states.pop_back();

        xmlLoadState(state_document);
        emit _parent->redoInvoked();
    }
    _disable_undo_logging = false;
}
