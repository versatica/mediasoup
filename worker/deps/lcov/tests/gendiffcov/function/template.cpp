// test genhtml function alias suppression

#include <iostream>

template <typename T, unsigned i=0>
void
func(const char * str)
{
  std::cout << str << ": sizeof(T) " << sizeof(T) << i << std::endl;
}

int
main(int ac, char **av)
{
  func<char, 1>("char/1");
  func<char, 2>("char/2");
  func<unsigned>("unsigned");
  return 0;
}
