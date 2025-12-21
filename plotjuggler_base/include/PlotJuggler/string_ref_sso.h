/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef STRING_REF_SSO_H
#define STRING_REF_SSO_H

#include <iostream>
#include <string.h>

namespace PJ
{
/**
 * @brief Super simple, unmutable, string_view with
 * small string optimization.
 * If the string is 15 bytes or less, it is copied, otherwise,
 * StringRef store a not-owning reference.
 */
class StringRef
{
private:
  static const uint64_t TYPE_BIT = uint64_t(1) << (sizeof(size_t) * 8 - 1);

  struct noSSO
  {
    const char* data;
    size_t size;
  };

  static const uint8_t SSO_SIZE = sizeof(noSSO) - 1;

  struct SSO
  {
    char data[sizeof(noSSO)];
  };

  union
  {
    noSSO no_sso;
    SSO sso;
  } _storage;

public:
  bool isSSO() const
  {
    return !(_storage.no_sso.size & TYPE_BIT);
  }

  StringRef() : StringRef(nullptr, 0)
  {
  }

  StringRef(const std::string& str) : StringRef(str.data(), str.size())
  {
  }

  StringRef(const char* str) : StringRef(str, strlen(str))
  {
  }

  StringRef(const std::string_view& str) : StringRef(str.data(), str.size())
  {
  }

  explicit StringRef(const char* data_ptr, size_t length)
  {
    _storage.no_sso.data = nullptr;
    _storage.no_sso.size = 0;

    if (length <= SSO_SIZE)
    {
      memcpy(_storage.sso.data, data_ptr, length);
      _storage.sso.data[SSO_SIZE] = SSO_SIZE - length;
    }
    else
    {
      _storage.no_sso.data = data_ptr;
      _storage.no_sso.size = length;
      _storage.no_sso.size |= TYPE_BIT;
    }
  }

  const char* data() const
  {
    return isSSO() ? _storage.sso.data : _storage.no_sso.data;
  }

  size_t size() const
  {
    return isSSO() ? (SSO_SIZE - _storage.sso.data[SSO_SIZE]) : _storage.no_sso.size & ~TYPE_BIT;
  }
};

}  // namespace PJ

#endif  // STRING_REF_SSO_H
