#ifndef VIDEO_DIALOG_H
#define VIDEO_DIALOG_H

#include <QDialog>
#include <QMediaPlayer>
#include <vlc/vlc.h>

namespace Ui {
class VideoDialog;
}

class VideoDialog : public QDialog
{
  Q_OBJECT

public:
  explicit VideoDialog(QWidget *parent = nullptr);
  ~VideoDialog();

private slots:
  void on_loadButton_clicked();

  void on_timeSlider_valueChanged(int value);

private:
  Ui::VideoDialog *ui;
//  QMediaPlayer* _media_player;

  libvlc_instance_t *_vlcinstance;
  libvlc_media_player_t *_media_player;
  libvlc_media_t *_media;

  void mediaStateChanged(QMediaPlayer::State state);

  void positionChanged(qint64 position)
  {

  }

  void durationChanged(qint64 duration)
  {

  }

  void setPosition(int position)
  {
//    _media_player->setPosition(position);
  }

  void handleError();
};

#endif // VIDEO_DIALOG_H
