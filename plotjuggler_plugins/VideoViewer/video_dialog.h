#ifndef VIDEO_DIALOG_H
#define VIDEO_DIALOG_H

#include <QDialog>
#include <QtAV>
#include <QSlider>
#include <QPushButton>
#include <QCloseEvent>
#include "ui_video_dialog.h"

class VideoDialog : public QDialog
{
  Q_OBJECT

public:
  explicit VideoDialog(QWidget *parent = nullptr);
  ~VideoDialog();

  QString referenceCurve() const;

  void pause(bool paused);

  bool isPaused() const;

  Ui::VideoDialog *ui;

  bool loadFile(QString filename);

private slots:
  void on_loadButton_clicked();

  void on_timeSlider_valueChanged(int value);

  void closeEvent (QCloseEvent *event)
  {
    emit closed();
  }

public Q_SLOTS:

  void seekByValue(double value);

private Q_SLOTS:
  void updateSliderPos(qint64 value);
  void updateSlider();
  void updateSliderUnit();

  void on_clearButton_clicked();

signals:

  void closed();

private:
  QtAV::VideoOutput *_video_output;
  QtAV::AVPlayer *_media_player;

  int _prev_frame = 0;

  bool eventFilter(QObject* obj, QEvent* ev);
  QString _dragging_curve;
};

#endif // VIDEO_DIALOG_H
