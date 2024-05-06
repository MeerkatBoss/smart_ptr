#include <iostream>

#include "SmartPointers.hpp"

using namespace my_stl;

int main()
{
  int* raw = new int(10);
  SharedPtr<int> ptr(raw);

  SharedPtr<int> ptr2(std::move(ptr));

  std::cout << *ptr << ' ' << ptr.use_count() << '\n';
}
