/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

#include <exception>
#include <string>

namespace ulog_cpp {

class ExceptionBase : public std::exception {
 public:
  explicit ExceptionBase(std::string reason) : _reason(std::move(reason)) {}
  const char* what() const noexcept override { return _reason.c_str(); }

 protected:
  const std::string _reason;
};

/**
 * Data stream exception, either during serialization (writing) or deserialization (reading)
 */
class ParsingException : public ExceptionBase {
 public:
  explicit ParsingException(std::string reason) : ExceptionBase(std::move(reason)) {}
};

/**
 * API is used in the wrong way, or some arguments do not satisfy some requirements
 */
class UsageException : public ExceptionBase {
 public:
  explicit UsageException(std::string reason) : ExceptionBase(std::move(reason)) {}
};

/**
 * Some field/subscription does not exist or index is out of range
 */
class AccessException : public ExceptionBase {
 public:
  explicit AccessException(std::string reason) : ExceptionBase(std::move(reason)) {}
};

}  // namespace ulog_cpp
