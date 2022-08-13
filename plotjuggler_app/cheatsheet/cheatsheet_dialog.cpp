#include "cheatsheet_dialog.h"
#include "ui_cheatsheet_dialog.h"

CheatsheetDialog::CheatsheetDialog(QWidget* parent)
  : QDialog(parent), ui(new Ui::CheatsheetDialog)
{
  ui->setupUi(this);

  QPixmap pixmap;

  pixmap.load(":/cheatsheet/img/tutorial_drag_drop.png");
  ui->labelImage_1->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_search.png");
  ui->labelImage_2->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_split.png");
  ui->labelImage_3->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_xy_drag.png");
  ui->labelImage_4->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_tracker.png");
  ui->labelImage_5->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_zoom.png");
  ui->labelImage_6->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_font_size.png");
  ui->labelImage_7->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_filter.png");
  ui->labelImage_8->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_filter.png");
  ui->labelImage_8->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_custom.png");
  ui->labelImage_9->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_layout.png");
  ui->labelImage_10->setPixmap(pixmap);

  pixmap.load(":/cheatsheet/img/tutorial_colormap.png");
  ui->labelImage_11->setPixmap(pixmap);
}

CheatsheetDialog::~CheatsheetDialog()
{
  delete ui;
}

void CheatsheetDialog::on_listWidget_currentRowChanged(int currentRow)
{
  ui->stackedWidget->setCurrentIndex(currentRow);
}
