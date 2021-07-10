getopt = library(
  'getopt',
  'getopt/src/getopt.c',
  include_directories: include_directories(
    'getopt/src',
  ),
)

getopt_dep = declare_dependency(
  link_with: getopt,
  include_directories: include_directories(
    'getopt/src',
  ),
)
