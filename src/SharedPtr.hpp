/**
 * @file SharedPtr.hpp
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-05
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __SHARED_PTR_HPP
#define __SHARED_PTR_HPP

#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>

namespace my_stl {

template <typename T>
class WeakPtr;

namespace detail {

class ControlBlockBase {
 public:
  void add_strong_link() {
    assert(is_valid());
    ++strong_count_;
  }
  void add_weak_link() {
    assert(is_valid() || weak_count_ > 0);
    ++weak_count_;
  }

  void remove_strong_link();
  void remove_weak_link();

  bool is_valid() const { return get_use_count() > 0; }
  size_t get_use_count() const { return strong_count_; }

  template <typename T>
  T* get_data() {
    assert(is_valid());
    return static_cast<T*>(data_);
  }

 protected:
  ControlBlockBase(void* data) : data_(data) {}

  virtual void delete_data(void) = 0;
  virtual void delete_block(void) = 0;

 private:
  virtual ~ControlBlockBase() = default;

  void* data_;
  size_t strong_count_{1};
  size_t weak_count_{0};
};

template <typename T, typename Deleter, typename Alloc>
class PtrControlBlock final : public ControlBlockBase {
  using block_alloc =
      std::allocator_traits<Alloc>::template rebind_alloc<PtrControlBlock>;
  using block_alloc_traits = std::allocator_traits<block_alloc>;

 public:
  static PtrControlBlock* make_block(T* ptr, Deleter deleter, Alloc alloc);

 private:
  PtrControlBlock(T* ptr, Deleter deleter, Alloc alloc)
      : ControlBlockBase(ptr), deleter_(deleter), alloc_(alloc) {}

  virtual void delete_data(void) override;
  virtual void delete_block(void) override;

  [[no_unique_address]] Deleter deleter_;
  [[no_unique_address]] block_alloc alloc_;
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
class SharedPtr final {
 public:
  SharedPtr();

  template <std::derived_from<T> U, typename Deleter = DefaultDelete<U>,
            typename Alloc = std::allocator<U>>
  SharedPtr(T* ptr, Deleter deleter = Deleter(), Alloc alloc = Alloc());

  template <std::derived_from<T> U>
  SharedPtr(const SharedPtr<U>& other);

  template <std::derived_from<T> U>
  SharedPtr& operator=(const SharedPtr<U>& other);

  template <std::derived_from<T> U>
  SharedPtr(SharedPtr<U>&& other);

  template <std::derived_from<T> U>
  SharedPtr& operator=(SharedPtr<U>&& other);

  ~SharedPtr();

  size_t use_count() const;
  T* get() const;
  void reset();

  T& operator*() const;
  T* operator->() const;

 private:
  friend class WeakPtr<T>;
  SharedPtr(detail::ControlBlockBase* ctl_block);

  detail::ControlBlockBase* ctl_block_;
};

template <typename T>
class WeakPtr final
{
 public:
  WeakPtr();

  template <std::derived_from<T> U>
  WeakPtr(const WeakPtr<U>& other);

  template <std::derived_from<T> U>
  WeakPtr& operator=(const WeakPtr<U>& other);

  template <std::derived_from<T> U>
  WeakPtr(WeakPtr<U>&& other);

  template <std::derived_from<T> U>
  WeakPtr& operator=(WeakPtr<U>&& other);

  template <std::derived_from<T> U>
  WeakPtr(const SharedPtr<U>& owner);

  ~WeakPtr();

  bool expired() const { return !ctl_block_->is_valid(); }
  SharedPtr<T> lock() const { return SharedPtr<T>(ctl_block_); }

 private:
  detail::ControlBlockBase* ctl_block_;
};

}  // namespace my_stl

#endif /* SharedPtr.hpp */
