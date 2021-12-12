#ifndef STATE_PUBLISHER_VIDEO_VIEWER_H
#define STATE_PUBLISHER_VIDEO_VIEWER_H

#include <QObject>
#include <QtPlugin>
#include <zmq.hpp>
#include <thread>
#include <mutex>
#include "PlotJuggler/statepublisher_base.h"
#include "video_dialog.h"

class PublisherVideo : public PJ::StatePublisher
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.StatePublisher")
  Q_INTERFACES(PJ::StatePublisher)

public:
  PublisherVideo();

  virtual ~PublisherVideo() {}

  virtual const char* name() const override
  {
    return "Video Viewer";
  }

  virtual bool enabled() const override
  {
    return _enabled;
  }

  virtual void updateState(double current_time) override
  {
  }

  virtual void play(double current_time) override
  {
  }

public slots:
  virtual void setEnabled(bool enabled) override;

private:

  bool _enabled = false;

  VideoDialog* _dialog;
};

#endif  // STATE_PUBLISHER_VIDEO_VIEWER_H
