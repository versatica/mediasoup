netstring = library(
  'netstring',
  'netstring-c/netstring.c',
  include_directories: include_directories(
    'netstring-c',
  ),
)

netstring_dep = declare_dependency(
  link_with: netstring,
  include_directories: include_directories(
    'netstring-c',
  ),
)
