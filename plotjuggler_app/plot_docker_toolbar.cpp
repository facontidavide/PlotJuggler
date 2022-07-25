/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "plot_docker_toolbar.h"
#include "PlotJuggler/svg_util.h"

DockToolbar::DockToolbar(ads::CDockWidget* parent)
  : QWidget(parent)
  , _parent(parent)
  , ui(new Ui::DraggableToolbar)
  , _fullscreen_mode(false)
{
  ui->setupUi(this);

  QSettings settings;
  QString theme = settings.value("StyleSheet::theme", "light").toString();
  on_stylesheetChanged(theme);

  ui->buttonFullscreen->setVisible(false);
  ui->buttonSplitHorizontal->setVisible(false);
  ui->buttonSplitVertical->setVisible(false);
  ui->buttonBackground->setVisible(false);

  setMouseTracking(true);
  ui->widgetButtons->setMouseTracking(true);

  ui->label->installEventFilter(this);

  setAcceptDrops(true);
}

DockToolbar::~DockToolbar()
{
  delete ui;
}

static void setButtonIcon(QPushButton* button, QIcon icon)
{
  button->setIcon(icon);
  button->setText("");
}

void DockToolbar::toggleFullscreen()
{
  _fullscreen_mode = !_fullscreen_mode;

  setButtonIcon(ui->buttonFullscreen, _fullscreen_mode ? _collapse_icon : _expand_icon);

  ui->buttonClose->setHidden(_fullscreen_mode);
  if (_fullscreen_mode)
  {
    ui->buttonSplitHorizontal->setVisible(false);
    ui->buttonSplitVertical->setVisible(false);
  }
}

void DockToolbar::mousePressEvent(QMouseEvent* ev)
{
  _parent->dockAreaWidget()->titleBar()->mousePressEvent(ev);
}

void DockToolbar::mouseReleaseEvent(QMouseEvent* ev)
{
  _parent->dockAreaWidget()->titleBar()->mouseReleaseEvent(ev);
}

void DockToolbar::mouseMoveEvent(QMouseEvent* ev)
{
  ui->buttonFullscreen->setVisible(true);
  ui->buttonBackground->setVisible(true);
  ui->buttonSplitHorizontal->setVisible(!_fullscreen_mode);
  ui->buttonSplitVertical->setVisible(!_fullscreen_mode);
  _parent->dockAreaWidget()->titleBar()->mouseMoveEvent(ev);

  ev->accept();
  QWidget::mouseMoveEvent(ev);
}

void DockToolbar::enterEvent(QEvent* ev)
{
  ui->buttonFullscreen->setVisible(true);
  ui->buttonBackground->setVisible(true);
  ui->buttonSplitHorizontal->setVisible(!_fullscreen_mode);
  ui->buttonSplitVertical->setVisible(!_fullscreen_mode);

  ev->accept();
  QWidget::enterEvent(ev);
}

bool DockToolbar::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::MouseButtonDblClick)
  {
    bool ok = true;
    QString newName =
        QInputDialog::getText(this, tr("Change name of the Area"), tr("New name:"),
                              QLineEdit::Normal, ui->label->text(), &ok);
    if (ok)
    {
      ui->label->setText(newName);
      emit titleChanged(newName);
    }
    return true;
  }
  else
  {
    return QObject::eventFilter(object, event);
  }
}

void DockToolbar::on_stylesheetChanged(QString theme)
{
  _expand_icon = LoadSvg(":/resources/svg/expand.svg", theme);
  _collapse_icon = LoadSvg(":/resources/svg/collapse.svg", theme);
  setButtonIcon(ui->buttonFullscreen, _fullscreen_mode ? _collapse_icon : _expand_icon);
  setButtonIcon(ui->buttonClose, LoadSvg(":/resources/svg/close-button.svg", theme));
  setButtonIcon(ui->buttonSplitHorizontal,
                LoadSvg(":/resources/svg/add_column.svg", theme));
  setButtonIcon(ui->buttonSplitVertical, LoadSvg(":/resources/svg/add_row.svg", theme));
}

void DockToolbar::dragEnterEvent(QDragEnterEvent* event)
{
  const QMimeData* mimeData = event->mimeData();
  QStringList mimeFormats = mimeData->formats();

  bool has_curve = false;
  for (const QString& format : mimeFormats)
  {
    QByteArray encoded = mimeData->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    if (format == "curveslist/add_curve")
    {
      while (!stream.atEnd())
      {
        QString curve_name;
        stream >> curve_name;
        if (!curve_name.isEmpty())
        {
          if (!has_curve)
          {
            has_curve = true;
            _dragging_curve = curve_name;
          }
          else
          {
            // multiple curves, discard
            _dragging_curve.clear();
            return;
          }
        }
      }
    }
  }
  if (has_curve)
  {
    event->accept();
    ui->buttonFullscreen->setVisible(true);
    ui->buttonBackground->setVisible(true);
    ui->buttonSplitHorizontal->setVisible(!_fullscreen_mode);
    ui->buttonSplitVertical->setVisible(!_fullscreen_mode);
  }
}

void DockToolbar::dragLeaveEvent(QDragLeaveEvent*)
{
  ui->buttonFullscreen->setVisible(false);
  ui->buttonBackground->setVisible(ui->buttonBackground->isChecked());
  ui->buttonSplitHorizontal->setVisible(false);
  ui->buttonSplitVertical->setVisible(false);
}

void DockToolbar::dropEvent(QDropEvent* event)
{
  auto global_pos = mapToGlobal(event->pos());
  auto pos = ui->buttonBackground->mapFromGlobal(global_pos);
  bool on_label = ui->buttonBackground->rect().contains(pos);
  if (on_label)
  {
    emit backgroundColorRequest(_dragging_curve);
    _dragging_curve.clear();
  }
}

void DockToolbar::leaveEvent(QEvent* ev)
{
  ui->buttonFullscreen->setVisible(_fullscreen_mode);
  ui->buttonSplitHorizontal->setVisible(false);
  ui->buttonSplitVertical->setVisible(false);
  ui->buttonBackground->setVisible(ui->buttonBackground->isChecked());
  QWidget::leaveEvent(ev);
}

void DockToolbar::on_buttonBackground_clicked()
{
  emit backgroundColorRequest({});
}
