#ifndef DATETIMEHELP_H
#define DATETIMEHELP_H

#include <QDialog>

namespace Ui {
class DateTimeHelp;
}

class DateTimeHelp : public QDialog
{
    Q_OBJECT

public:
    explicit DateTimeHelp(QDialog *parent = nullptr);
    ~DateTimeHelp();

    void refreshExample();

private:
    Ui::DateTimeHelp *ui;

    QDialog* _parent;
};

#endif // DATETIMEHELP_H
