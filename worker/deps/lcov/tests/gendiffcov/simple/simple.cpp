/**
 * @file   simple.cpp
 * @author MTK50321 Henry Cox hcox@HC-EL6-4502226
 * @date   Mon Apr 13 13:00:49 2020
 * 
 * @brief  Test for differential coverage
 */

#include <iostream>

int
main(int ac, char ** av)
{
  int cond = 0;
  if (ac == 1)
  {
    std::cout << "ac == 1 - code exercised" << std::endl;// CBC
#ifdef ADD_CODE
    std::cout << " this code will be GIC" << std::endl;
#endif
#ifndef REMOVE_CODE
    cond = 1; // when this goes away, baseline coverage is reduced
    std::cout << " this code will be ECB" << std::endl;
#endif
    std::cout << " this code will be DCB" << std::endl;
  }
  else
  {
    // this code not hit in 'regress'
    cond = 2;
    std::cout << "ac = " << ac << std::endl;// UBC
#ifdef ADD_CODE
    std::cout << " this code will be UIC" << std::endl;
#endif
#ifndef REMOVE_CODE
    std::cout << " this code will be EUB" << std::endl;
#endif
    std::cout << " this code will be DUB" << std::endl;
  }
  if (cond == 1)
    std::cout << "cond == " << cond << "... code exercised" << std::endl;// LBC
  else if (cond == 2)
    std::cout << "cond == " << cond << "... code not exercised" << std::endl;
  return 0;
}
