{
  'variables': {
    'mediasoup_build_type%': 'Release',
    'mediasoup_worker_lib%': '',
    'meson_args%': ''
  },
  "targets": [
    {
      'target_name': 'worker-channel',
      'sources': [
        'src/binding.cpp',
        'src/WorkerChannel.cpp'
      ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'include_dirs': [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/../../../worker/include",
      ],
      'conditions': [
        ['mediasoup_worker_lib==""', {
          'libraries': [
            '<(module_root_dir)/../../../worker/out/<(mediasoup_build_type)/libmediasoup-worker.a'
          ],
        }, {
          'libraries': [
            '<(mediasoup_worker_lib)'
            ],
          }
        ],
        ['OS=="win"', {
          'libraries': [
            'Ws2_32.lib', 'Dbghelp.lib', 'Crypt32.lib', 'Userenv.lib',
          ],
          'conditions': [
            ['mediasoup_build_type=="Release"', {
              'variables': {
                'runtime_library': '2',
              },
            }, {
              'variables': {
                'runtime_library': '3',
              }
            }]
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1,
              # RuntimeLibrary:
              # 0 - MultiThreaded (/MT)
              # 1 - MultiThreadedDebug (/MTd)
              # 2 - MultiThreadedDLL (/MD)
              # 3 - MultiThreadedDebugDLL (/MDd)
              'RuntimeLibrary': '<(runtime_library)',
            },
            'VCLinkerTool': {
              'AdditionalOptions': ['/FORCE:MULTIPLE'],
            }
          }
        }],
        ['OS=="mac"', {
          'cflags!': [ '-fno-exceptions' ],
          'cflags_cc!': [ '-fno-exceptions' ],
          'xcode_settings': {
            "CLANG_CXX_LIBRARY": 'libc++',
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            # NOTE: This should be the same as the one used for libmediasoup
            'MACOSX_DEPLOYMENT_TARGET': '15'
          }
        }],
        ['OS=="linux"', {
          'cflags!': [ '-fno-exceptions' ],
          'cflags_cc!': [ '-fno-exceptions' ],
        }]
      ]
    }
  ]
}
