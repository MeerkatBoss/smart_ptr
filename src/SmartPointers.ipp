/**
 * @file SmartPointers.ipp
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-05
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __SMART_POINTERS_IPP
#define __SMART_POINTERS_IPP

#include <memory>
#include <utility>

#include "SmartPointers.hpp"

namespace my_stl {

namespace detail {

template <typename T, typename Deleter, typename Alloc>
class PtrControlBlock final : public ControlBlockBase {
  using block_alloc = typename
      std::allocator_traits<Alloc>::template rebind_alloc<PtrControlBlock>;
  using block_alloc_traits = std::allocator_traits<block_alloc>;

 public:
  PtrControlBlock(T* ptr, Deleter deleter, Alloc alloc)
      : ControlBlockBase(ptr), deleter_(deleter), alloc_(alloc) {}

  ~PtrControlBlock() = default;

  static PtrControlBlock* make_block(T* data, Deleter deleter, Alloc alloc) {
    block_alloc blk_alloc(alloc);
    PtrControlBlock* block = block_alloc_traits::allocate(blk_alloc, 1);
    block_alloc_traits::construct(blk_alloc, block, data, deleter, alloc);

    return block;
  }

 private:
  void delete_data(void) override { deleter_(get_data<T>()); }

  void delete_block(void) override {
    block_alloc alloc = std::move(alloc_);

    block_alloc_traits::destroy(alloc, this);
    block_alloc_traits::deallocate(alloc, this, 1);
  }

  [[no_unique_address]] Deleter deleter_;
  [[no_unique_address]] block_alloc alloc_;
};

template <typename T, typename Alloc>
class ValueControlBlock final : public ControlBlockBase {
  using block_alloc = typename
      std::allocator_traits<Alloc>::template rebind_alloc<ValueControlBlock>;
  using block_alloc_traits = std::allocator_traits<block_alloc>;

 public:
  template <typename... Ts>
  ValueControlBlock(Alloc alloc, Ts... args)
      : ControlBlockBase(nullptr),
        value(std::forward<Ts>(args)...),
        alloc_(alloc) {
    set_data(&value);
  }

  ~ValueControlBlock() {}

  template <typename... Ts>
  static ValueControlBlock* make_block(Alloc alloc, Ts... args) {
    block_alloc blk_alloc(alloc);
    ValueControlBlock* block = block_alloc_traits::allocate(blk_alloc, 1);
    block_alloc_traits::construct(blk_alloc, block, alloc,
                                  std::forward<Ts>(args)...);

    return block;
  }

 private:
  void delete_data(void) override { value.~T(); }

  void delete_block(void) override {
    block_alloc alloc = std::move(alloc_);

    block_alloc_traits::destroy(alloc, this);
    block_alloc_traits::deallocate(alloc, this, 1);
  }

  union {
    T value;
    char raw_data[sizeof(T)];
  };
  [[no_unique_address]] block_alloc alloc_;
};

}  // namespace detail

template <typename T>
template <typename Deleter, typename Alloc>
SharedPtr<T>::SharedPtr(T* ptr, Deleter deleter, Alloc alloc)
    : SharedPtr(detail::PtrControlBlock<T, Deleter, Alloc>::make_block(
          ptr, deleter, alloc)) {}

template <typename T>
template <std::derived_from<T> U, typename Deleter, typename Alloc>
SharedPtr<T>::SharedPtr(U* ptr, Deleter deleter, Alloc alloc)
    : SharedPtr(detail::PtrControlBlock<U, Deleter, Alloc>::make_block(
          ptr, deleter, alloc)) {}

template <typename T, typename Alloc, typename... Ts>
SharedPtr<T> AllocateShared(const Alloc& alloc, Ts&&... args) {
  return SharedPtr<T>(detail::ValueControlBlock<T, Alloc>::make_block(
      alloc, std::forward<Ts>(args)...));
}

}  // namespace my_stl

#endif /* SmartPointers.ipp */
