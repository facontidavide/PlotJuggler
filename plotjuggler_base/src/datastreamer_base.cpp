/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "PlotJuggler/datastreamer_base.h"

namespace PJ
{

void PJ::DataStreamer::setMaximumRangeX(double range)
{
  std::lock_guard<std::mutex> lock(mutex());
  for (auto& it : dataMap().numeric)
  {
    it.second.setMaximumRangeX(range);
  }
  for (auto& it : dataMap().strings)
  {
    it.second.setMaximumRangeX(range);
  }
  for (auto& it : dataMap().user_defined)
  {
    it.second.setMaximumRangeX(range);
  }
}

void DataStreamer::setParserFactories(ParserFactories *parsers)
{
  _parser_factories = parsers;
}

const ParserFactories *DataStreamer::parserFactories() const
{
  return _parser_factories;
}

}  // namespace PJ
