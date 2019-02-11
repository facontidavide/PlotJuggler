#ifndef TABBEDPLOTWIDGET_H
#define TABBEDPLOTWIDGET_H

#include <QWidget>
#include <QMainWindow>
#include <QTableWidget>
#include "plotmatrix.h"

namespace Ui {
class TabbedPlotWidget;
}


class TabbedPlotWidget : public QWidget
{
    Q_OBJECT

public:
    typedef struct{} MainWindowArea;

    explicit TabbedPlotWidget(QString name,
                              QMainWindow *main_window,
                              PlotMatrix* first_tab,
                              PlotDataMapRef &mapped_data,
                              QMainWindow *parent );

    PlotMatrix* currentTab();

    QTabWidget* tabWidget();

    const QTabWidget* tabWidget() const;

    void addTab(PlotMatrix *tab = nullptr);

    QDomElement xmlSaveState(QDomDocument &doc) const;
    bool xmlLoadState(QDomElement &tabbed_area);

    ~TabbedPlotWidget();

    QString name() const { return _name; }

    static const std::map<QString,TabbedPlotWidget*>& instances();

    static TabbedPlotWidget *instance(const QString& key);

    void setControlsVisible(bool visible);

public slots:

    void setStreamingMode(bool streaming_mode);

    static void saveTabImage(QString fileName, PlotMatrix* matrix);

private slots:

    void on_renameCurrentTab();

    void on_savePlotsToFile();

    void on_pushAddColumn_pressed();

    void on_pushVerticalResize_pressed();

    void on_pushHorizontalResize_pressed();

    void on_pushAddRow_pressed();

    void on_addTabButton_pressed();

    void on_pushRemoveEmpty_pressed();

    void on_tabWidget_currentChanged(int index);

    void on_tabWidget_tabCloseRequested(int index);

    void on_buttonLinkHorizontalScale_toggled(bool checked);

    void on_requestTabMovement(const QString &destination_name);

    void on_moveTabIntoNewWindow();

    void on_pushButtonShowLabel_toggled(bool checked);

    void on_pushButtonZoomMax_pressed();

private:
    Ui::TabbedPlotWidget *ui;

    QAction* _action_renameTab;
    QAction* _action_savePlots;

    QMenu* _tab_menu;

    const QString _name;

    PlotDataMapRef& _mapped_data;

    bool _horizontal_link;

    QString _parent_type;

    virtual void closeEvent(QCloseEvent *event) override;

protected:

    virtual bool eventFilter(QObject *obj, QEvent *event);

    static std::map<QString,TabbedPlotWidget*> _instances;

signals:
    void created();
    void undoableChangeHappened();
    void matrixAdded( PlotMatrix * );
    void sendTabToNewWindow(PlotMatrix *);
};

#endif // TABBEDPLOTWIDGET_H
