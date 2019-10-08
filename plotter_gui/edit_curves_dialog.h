#ifndef EDIT_CURVES_DIALOG_H
#define EDIT_CURVES_DIALOG_H


#include "ui_edit_curves_dialog.h"
#include "plotwidget.h"
#include "series_data.h"
#include "PlotJuggler/random_color.h"

#include <deque>

#include <QLineEdit>
#include <QDialog>
#include <QCompleter>
#include <QShortcut>
#include <QSignalMapper>
#include <QDomElement>
#include <QElapsedTimer>
#include <QDebug>

namespace Ui {
class EditCurvesDialog;
}

class MainWindow;
class PlotWidget;

class EditCurvesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditCurvesDialog(const PlotDataMapRef& plot_map_data,
                              bool isXYCurve,
                              PlotWidget *parent = nullptr);
    ~EditCurvesDialog();

    void addCurveName(const QString &name, const QColor &color);

    void onUndoableChange();

private slots:
    void on_pushButtonClose_pressed();
    void on_pushButtonApply_pressed();
    void on_pushButtonAddNew_pressed();
    void on_pushButtonRemoveSelected_pressed();
    void on_pushButtonToggleCurveType_pressed();
    void on_listCurveWidget_itemSelectionChanged();

    void onUndoInvoked();
    void onRedoInvoked();

private:
    void setCurveType();
    QDomDocument xmlSaveState() const;
    bool xmlLoadState(QDomDocument state_document);

    QShortcut _undo_shortcut;
    QShortcut _redo_shortcut;
    std::deque<QDomDocument> _undo_states;
    std::deque<QDomDocument> _redo_states;
    QElapsedTimer _undo_timer;
    bool _disable_undo_logging;

    Ui::EditCurvesDialog *ui;
    QCompleter *_completerX;
    QCompleter *_completerY;
    QStringList _numericPlotNames;
    bool _isXYCurve;
    PlotWidget *_parent;
};

#endif // EDIT_CURVES_DIALOG_H
