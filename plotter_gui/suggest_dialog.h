#ifndef SUGGEST_DIALOG_H
#define SUGGEST_DIALOG_H

#include <QDialog>

namespace Ui {
class SuggestDialog;
}

class SuggestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SuggestDialog(const std::string &name_x,
                           const std::string &name_y,
                           double &start_x,
                           double &end_x,
                           double &start_y,
                           double &end_y,
                           QWidget *parent = nullptr);
    ~SuggestDialog();

    QString nameX() const;
    QString nameY() const;
    double offsetX() const;
    double offsetY() const;
    QString suggestedName() const;

    void updateSuggestion();

private slots:
    void on_pushButtonSwap_pressed();
    void on_pushButtonRemoveYStart_pressed();

private:
    Ui::SuggestDialog *ui;
};

#endif // SUGGEST_DIALOG_H
