/**
 * @file   initial.cpp
 * @brief  baseline code for function categorization test
 */

#include <iostream>

void
unchanged()
{
  std::cout << "called unchanged function" << std::endl;
}

void
unchanged_notCalled()
{
  std::cout << "called unchanged_notCalled function" << std::endl;
}

void
added()
{
  std::cout << "called added function" << std::endl;
}

void
added_notCalled()
{
  std::cout << "called add_notCalled function" << std::endl;
}

void
removed()
{
  std::cout << "called removed function" << std::endl;
  std::cout << "this line gets removed" << std::endl;
}

void
removed_notCalled()
{
  std::cout << "called removed_notCalled function" << std::endl;
  std::cout << "this line gets removed" << std::endl;
}

void
included()
{
  std::cout << "called included function" << std::endl;
#ifdef ADD_CODE
  std::cout << "this code excluded in baseline" << std::endl;
#endif
}

void
included_notCalled()
{
  std::cout << "called included_notCalled function" << std::endl;
#ifdef ADD_CODE
  std::cout << "this code excluded in baseline" << std::endl;
#endif
}

void
excluded()
{
  std::cout << "called excluded function" << std::endl;
#ifndef REMOVE_CODE
  std::cout << "this code excluded in current" << std::endl;
#endif
}

void
excluded_notCalled()
{
  std::cout << "called excluded_notCalled function" << std::endl;
#ifndef REMOVE_CODE
  std::cout << "this code excluded in current" << std::endl;
#endif
}

void deleted()
{
  std::cout << "function deleted gets deleted" << std::endl;
}

void deleted_notCalled()
{
  std::cout << "this deleted_notCalled gets deleted" << std::endl;
}

int main(int ac, char **av)
{
#ifdef CALL_FUNCTIONS
  unchanged();
  added();
  removed();
  included();
  excluded();
  deleted();
#endif
}
