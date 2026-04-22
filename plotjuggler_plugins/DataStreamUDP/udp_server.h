/*DataStreamServer PlotJuggler  Plugin license(Faircode)

Copyright(C) 2018 Philippe Gauthier - ISIR - UPMC
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

#include <QUdpSocket>
#include <QtPlugin>
#include <map>
#include <string>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"

using namespace PJ;

class UDP_Server : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  UDP_Server();

  virtual bool start(QStringList*) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override
  {
    return _running;
  }

  virtual ~UDP_Server() override;

  virtual const char* name() const override
  {
    return "UDP Server";
  }

  virtual bool isDebugPlugin() override
  {
    return false;
  }

private:
  bool _running;
  QUdpSocket* _udp_socket;

  // Parser factory kept around so we can lazily build per-ID parsers when
  // Multi-type dispatch is enabled and a new discriminator value arrives.
  PJ::ParserFactoryPlugin::Ptr _parser_creator;

  // Key is the decoded ID string ("" when dispatch is disabled, so the single
  // parser lives at _parsers[""]).
  std::map<std::string, PJ::MessageParserPtr> _parsers;

  // Transport-level discriminator config. All zero / false when disabled.
  bool _dispatch_enabled = false;
  int _dispatch_offset = 0;  // bytes from start of datagram
  int _dispatch_length = 1;  // 1 / 2 / 4 / 8
  bool _dispatch_little_endian = true;
  bool _dispatch_display_hex = false;

private slots:

  void processMessage();
};
