#ifndef EDIT_LABELS_DIALOG_H
#define EDIT_LABELS_DIALOG_H

#include <QDialog>

namespace Ui {
class EditLabelsDialog;
}

class EditLabelsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditLabelsDialog(QWidget *parent = 0);
    ~EditLabelsDialog();
    QString labelX();
    QString labelY();
    QString plotLabel();
    void setLabelX(const QString &label);
    void setLabelY(const QString &label);
    void setPlotLabel(const QString &label);

private slots:
    void on_pushButtonDone_pressed();

    void on_pushButtonCancel_pressed();

private:

    //virtual void closeEvent(QCloseEvent *event) override;

    Ui::EditLabelsDialog *ui;

    QString _plot_label;
    QString _x_label;
    QString _y_label;
};

#endif // EDIT_LABELS_DIALOG_H
