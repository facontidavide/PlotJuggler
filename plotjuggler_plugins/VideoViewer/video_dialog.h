#ifndef VIDEO_DIALOG_H
#define VIDEO_DIALOG_H

#include <memory>
#include <QDialog>
#include <QtAV>
#include <QSlider>
#include <QPushButton>
#include <QCloseEvent>
#include <QtAV/FrameReader.h>
#include "ui_video_dialog.h"

#include "qoi.h"

class ImageLabel : public QWidget
{
  Q_OBJECT

public:
  explicit ImageLabel(QWidget *parent = nullptr);
  const QPixmap* pixmap() const;

public slots:
  void setPixmap(const QPixmap&);

protected:
  void paintEvent(QPaintEvent *);

private:
  QPixmap pix;
};


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

  void on_decodeButton_clicked();

signals:

  void closed();

private:
  QtAV::VideoOutput *_video_output;
  QtAV::AVPlayer *_media_player;
  std::unique_ptr<QtAV::FrameReader> _frame_reader;
  //std::vector<QImage> _frames;
  struct CompressedFrame
  {
    CompressedFrame(): length(0), data(nullptr) {}
    CompressedFrame(const CompressedFrame&) = delete;
    CompressedFrame(CompressedFrame&& other): length(0), data(nullptr)
    {
      std::swap(other.data, data);
      std::swap(other.length, length);
    }

    ~CompressedFrame()
    {
      if(data) free(data);
    }

    int length;
    qoi_desc info;
    void* data;
  };
  std::vector<CompressedFrame> _compressed_frames;

  bool eventFilter(QObject* obj, QEvent* ev);
  QString _dragging_curve;

  ImageLabel *_label;

  bool _decoded = false;
};

#endif // VIDEO_DIALOG_H
