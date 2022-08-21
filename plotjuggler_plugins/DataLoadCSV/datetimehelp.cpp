#include "datetimehelp.h"
#include "ui_datetimehelp.h"

#include <QScrollBar>

//Source: https://stackoverflow.com/a/42458736
void verticalResizeTableViewToContents(QTableView *tableView)
{
    int count=tableView->verticalHeader()->count();
    int scrollBarHeight = 0;
    if(tableView->horizontalScrollBar()->isVisible())
    {
        scrollBarHeight=tableView->horizontalScrollBar()->height();
    }
    int horizontalHeaderHeight=tableView->horizontalHeader()->height();
    int rowTotalHeight=0;
    for (int i = 0; i < count; ++i) {
        // 2018-03 edit: only account for row if it is visible
        if (!tableView->verticalHeader()->isSectionHidden(i)) {
            rowTotalHeight+=tableView->verticalHeader()->sectionSize(i);
        }
    }
    tableView->setMinimumHeight(horizontalHeaderHeight+rowTotalHeight+scrollBarHeight);
}

DateTimeHelp::DateTimeHelp(QDialog *parent) :
    QDialog(parent), ui(new Ui::DateTimeHelp), _parent(parent)
{
    ui->setupUi(this);

    verticalResizeTableViewToContents(ui->dateFormatTable);
    verticalResizeTableViewToContents(ui->timeFormatTable);

    refreshExample();

    connect(ui->exampleDateTimeDateTimeEdit,&QDateTimeEdit::dateTimeChanged,this, [this](){
        refreshExample();
    });

    connect(ui->exampleFormatStringLineEdit,&QLineEdit::textChanged,this, [this](){
        refreshExample();
    });

    connect(_parent,&QDialog::finished,this,[this](){
        if(ui->autoCloseCheckBox->isChecked())
        {
            accept();
        }

    });

}

DateTimeHelp::~DateTimeHelp()
{
    delete ui;
}

void DateTimeHelp::refreshExample()
{
    auto dateTime = ui->exampleDateTimeDateTimeEdit->dateTime();
    auto formatString = ui->exampleFormatStringLineEdit->text();
    ui->resultLineEdit->setText(dateTime.toString(formatString));
}
