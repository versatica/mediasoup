{
  'variables':
  {
  	# libuv variables:
    'uv_library%': 'static_library',
    # openssl variables:
    'library%': 'static_library',
    # Others:
    'gcc_version%': 'unknown',
    'clang%': 1,
    'mediasoup_asan%': 'false',
    'openssl_fips%': 'false',
    'libopenssl': '<(PRODUCT_DIR)/libopenssl.a'
  },

  'target_defaults':
  {
    'default_configuration': 'Release',

    'configurations':
    {
      'Release':
      {
      	'cflags': [ '-g' ]
      },
      'Debug':
      {
        'defines': [ 'DEBUG', 'MS_LOG_TRACE', 'MS_LOG_FILE_LINE' ],
        'cflags': [ '-g', '-O0', '-fwrapv', '-Wno-parentheses-equality' ],
        'xcode_settings':
        {
          'GCC_OPTIMIZATION_LEVEL': '0'
        }
      },
      'Fuzzer':
      {
        'defines': [ 'DEBUG' ],
        'cflags': [ '-g', '-O0', '-fsanitize=address,fuzzer' ]
      }
    },

    'xcode_settings':
    {
      'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
      'GCC_WARN_ABOUT_MISSING_NEWLINE': 'YES', # -Wnewline-eof
      'PREBINDING': 'NO',                      # No -Wl,-prebind
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
        'xcode_settings': { 'ARCHS': [ 'i386' ] }
      }],
      [ 'target_arch == "x64"', {
        'xcode_settings': { 'ARCHS': [ 'x86_64' ] }
      }],
      [ 'OS in "linux freebsd openbsd solaris"', {
        'target_conditions':
        [
          [ '_type == "static_library"', {
            'standalone_static_library': 1, # Disable thin archive which needs binutils >= 2.19
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
