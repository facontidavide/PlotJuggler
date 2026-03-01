/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#pragma once
#include <utility>

#include "messages.hpp"

namespace ulog_cpp {

/**
 * TypedDataView is a view on a Data object, which allows to access the data using the
 * MessageFormat meta definition. The subscription object only contains the Data object,
 * without repeated reference to the underlying MessageFormat, for storage efficiency reasons.
 *
 * Note that TypedDataView should be a short-lived object solely to access data. As
 * it stores references to the underlying Data and MessageFormat objects, it is only valid
 * as long as the the MessageFormat and Data objects are untouched and valid.
 */
class TypedDataView {
 public:
  /**
   * Value is a view on a single field of a TypedDataView.
   * @param data_ref Reference to the underlying Data Object
   * @param message_format_ref  Reference to the underlying MessageFormat
   */
  TypedDataView(const Data& data_ref, const MessageFormat& message_format_ref)
      : _data_ref(data_ref), _message_format_ref(message_format_ref)
  {
  }

  /**
   * @return The name of the message format that this data object is based on
   */
  std::string name() const { return _message_format_ref.name(); }

  /**
   * Accesses the value at a specific field.
   * @param field The field to be accessed
   * @return The value object for the field
   */
  Value at(const Field& field) const
  {
    if (!field.definitionResolved()) {
      throw ParsingException("Field definition not resolved");
    }
    return Value(field, _data_ref.data());
  }

  /**
   * Accesses the value at a specific field.
   * @param field The field to be accessed
   * @return The value object for the field
   */
  Value at(const std::shared_ptr<Field>& field) const { return at(*field); }

  /**
   * Accesses the value at a specific field.
   * @param field_name
   * @return The value object for the field
   */
  Value at(const std::string& field_name) const
  {
    auto field = _message_format_ref.field(field_name);
    return at(field);
  }

  /**
   * Accesses the value at a specific field.
   * @param field_name
   * @return The value object for the field
   */
  Value operator[](const Field& field) const { return at(field); }

  /**
   * Accesses the value at a specific field.
   * @param field_name
   * @return The value object for the field
   */
  Value operator[](const std::shared_ptr<Field>& field) const { return at(field); }

  /**
   * Accesses the value at a specific field.
   * @param field_name
   * @return The value object for the field
   */
  Value operator[](const std::string& field_name) const { return at(field_name); }

  /**
   * @return The underlying MessageFormat object
   */
  const MessageFormat& format() const { return _message_format_ref; }

  /**
   * Checks if a field is present in the underlying MessageFormat
   */
  bool hasField(const std::string& field_name) const
  {
    const auto iter = _message_format_ref.fieldMap().find(field_name);
    return iter != _message_format_ref.fieldMap().end() && iter->second->definitionResolved();
  }

  /**
   * @return The underlying raw data vector
   */
  const std::vector<uint8_t>& rawData() const { return _data_ref.data(); }

 private:
  const Data& _data_ref;
  const MessageFormat& _message_format_ref;
};

/**
 * Iterator to iterate over a subscription. Subscriptions contain a vector of Data objects,
 * the Data objects themselves are not typed to a specific MessageFormat, but the Subscription is.
 * This iterator implicitly converts each Data object to a TypedDataView, which allows access to
 * all Fields.
 * @tparam base_iterator_type const iterator or modifiable iterator
 */
template <typename base_iterator_type>
class SubscriptionIterator {
 public:
  SubscriptionIterator(const base_iterator_type& it, std::shared_ptr<MessageFormat> message_format)
      : _it(it), _message_format(std::move(message_format))
  {
  }

  using iterator_category = std::random_access_iterator_tag;  // NOLINT(*-identifier-naming)
  using value_type = TypedDataView;                           // NOLINT(*-identifier-naming)
  using difference_type = std::ptrdiff_t;                     // NOLINT(*-identifier-naming)
  using pointer = TypedDataView*;                             // NOLINT(*-identifier-naming)
  using reference = TypedDataView&;                           // NOLINT(*-identifier-naming)

