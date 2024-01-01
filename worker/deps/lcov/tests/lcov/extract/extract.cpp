#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, const char *argv[])
{
  bool b = false;
  if (strcmp(argv[1], "1") == 0)
    b = true;

  char *a = nullptr;
  // TEST_BRANCH_START
  if (b) // TEST_BRANCH_LINE
    // TEST_BRANCH_STOP
    printf("Hai\n");
  delete[] a;

  std::string str("asdads");
  str = "cd";

  std::cout << str << std::endl;

  // LCOV_EXCL_START
  std::cout << "adding some code to ignore" << std::endl;
  // LCOV_EXCL_STOP
  return 0;
}
