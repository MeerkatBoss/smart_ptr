/**
 * @file SmartPointers.hpp
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-05
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __SMART_POINTERS_HPP
#define __SMART_POINTERS_HPP

#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>

namespace my_stl {

namespace detail {

class ControlBlockBase {
 public:
  // Non-Copyable
  ControlBlockBase(const ControlBlockBase&) = delete;
  ControlBlockBase& operator=(const ControlBlockBase&) = delete;

  // Non-Movable
  ControlBlockBase(ControlBlockBase&&) = delete;
  ControlBlockBase& operator=(ControlBlockBase&&) = delete;

  void add_strong_link() { ++strong_count_; }
  void add_weak_link() { ++weak_count_; }

  void remove_strong_link() {
    assert(is_valid());
    --strong_count_;
    if (strong_count_ == 0) {
      delete_data();

      if (weak_count_ == 0) {
        delete_block();
      }
    }
  }

  void remove_weak_link() {
    assert(is_valid() || weak_count_ > 0);
    --weak_count_;
    if (strong_count_ == 0 && weak_count_ == 0) {
      delete_block();
    }
  }

  bool is_valid() const { return get_use_count() > 0; }
  size_t get_use_count() const { return strong_count_; }

  template <typename T>
  T* get_data() {
    return static_cast<T*>(data_);
  }

 protected:
  ControlBlockBase(void* data) : data_(data) {}

  void set_data(void* data) { data_ = data; }

  virtual void delete_data(void) = 0;
  virtual void delete_block(void) = 0;

  virtual ~ControlBlockBase() = default;

 private:
  void* data_;
  size_t strong_count_{0};
  size_t weak_count_{0};
};

}  // namespace detail

template <typename T>
struct DefaultDelete {
  void operator()(T* data) const { delete data; }
};

template <typename T>
struct DefaultDelete<T[]> {
  void operator()(T* data) const { delete[] data; }
};

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr final {
  template <typename U>
  friend class SharedPtr;

  template <typename U, typename... Ts>
  friend SharedPtr<U> MakeShared(Ts&&... args);

  template <typename U, typename Alloc, typename... Ts>
  friend SharedPtr<U> AllocateShared(const Alloc& alloc, Ts&&... args);

  friend class WeakPtr<T>;

 public:
  SharedPtr() : ctl_block_{nullptr} {}
  SharedPtr(std::nullptr_t) : SharedPtr() {}

  // Copyable
  SharedPtr(const SharedPtr& other) : SharedPtr(other.ctl_block_) {}
  SharedPtr& operator=(const SharedPtr& other) {
    if (this == &other) {
      return *this;
    }
    reset();
    ctl_block_ = other.ctl_block_;
    ctl_block_->add_strong_link();
    return *this;
  }

  // Movable
  SharedPtr(SharedPtr&& other) : SharedPtr(other.ctl_block_) { other.reset(); }
  SharedPtr& operator=(SharedPtr&& other) {
    if (this == &other) {
      return *this;
    }
    reset();
    std::swap(ctl_block_, other.ctl_block_);
    return *this;
  }

  template <typename Deleter = DefaultDelete<T>,
            typename Alloc = std::allocator<T>>
  SharedPtr(T* ptr, Deleter deleter = Deleter(), Alloc alloc = Alloc());

  template <std::derived_from<T> U, typename Deleter = DefaultDelete<U>,
            typename Alloc = std::allocator<U>>
  SharedPtr(U* ptr, Deleter deleter = Deleter(), Alloc alloc = Alloc());

  template <std::derived_from<T> U>
  SharedPtr(const SharedPtr<U>& other) : SharedPtr(other.ctl_block_) {}

  template <std::derived_from<T> U>
  SharedPtr& operator=(const SharedPtr<U>& other) {
    reset();
    ctl_block_ = other.ctl_block_;
    ctl_block_->add_strong_link();
    return *this;
  }

  template <std::derived_from<T> U>
  SharedPtr(SharedPtr<U>&& other) : SharedPtr(other.ctl_block_) {
    other.reset();
  }

  template <std::derived_from<T> U>
  SharedPtr& operator=(SharedPtr<U>&& other) {
    reset();
    std::swap(ctl_block_, other.ctl_block_);
    return *this;
  }

  ~SharedPtr() { reset(); }

  size_t use_count() const {
    return ctl_block_ == nullptr ? 0 : ctl_block_->get_use_count();
  }

  T* get() const {
    return ctl_block_ == nullptr ? nullptr : ctl_block_->get_data<T>();
  }

  void reset() {
    if (ctl_block_ != nullptr) {
      ctl_block_->remove_strong_link();
    }
    ctl_block_ = nullptr;
  }

  T& operator*() const {
    assert(ctl_block_ != nullptr);
    assert(ctl_block_->is_valid());
    return *ctl_block_->get_data<T>();
  }

  T* operator->() const { return &**this; }

 private:
  SharedPtr(detail::ControlBlockBase* ctl_block) : ctl_block_(ctl_block) {
    if (ctl_block_ != nullptr) {
      ctl_block_->add_strong_link();
    }
  }

  detail::ControlBlockBase* ctl_block_;
};

template <typename T, typename Alloc, typename... Ts>
SharedPtr<T> AllocateShared(const Alloc& alloc, Ts&&... args);

template <typename T, typename... Ts>
SharedPtr<T> MakeShared(Ts&&... args) {
  return AllocateShared<T>(std::allocator<T>(), std::forward<Ts>(args)...);
}

template <typename T>
class WeakPtr final {
 public:
  WeakPtr() : WeakPtr(nullptr) {}

  template <std::derived_from<T> U>
  WeakPtr(const WeakPtr<U>& other) : WeakPtr(other.ctl_block_) {}

  template <std::derived_from<T> U>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    reset();
    ctl_block_ = other.ctl_block_;
    ctl_block_->add_weak_link();
    return *this;
  }

  template <std::derived_from<T> U>
  WeakPtr(WeakPtr<U>&& other) : WeakPtr(other.ctl_block_) {
    other.reset();
  }

  template <std::derived_from<T> U>
  WeakPtr& operator=(WeakPtr<U>&& other) {
    reset();
    std::swap(ctl_block_, other.ctl_block_);
    return *this;
  }

  template <std::derived_from<T> U>
  WeakPtr(const SharedPtr<U>& owner) : WeakPtr(owner.ctl_block_) {}

  ~WeakPtr() { reset(); }

  void reset() {
    if (ctl_block_ != nullptr) {
      ctl_block_->remove_weak_link();
      ctl_block_ = nullptr;
    }
  }
  bool expired() const {
    return ctl_block_ == nullptr || !ctl_block_->is_valid();
  }
  SharedPtr<T> lock() const { return SharedPtr<T>(ctl_block_); }

 private:
  WeakPtr(detail::ControlBlockBase* ctl_block) : ctl_block_(ctl_block) {
    if (ctl_block_ != nullptr) {
      ctl_block_->add_weak_link();
    }
  }

  detail::ControlBlockBase* ctl_block_;
};

}  // namespace my_stl

#ifndef __SMART_POINTERS_IPP
#include "SmartPointers.ipp"
#endif

#endif /* SmartPointers.hpp */
