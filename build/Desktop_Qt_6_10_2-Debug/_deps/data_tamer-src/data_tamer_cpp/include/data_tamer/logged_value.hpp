#pragma once

#include "data_tamer/types.hpp"
#include "data_tamer/details/locked_reference.hpp"

#include <memory>
#include <shared_mutex>

namespace DataTamer
{

class LogChannel;

/**
 * @brief The LoggedValue class is a container of a variable that
 * automatically register/unregister to a Channel when created/destroyed.
 *
 * Setting and getting values is thread-safe.
 */
template <typename T>
class LoggedValue
{
protected:
  LoggedValue(const std::shared_ptr<LogChannel>& channel, const std::string& name,
              T initial_value);

  friend LogChannel;

public:
  LoggedValue() {}

  ~LoggedValue();

  LoggedValue(LoggedValue const& other) = delete;
  LoggedValue& operator=(LoggedValue const& other) = delete;

  LoggedValue(LoggedValue&& other) = default;
  LoggedValue& operator=(LoggedValue&& other) = default;

  /**
   * @brief set the value of the variable.
   *
   * @param value   new value
   * @param auto_enable  if true and the current instance is disabled, call setEnabled(true)
   */
  void set(const T& value, bool auto_enable = true);

  /// @brief get the stored value.
  [[nodiscard]] T get();

  [[deprecated("use getMutablePtr() instead")]] [[nodiscard]] MutablePtr<T> getLockedPtr()
  {
    return getMutablePtr();
  }

  /** This method allows the user to access the object by reference (read/write access),
   *  in a thread-safe way.
   *
   *  Usage:
   *
   *    if(auto ref = value->getMutablePtr()) {
   *      auto current_value = *ref;
   *    };
   */
  [[nodiscard]] MutablePtr<T> getMutablePtr();

  /** This method allows the user to access the object as const reference (read-only)
   *  in a thread-safe way.
   *
   *  Usage:
   *
   *    if(auto ref = value->getMutablePtr()) {
   *      auto current_value = *ref;
   *    };
   */
  [[nodiscard]] ConstPtr<T> getConstPtr();

  /// @brief Disabling a LoggedValue means that we will not record it in the snapshot
  void setEnabled(bool enabled);

  [[nodiscard]] bool isEnabled() const { return enabled_; }

private:
  std::weak_ptr<LogChannel> channel_;
  T value_ = {};
  RegistrationID id_;
  bool enabled_ = true;
  std::shared_mutex rw_mutex_;
};

}  // namespace DataTamer
