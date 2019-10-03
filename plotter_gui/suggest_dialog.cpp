#include <QSettings>
#include "suggest_dialog.h"
#include "ui_suggest_dialog.h"

SuggestDialog::SuggestDialog(const std::string& name_x,
                             const std::string& name_y,
                             double& start_x,
                             double& end_x,
                             double& start_y,
                             double& end_y,
                             QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SuggestDialog)
{
    ui->setupUi(this);

    QSettings settings;
    restoreGeometry(settings.value("SuggestDialog.geometry").toByteArray());

    ui->lineEditX->setText( QString::fromStdString(name_x) );
    ui->lineEditY->setText( QString::fromStdString(name_y) );

    ui->lineEditXStart->setText( QString::fromStdString(std::to_string(start_x)) );
    ui->lineEditYStart->setText( QString::fromStdString(std::to_string(start_y)) );
    ui->lineEditXEnds->setText( QString::fromStdString(std::to_string(end_x)) );
    ui->lineEditYEnds->setText( QString::fromStdString(std::to_string(end_y)) );

    ui->lineEditOffsetX->setText( QString::fromStdString("0") );
    ui->lineEditOffsetY->setText( QString::fromStdString("0") );
    
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

double SuggestDialog::offsetX() const
{
    return ui->lineEditOffsetX->text().toDouble();
}

double SuggestDialog::offsetY() const
{
    return ui->lineEditOffsetY->text().toDouble();
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


    if( name_x.size() > name_y.size() )
    {
        std::swap( name_x, name_y );
    }
    common_prefix = std::string( name_x.begin(), std::mismatch( name_x.begin(), name_x.end(), name_y.begin() ).first ) ;

    std::string suffix_x = name_x.substr( common_prefix.size() );
    std::string suffix_y = name_y.substr( common_prefix.size() );

    std::string suggestion = common_prefix + "[" + suffix_x + ";" + suffix_y + "]";
    ui->lineEditName->setText( QString::fromStdString(suggestion) );
}

void SuggestDialog::on_pushButtonSwap_pressed()
{
    auto temp = ui->lineEditX->text();
    ui->lineEditX->setText( ui->lineEditY->text() );
    ui->lineEditY->setText( temp );
    temp = ui->lineEditOffsetX->text();
    ui->lineEditOffsetX->setText( ui->lineEditOffsetY->text() );
    ui->lineEditOffsetY->setText( temp );
    auto start_x = ui->lineEditXStart->text();
    auto ends_x = ui->lineEditXEnds->text();
    auto start_y = ui->lineEditYStart->text();
    auto ends_y = ui->lineEditYEnds->text();
    ui->lineEditXStart->setText( start_y );
    ui->lineEditXEnds->setText( ends_y);
    ui->lineEditYStart->setText( start_x );
    ui->lineEditYEnds->setText( ends_x );

    updateSuggestion();
}
