#pragma once

#include <shared_mutex>

using Mutex = std::shared_mutex;

/**
 * @brief The ConstPtr is a wrapper to a const
 * pointer that locks a mutex in the constructor
 * and unlocks it in the destructor.
 */
template <typename T>
class ConstPtr
{
public:
  ConstPtr(const T* obj, Mutex* mutex);
  ConstPtr(const ConstPtr&) = delete;
  ConstPtr& operator=(const ConstPtr&) = delete;
  ConstPtr(ConstPtr&&);
  ConstPtr& operator=(ConstPtr&&);
  ~ConstPtr();

  operator bool() const;
  Mutex* mutex();
  const T& operator*() const;
  const T* operator->() const;

private:
  const T* obj_ = nullptr;
  Mutex* mutex_;
};

/**
 * @brief The ConstPtr is a wrapper to a
 * pointer that locks a mutex in the constructor
 * and unlocks it in the destructor.
 */
template <typename T>
class MutablePtr
{
public:
  MutablePtr(T* obj, Mutex* mutex);
  MutablePtr(const MutablePtr&) = delete;
  MutablePtr& operator=(const MutablePtr&) = delete;
  MutablePtr(MutablePtr&&);
  MutablePtr& operator=(MutablePtr&&);
  ~MutablePtr();

  operator bool() const;
  Mutex* mutex();
  T& operator*();
  T* operator->();

private:
  T* obj_ = nullptr;
  Mutex* mutex_;
};

//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------

template <typename T>
inline ConstPtr<T>::ConstPtr(const T* obj, Mutex* mutex) : obj_(obj), mutex_(mutex)
{
  if(mutex_)
  {
    mutex_->lock_shared();
  }
}

template <typename T>
inline ConstPtr<T>::ConstPtr(ConstPtr&& other) : obj_(other.obj_), mutex_(other.mutex_)
{}

template <typename T>
inline ConstPtr<T>& ConstPtr<T>::operator=(ConstPtr&& other)
{
  mutex_ = other.mutex_;
  std::swap(obj_, other.obj_);
  return *this;
}

template <typename T>
inline ConstPtr<T>::~ConstPtr()
{
  if(mutex_)
  {
    mutex_->unlock_shared();
  }
}

template <typename T>
inline ConstPtr<T>::operator bool() const
{
  return obj_ != nullptr;
}

template <typename T>
inline Mutex *ConstPtr<T>::mutex()
{
  return mutex_;
}

template <typename T>
inline const T& ConstPtr<T>::operator*() const
{
  return *obj_;
}

template <typename T>
inline const T* ConstPtr<T>::operator->() const
{
  return obj_;
}

//----------------------------------------------------

template <typename T>
inline MutablePtr<T>::MutablePtr(T* obj, Mutex *mutex) : obj_(obj), mutex_(mutex)
{
  if(mutex_)
  {
    mutex_->lock();
  }
}

template <typename T>
inline MutablePtr<T>::MutablePtr(MutablePtr&& other) : mutex_(other.mutex_)
{
  std::swap(obj_, other.obj_);
}

template <typename T>
inline MutablePtr<T>& MutablePtr<T>::operator=(MutablePtr<T>&& other)
{
  mutex_ = other.mutex_;
  std::swap(obj_, other.obj_);
  return *this;
}

template <typename T>
inline MutablePtr<T>::~MutablePtr()
{
  if(mutex_)
  {
    mutex_->unlock();
  }
}

template <typename T>
inline MutablePtr<T>::operator bool() const
{
  return obj_ != nullptr;
}

template <typename T>
inline Mutex* MutablePtr<T>::mutex()
{
  return mutex_;
}

template <typename T>
inline T& MutablePtr<T>::operator*()
{
  return *obj_;
}

template <typename T>
inline T* MutablePtr<T>::operator->()
{
  return obj_;
}
