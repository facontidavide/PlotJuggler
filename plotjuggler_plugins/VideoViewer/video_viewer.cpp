#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include "video_viewer.h"


PublisherVideo::PublisherVideo()
{
  _dialog = new VideoDialog(nullptr);
}

void PublisherVideo::setEnabled(bool enabled)
{
  _dialog->show();

}
