/**
 * @file   branch.cpp
 * @author Henry Cox
 * @date   Wed Jun  7 09:25:11 2023
 *
 * @brief  test branches with different numbers of expressions on same line
 */

#include <iostream>

template <unsigned v>
void func(bool a, bool b)
{
  if ((v == 1 && a) ||  (v == 0 && a && b))
    std::cout << "true";
}

#ifdef MACRO
#  define EXPR a || b
#else
#  define EXPR a
#endif
int
main(int ac, char **av)
{
  bool a = ac > 1;
  bool b = ac > 2;

  if (EXPR) {
    std::cout << "EXPR was true" << std::endl;
  }
#ifdef MACRO
  func<1>(a, b);
#else
  func<0>(a, b);
#endif
  func<2>(a, b);
  return 0;
}
