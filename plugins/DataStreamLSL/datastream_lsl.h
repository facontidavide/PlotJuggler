#pragma once
#include <QDialog>

#include <QtPlugin>
#include <QThread>
#include <QMap>
#include <QStringListModel>
#include <QStandardItemModel>
#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"
#include "ui_datastream_lsl.h"
#include "lsl_cpp.h"
#include "streamer.h"

class StreamLSLDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StreamLSLDialog(QWidget *parent = nullptr);
    ~StreamLSLDialog();

    QStringList getSelectedStreams();

private:
    void resolveLSLStreams();

private:
    Ui::DataStreamLSL *ui;
    QStandardItemModel model;
};


class DataStreamLSL : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  DataStreamLSL();

  virtual ~DataStreamLSL() override;

  virtual bool start(QStringList*) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override
  {
    return _running;
  }

  virtual const char* name() const override
  {
    return "LSL Subscriber";
  }

  virtual bool isDebugPlugin() override
  {
    return false;
  }

public slots:
    void dataReceived( std::vector< std::vector<double> > *chunk, std::vector<double> *stamps);

private:
  bool _running;

  QMap<QThread *, Streamer *> _streams;
};
