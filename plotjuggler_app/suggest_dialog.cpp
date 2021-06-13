#include <QSettings>
#include "suggest_dialog.h"
#include "ui_suggest_dialog.h"

SuggestDialog::SuggestDialog(const std::string& name_x, const std::string& name_y, QWidget* parent)
  : QDialog(parent), ui(new Ui::SuggestDialog)
{
  ui->setupUi(this);

  QSettings settings;
  restoreGeometry(settings.value("SuggestDialog.geometry").toByteArray());

  ui->lineEditX->setText(QString::fromStdString(name_x));
  ui->lineEditY->setText(QString::fromStdString(name_y));
  updateSuggestion();
}

SuggestDialog::~SuggestDialog()
{
  QSettings settings;
  settings.setValue("SuggestDialog.geometry", saveGeometry());
  delete ui;
}

QString SuggestDialog::nameX() const
{
  return ui->lineEditX->text();
}

QString SuggestDialog::nameY() const
{
  return ui->lineEditY->text();
}

QString SuggestDialog::suggestedName() const
{
  return ui->lineEditName->text();
}

void SuggestDialog::updateSuggestion()
{
  std::string common_prefix;
  std::string name_x = ui->lineEditX->text().toStdString();
  std::string name_y = ui->lineEditY->text().toStdString();

  if (name_x.size() > name_y.size())
  {
    std::swap(name_x, name_y);
  }
  common_prefix = std::string(name_x.begin(), std::mismatch(name_x.begin(), name_x.end(), name_y.begin()).first);

  std::string suffix_x = name_x.substr(common_prefix.size());
  std::string suffix_y = name_y.substr(common_prefix.size());

  std::string suggestion = common_prefix + "[" + suffix_x + ";" + suffix_y + "]";
  ui->lineEditName->setText(QString::fromStdString(suggestion));
}

void SuggestDialog::on_pushButtonSwap_pressed()
{
  auto temp = ui->lineEditX->text();
  ui->lineEditX->setText(ui->lineEditY->text());
  ui->lineEditY->setText(temp);
  updateSuggestion();
}
