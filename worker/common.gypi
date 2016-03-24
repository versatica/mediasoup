{
  'variables':
  {
  	# libuv variables:
    # - Static libuv, but allow override to 'shared_library' for DLL/.so builds
    'uv_library%': 'static_library',
    # openssl variables:
    'library%': 'static_library',
    # Others:
    'gcc_version%': 'unknown',
    'clang%': 1,
    'openssl_fips%': 'false',
    'mediasoup_asan%': 'false'
  },

  'target_defaults':
  {
    'default_configuration': 'Release',

    'configurations':
    {
      'Debug':
      {
        'defines': [ ' MS_DEVEL', 'DEBUG' ],
        'cflags': [ '-g', '-O0', '-fwrapv', '-Wno-parentheses-equality' ],
        'xcode_settings':
        {
          'GCC_OPTIMIZATION_LEVEL': '0'
        }
      },
      'Release':
      {
      	'cflags': [ '-g' ]
      }
    },

    'xcode_settings':
    {
      'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
      'GCC_WARN_ABOUT_MISSING_NEWLINE': 'YES',  # -Wnewline-eof
      'PREBINDING': 'NO',                       # No -Wl,-prebind
      'OTHER_CFLAGS':
      [
        '-fstrict-aliasing',
        '-g',
      ],
      'WARNING_CFLAGS':
      [
        '-Wall',
        '-Wendif-labels',
        '-W',
        '-Wno-unused-parameter',
        '-Wundeclared-selector',
        '-Wno-parentheses-equality',
      ],
    },
    'conditions':
    [
      [ 'target_arch == "ia32"', {
        'xcode_settings': { 'ARCHS': ['i386'] }
      }],
      ['target_arch == "x64"', {
        'xcode_settings': { 'ARCHS': ['x86_64'] }
      }],
      [ 'OS in "linux freebsd openbsd solaris"', {
        'target_conditions':
        [
          [ '_type == "static_library"', {
            'standalone_static_library': 1, # disable thin archive which needs binutils >= 2.19
          }]
        ],
        'conditions':
        [
          [ 'target_arch == "ia32"', {
            'cflags': [ '-m32' ],
            'ldflags': [ '-m32' ]
          }],
          [ 'target_arch == "x64"', {
            'cflags': [ '-m64' ],
            'ldflags': [ '-m64' ]
          }],
        ],
      }]
    ]
  }
}
