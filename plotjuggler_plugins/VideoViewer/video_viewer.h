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

  virtual ~PublisherVideo();

  const char* name() const override
  {
    return "Video Viewer";
  }

  bool enabled() const override
  {
    return _enabled;
  }

  void updateState(double current_time) override;

  void play(double current_time) override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

public slots:
  virtual void setEnabled(bool enabled) override;

private:

  bool _enabled = false;
  bool _xml_loaded = false;

  VideoDialog* _dialog = nullptr;
};

#endif  // STATE_PUBLISHER_VIDEO_VIEWER_H
