#include "video_dialog.h"
#include "ui_video_dialog.h"
#include <QSettings>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QSlider>
#include <QDebug>
#include <QVideoWidget>

VideoDialog::VideoDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::VideoDialog)
{
  ui->setupUi(this);

  //  _media_player = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
  //  QVideoWidget *video_widget = new QVideoWidget(this);
  //  ui->verticalLayout->addWidget(video_widget, 1);
  //  _media_player->setVideoOutput(video_widget);
  //  connect(_media_player, &QMediaPlayer::stateChanged,
  //          this, &VideoDialog::mediaStateChanged);
  //  connect(_media_player, &QMediaPlayer::positionChanged,
  //          this, &VideoDialog::positionChanged);
  //  connect(_media_player, &QMediaPlayer::durationChanged,
  //          this, &VideoDialog::durationChanged);
  //  connect(_media_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
  //          this, &VideoDialog::handleError);

  const char * const vlc_args[1] = {
    "--verbose=2" //be much more verbose then normal for debugging purpose
  };

  //create a new libvlc instance
  _vlcinstance = libvlc_new(1, vlc_args);

  if( !_vlcinstance )
  {
    throw std::runtime_error(libvlc_errmsg());
  }
  _media_player = libvlc_media_player_new (_vlcinstance);

  libvlc_video_set_key_input(_media_player, false);
  libvlc_video_set_mouse_input(_media_player, false);
}

VideoDialog::~VideoDialog()
{
  delete ui;
}

void VideoDialog::on_loadButton_clicked()
{
  QSettings settings;

  QString directory_path =
      settings.value("VideoDialog.loadDirectory", QDir::currentPath()).toString();

  QString filename = QFileDialog::getOpenFileName(this, tr("Load Video"),
                                                  directory_path, tr("(*.*)"));
  if (filename.isEmpty())
  {
    return;
  }

  //  _media_player->setMedia(QUrl(filename));
  //  _media_player->play();

  _media = libvlc_media_new_path (_vlcinstance, filename.toStdString().c_str());
  libvlc_media_player_set_media (_media_player, _media);
#if defined(Q_OS_WIN)
  uint32_t winId = reinterpret_cast<uint32_t>(_videoWidget->winId());
  libvlc_media_player_set_drawable(_media_player, winId );
#elif defined(Q_OS_MAC)
  libvlc_media_player_set_drawable(_media_player, _videoWidget->winId() );
#else //Linux
  uint32_t winId = ui->videoFrame->winId();
  libvlc_media_player_set_xwindow (_media_player, winId );
#endif

  auto duration_ms = libvlc_media_player_get_length(_media_player);
  ui->timeSlider->setRange(0, duration_ms);

  ui->lineFilename->setText(filename) ;

  directory_path = QFileInfo(filename).absolutePath();
  settings.setValue("VideoDialog.loadDirectory", directory_path);
}

void VideoDialog::mediaStateChanged(QMediaPlayer::State state)
{
  switch(state)
  {
    case QMediaPlayer::PlayingState:

      break;
    default:

      break;
  }
}

void VideoDialog::handleError()
{
  //  const QString errorString = _media_player->errorString();
}

void VideoDialog::on_timeSlider_valueChanged(int value)
{
  if(!_media)
  {
    return;
  }
  libvlc_media_player_set_time(_media_player, value);
}
