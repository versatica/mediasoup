project(
  'wingetopt',
  'c',
  version: '0.95',
  license : ['ISC', 'BSD-3-Clause'],
  meson_version: '>= 0.55',
  default_options: [
    'warning_level=1',
  ],
)

compiler = meson.get_compiler('c')

c_args = []

if compiler.get_id() == 'msvc'
  c_args += [
    '-D_CRT_SECURE_NO_WARNINGS',
  ]

  if get_option('default_library') == 'shared'
    c_args += [
      '-DBUILDING_WINGETOPT_DLL',
      '-DWINGETOPT_SHARED_LIB',
    ]
  endif
endif

wingetopt_lib = library(
  'wingetopt',
  'src/getopt.c',
  include_directories: include_directories(
    'src',
  ),
  c_args: c_args,
)

wingetopt_dep = declare_dependency(
  link_with: wingetopt_lib,
  include_directories: include_directories(
    'src',
  ),
)
