#include <iostream>

#include "SmartPointers.hpp"

using namespace my_stl;

int main()
{
  SharedPtr<int> ptr(new int{1});
  *ptr = 3;
  SharedPtr<int> copy = ptr;
  SharedPtr<int> copy2;
  copy = ptr;

  std::cout << *ptr << ' ' << ptr.use_count() << '\n';
  return 0;
}
