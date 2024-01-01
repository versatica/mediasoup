#include <string>
#include <stdio.h>

int
main(int ac, char **av)
{
  // LCOV_EXCL_EXCEPTION_BR_START
  printf("Simple branching:  argc=%d\n", ac);
  // LCOV_EXCL_EXCEPTION_BR_STOP
  int branch(0);

  if (ac == 1)
    branch = 1;
  else
    branch = 2;

  printf("std::string creating branches...\n");
  std::string name("name1"); // LCOV_EXCL_EXCEPTION_BR_LINE
  std::size_t len = name.length();
  name.resize(len + 10);
  
  return 0;
}