  TypedDataView operator*() { return TypedDataView(*_it, *_message_format); }

  TypedDataView operator*() const { return TypedDataView(*_it, *_message_format); }

  SubscriptionIterator& operator++()
  {
    ++_it;
    return *this;
  }

  SubscriptionIterator operator++(int)
  {
    SubscriptionIterator tmp(*this);
    operator++();
    return tmp;
  }

  SubscriptionIterator& operator--()
  {
    --_it;
    return *this;
  }

  SubscriptionIterator operator--(int)
  {
    SubscriptionIterator tmp(*this);
    operator--();
    return tmp;
  }

  SubscriptionIterator& operator+=(difference_type n)
  {
    _it += n;
    return *this;
  }

  SubscriptionIterator operator+(difference_type n) const
  {
    SubscriptionIterator tmp(*this);
    tmp += n;
    return tmp;
  }

  SubscriptionIterator& operator-=(difference_type n)
  {
    _it -= n;
    return *this;
  }

  SubscriptionIterator operator-(difference_type n) const
  {
    SubscriptionIterator tmp(*this);
    tmp -= n;
    return tmp;
  }

  difference_type operator-(const SubscriptionIterator& other) const { return _it - other._it; }

  TypedDataView operator[](difference_type n) { return *((*this) + n); }

  bool operator==(const SubscriptionIterator& other) const { return _it == other._it; }

  bool operator!=(const SubscriptionIterator& other) const { return _it != other._it; }

  bool operator<(const SubscriptionIterator& other) const { return _it < other._it; }

  bool operator>(const SubscriptionIterator& other) const { return _it > other._it; }

  bool operator<=(const SubscriptionIterator& other) const { return _it <= other._it; }

  bool operator>=(const SubscriptionIterator& other) const { return _it >= other._it; }

 private:
  base_iterator_type _it;
  std::shared_ptr<MessageFormat> _message_format;
};

class Subscription {
 public:
  Subscription(AddLoggedMessage add_logged_message, std::vector<Data> samples,
               std::shared_ptr<MessageFormat> message_format)
      : _add_logged_message(std::move(add_logged_message)),
        _message_format(std::move(message_format)),
        _samples(std::move(samples))
  {
  }

  void emplaceSample(const Data& sample) { _samples.emplace_back(sample); }

  const AddLoggedMessage& getAddLoggedMessage() const { return _add_logged_message; }

  const std::vector<Data>& rawSamples() const { return _samples; }

  const std::shared_ptr<MessageFormat>& format() const { return _message_format; }

  const std::map<std::string, std::shared_ptr<Field>>& fieldMap() const
  {
    return _message_format->fieldMap();
  }

  std::vector<std::string> fieldNames() const { return _message_format->fieldNames(); }

  std::shared_ptr<Field> field(const std::string& name) const
  {
    return _message_format->field(name);
  }

  auto begin()
  {
    return SubscriptionIterator<std::vector<Data>::iterator>(_samples.begin(), _message_format);
  }
  auto end()
  {
    return SubscriptionIterator<std::vector<Data>::iterator>(_samples.end(), _message_format);
  }

  auto begin() const
  {
    return SubscriptionIterator<std::vector<Data>::const_iterator>(_samples.begin(),
                                                                   _message_format);
  }
  auto end() const
  {
    return SubscriptionIterator<std::vector<Data>::const_iterator>(_samples.cend(),
                                                                   _message_format);
  }

  TypedDataView at(std::size_t n) const
  {
    if (n >= size()) {
      throw AccessException("Index out of range: " + std::to_string(n));
    }
    return begin()[n];
  }

  TypedDataView operator[](std::size_t n) { return at(n); }

  std::size_t size() const { return _samples.size(); }

 private:
  AddLoggedMessage _add_logged_message;
  std::shared_ptr<MessageFormat> _message_format;
  std::vector<Data> _samples;
};

}  // namespace ulog_cpp