/***** MIT License ****
*
*   Copyright (c) 2016-2022 Davide Faconti
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*/

#ifndef ROS_INTROSPECTION_FieldTreeLeaf_H
#define ROS_INTROSPECTION_FieldTreeLeaf_H

#include <vector>
#include <map>
#include <iostream>

#include "rosx_introspection/ros_message.hpp"

// Brutally faster for numbers below 100
inline int print_number(char* buffer, uint16_t value)
{
  const char DIGITS[] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";
  if (value < 10)
  {
    buffer[0] = static_cast<char>('0' + value);
    return 1;
  }
  else if (value < 100) {
    value *= 2;
    buffer[0] = DIGITS[ value ];
    buffer[1] = DIGITS[ value+1 ];
    return 2;
  }
  else{
    return sprintf( buffer,"%d", value );
  }
}

namespace RosMsgParser{

/**
 * @brief The FieldTreeLeaf is, as the name suggests, a leaf (terminal node)
 * of a StringTree.
 * It provides the pointer to the node and a list of numbers that represent
 * the index that corresponds to the placeholder "#".
 *
 * For example if you want to represent the string
 *
 *      foo/2/bar/3/hello/world
 *
 * This would correspond to a branch of the tree (from root to the leaf) equal to these 6 nodes,
 * where "foo" is the root and "world" is the leaf
 *
 * foo -> # -> bar -> # ->hello -> world
 *
 * array_size will be equal to two and index_array will contain these numbers {2,3}
 *
 */
struct FieldLeaf
{
  const FieldTreeNode* node;
  SmallVector<uint16_t, 4> index_array;
};

struct FieldsVector
{
  FieldsVector() = default;

  FieldsVector(const FieldLeaf& leaf);

  SmallVector<const ROSField*, 8> fields;
  SmallVector<uint16_t, 4> index_array;

  /// Utility functions to print the entire branch
  void toStr(std::string &destination) const;

  std::string toStdString() const
  {
    std::string out;
    toStr(out);
    return out;
  }
};

//---------------------------------

inline std::ostream& operator<<(std::ostream &os, const FieldsVector& leaf )
{
  std::string dest;
  leaf.toStr(dest);
  os << dest;
  return os;
}


}

#endif // ROSTYPE_H
