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
#include <QPixmap>
#include <QImage>
#include <QProgressDialog>

#include "PlotJuggler/svg_util.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

ImageLabel::ImageLabel(QWidget *parent) :
  QWidget(parent)
{
}

const QPixmap* ImageLabel::pixmap() const {
  return &pix;
}

void ImageLabel::setPixmap (const QPixmap &pixmap){
  pix = pixmap;
}

void ImageLabel::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  if (pix.isNull())
  {
    return;
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  QSize rect_size = event->rect().size();
  QSize pix_size = pix.size();
  pix_size.scale(event->rect().size(), Qt::KeepAspectRatio);

  QPoint corner((rect_size.width() - pix_size.width()) / 2,
                (rect_size.height() - pix_size.height()) / 2);

  QPixmap scaled_pix = pix.scaled(pix_size,
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
  painter.drawPixmap(corner, scaled_pix);
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
  ui->verticalLayoutMain->addWidget(_video_output->widget(), 1.0);

  connect(_media_player, &AVPlayer::started,
          this, &VideoDialog::updateSlider);

  ui->lineEditReference->installEventFilter(this);

  QSettings settings;
  QString theme = settings.value("Preferences::theme", "light").toString();
  ui->clearButton->setIcon(LoadSvg(":/resources/svg/trash.svg", theme));

  _label = new ImageLabel;
  ui->verticalLayoutMain->addWidget(_label, 1.0);
  _label->setHidden(true);
}

VideoDialog::~VideoDialog()
{
  delete ui;
}

bool VideoDialog::loadFile(QString filename)
{
  if(!filename.isEmpty() && QFileInfo::exists(filename))
  {
    _media_player->play(filename);
    _media_player->pause(true);
    ui->lineFilename->setText(filename);

    _frame_reader = std::make_unique<QtAV::FrameReader>();
    _frame_reader->setMedia(filename);
    _compressed_frames.clear();
    ui->decodeButton->setEnabled(true);

    _decoded = false;

    _video_output->widget()->setHidden(false);
    _label->setHidden(true);
    return true;
  }
  return false;
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

  ui->timeSlider->setEnabled(true);

  if(_media_player->isSeekable() == false || duration_ms == 0)
  {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Video is not seekable");
    msgBox.setText(tr("Video is not seekable. You will need to decode the video into individual frames.\n"));
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton* button = msgBox.addButton(tr("Decode!"), QMessageBox::YesRole);
    msgBox.setDefaultButton(button);

    int res = msgBox.exec();

    if (res < 0 || res == QMessageBox::Cancel)
    {
      ui->timeSlider->setEnabled(false);
    }
    else{
      on_decodeButton_clicked();
    }
  }
}

void VideoDialog::on_timeSlider_valueChanged(int num)
{
  double fps = _media_player->statistics().video.frame_rate;
  double period = 1000 / fps;
  qint64 frame_pos = static_cast<qint64>(qreal(num) * period );

  if( _decoded )
  {
    num = std::max(0, num);
    num = std::min(int(_compressed_frames.size()-1), num);

    auto& frame = _compressed_frames[num];
    void* data = qoi_decode( frame.data, frame.length, &frame.info, 3);
    QImage image(static_cast<uchar*>(data), frame.info.width, frame.info.height, QImage::Format_RGB888);

    //qDebug() << "ratio: " << double(3*frame.info.width*frame.info.height) / double(frame.length);

    _label->setPixmap( QPixmap::fromImage( image ) );
    _label->repaint();
    free(data);
  }
  else
  {
    _media_player->setSeekType(QtAV::SeekType::AccurateSeek);
    _media_player->seek(frame_pos);
  }
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
    lineEdit->setText(_dragging_curve);
  }
  return false;
}

void VideoDialog::on_clearButton_clicked()
{
  _media_player->stop();
  ui->lineFilename->setText("");
  ui->lineEditReference->setText("");
  ui->timeSlider->setEnabled(false);
  ui->timeSlider->setValue(0);
  ui->decodeButton->setEnabled(false);

  _decoded = false;
  _video_output->widget()->setHidden(false);
  _label->setHidden(true);
  _compressed_frames.clear();
}


void VideoDialog::on_decodeButton_clicked()
{
  if( _decoded )
  {
    return;
  }

  double fps = _media_player->statistics().video.frame_rate;
  QProgressDialog progress_dialog;
  progress_dialog.setWindowTitle("PlotJuggler Video");
  progress_dialog.setLabelText("Decoding file");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, _media_player->duration() * fps / 1000);
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  int count = 0;
  while (_frame_reader->readMore())
  {
    while (_frame_reader->hasEnoughVideoFrames())
    {
      const QtAV::VideoFrame frame = _frame_reader->getVideoFrame();
      if (!frame)
      {
        continue;
      }

      QImage image = frame.toImage(QImage::Format_RGB888);

      CompressedFrame compressed_frame;
      compressed_frame.info.width = frame.width();
      compressed_frame.info.height = frame.height();
      compressed_frame.info.channels = 3;
      compressed_frame.info.colorspace = QOI_LINEAR;
      compressed_frame.data = qoi_encode(image.bits(), &compressed_frame.info, &compressed_frame.length);

//      _frames.push_back( std::move(image) );
      _compressed_frames.push_back( std::move(compressed_frame) );

      if( ++count % 10 == 0 )
      {
        progress_dialog.setValue(count);
        QApplication::processEvents();
        if (progress_dialog.wasCanceled())
        {
          _compressed_frames.clear();
          return;
        }
      }
    }
  }
  _decoded = true;
  _video_output->widget()->hide();
  _label->setHidden(false);

  ui->decodeButton->setEnabled(false);
  ui->timeSlider->setRange(0, _compressed_frames.size()-1);
  on_timeSlider_valueChanged( ui->timeSlider->value() );
}
