/*DatastreamSerialPort PlotJuggler  Plugin license(Faircode)

Copyright(C) 2026 Valentin Platzgummer
Permission is hereby granted to any person obtaining a copy of this software and
associated documentation files(the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and / or sell copies("Use") of the Software, and to permit persons
to whom the Software is furnished to do so. The above copyright notice and this permission
notice shall be included in all copies or substantial portions of the Software. THE
SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#pragma once

#include <QtPlugin>
#include <QByteArray>
#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"

class QSerialPort;
class MsgSplitter;

using namespace PJ;

class DatastreamSerialPort : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  DatastreamSerialPort();

  virtual bool start(QStringList*) override;
  virtual void shutdown() override;
  virtual bool isRunning() const override
  {
    return _running;
  }
  virtual ~DatastreamSerialPort() override;
  virtual const char* name() const override
  {
    return "Serial Port";
  }
  virtual bool isDebugPlugin() override
  {
    return false;
  }

private:
  bool _running;
  PJ::MessageParserPtr _parser;
  QSerialPort* _serialPort;
  MsgSplitter* _splitter;

private slots:

  void processMessage();
};
