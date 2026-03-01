/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_STRING_DICT_INDEX_H
#define PJ_STRING_DICT_INDEX_H

#include <cstdint>
#include <limits>

namespace PJ
{

struct StringDictIndex
{
  uint32_t index;

  static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();

  StringDictIndex() : index(INVALID)
  {
  }
  explicit StringDictIndex(uint32_t idx) : index(idx)
  {
  }

  bool isValid() const
  {
    return index != INVALID;
  }

  bool operator==(const StringDictIndex& o) const
  {
    return index == o.index;
  }
  bool operator!=(const StringDictIndex& o) const
  {
    return index != o.index;
  }
};

}  // namespace PJ

#endif
