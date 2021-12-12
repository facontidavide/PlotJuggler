#include "video_dialog.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QDebug>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QSettings>
#include <cmath>
#include "PlotJuggler/svg_util.h"

bool VideoDialog::loadFile(QString filename)
{
  if(!filename.isEmpty() && QFileInfo::exists(filename))
  {
    _media_player->play(filename);
    _media_player->pause(true);
    ui->lineFilename->setText(filename);
    return true;
  }
  return false;
}

VideoDialog::VideoDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::VideoDialog)
{
  using namespace QtAV;

  ui->setupUi(this);
  _media_player = new AVPlayer(this);
  _video_output = new VideoOutput(this);

  if (!_video_output->widget())
  {
    QMessageBox::warning(this, QString::fromLatin1("QtAV error"),
                         tr("Can not create video renderer"));
    return;
  }
  _media_player->setRenderer(_video_output);
  ui->verticalLayoutMain->addWidget(_video_output->widget());

  connect(_media_player, &AVPlayer::started,
          this, &VideoDialog::updateSlider);

  ui->lineEditReference->installEventFilter(this);

  QSettings settings;
  QString theme = settings.value("Preferences::theme", "light").toString();
  ui->clearButton->setIcon(LoadSvg(":/resources/svg/trash.svg", theme));
}

VideoDialog::~VideoDialog()
{
  delete ui;
}

QString VideoDialog::referenceCurve() const
{
  return ui->lineEditReference->text();
}

void VideoDialog::pause(bool paused)
{
  _media_player->pause(paused);
}

bool VideoDialog::isPaused() const
{
  return _media_player->isPaused();
}

void VideoDialog::on_loadButton_clicked()
{
  QSettings settings;

  QString directory_path =
      settings.value("VideoDialog.loadDirectory", QDir::currentPath()).toString();

  QString filename = QFileDialog::getOpenFileName(this, tr("Load Video"),
                                                  directory_path, tr("(*.*)"));
  if (!loadFile(filename))
  {
    return;
  }
  directory_path = QFileInfo(filename).absolutePath();
  settings.setValue("VideoDialog.loadDirectory", directory_path);
}

void VideoDialog::updateSlider()
{
  const auto& fps = _media_player->statistics().video.frame_rate;
  qint64 duration_ms = _media_player->duration();
  int num_frames = static_cast<int>(qreal(duration_ms) * fps / 1000.0);
  ui->timeSlider->setRange(0, num_frames);
  _media_player->setNotifyInterval( static_cast<int>(1000 / fps) );
  _media_player->pause(true);
}

void VideoDialog::on_timeSlider_valueChanged(int frame)
{
  double fps = _media_player->statistics().video.frame_rate;
  double period = 1000 / fps;
  qint64 frame_pos = static_cast<qint64>(qreal(frame) * period );
//  qint64 current_pos = m_player->position();
//  qint64 frames_diff = frame - _prev_frame;

  _media_player->setSeekType(QtAV::SeekType::AccurateSeek);
  _media_player->seek(frame_pos);

  _prev_frame = frame;
}

void VideoDialog::seekByValue(double value)
{
  if( ui->radioButtonFrame->isChecked() )
  {
    ui->timeSlider->setValue(static_cast<int>(value));
  }
  else if( ui->radioButtonTime->isChecked() )
  {
    const auto& fps = _media_player->statistics().video.frame_rate;
    ui->timeSlider->setValue( static_cast<int>( value * 1000 / fps ));
  }
}

bool VideoDialog::eventFilter(QObject* obj, QEvent* ev)
{
  if( obj != ui->lineEditReference )
  {
    return false;
  }
  if (ev->type() == QEvent::DragEnter)
  {
    auto event = static_cast<QDragEnterEvent*>(ev);
    const QMimeData* mimeData = event->mimeData();
    QStringList mimeFormats = mimeData->formats();

    QString& format = mimeFormats.front();

    QByteArray encoded = mimeData->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    if (format != "curveslist/add_curve")
    {
      return false;
    }

    QStringList curves;
    while (!stream.atEnd())
    {
      QString curve_name;
      stream >> curve_name;
      if (!curve_name.isEmpty())
      {
        curves.push_back(curve_name);
      }
    }
    if (curves.size() != 1)
    {
      return false;
    }

    _dragging_curve = curves.front();
    event->acceptProposedAction();
    return true;
  }
  else if (ev->type() == QEvent::Drop)
  {
    auto lineEdit = qobject_cast<QLineEdit*>(obj);

    if (!lineEdit)
    {
      return false;
    }
    lineEdit->setText(_dragging_curve);
  }
  return false;
}

void VideoDialog::on_clearButton_clicked()
{
  _media_player->stop();
  ui->lineFilename->setText("");
  ui->lineEditReference->setText("");
}
