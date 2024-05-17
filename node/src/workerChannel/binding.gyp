{
  'variables': {
    'mediasoup_build_type%': 'Release',
    'mediasoup_worker_lib%': ''
  },
  "targets": [
    {
      'target_name': 'worker-channel',
      'sources': [
        'src/binding.cpp',
        'src/workerChannel.cpp'
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
            '<(module_root_dir)/../../../worker/out/<(mediasoup_build_type)/build/libmediasoup-worker.a'
          ],
        }, {
          "libraries": [
            '<(mediasoup_worker_lib)'
            ],
          }
        ],
        ['OS=="win"', {
          'libraries': [
            'Ws2_32.lib', 'Dbghelp.lib', 'Crypt32.lib', 'Userenv.lib',
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1,
            },
            'VCLinkerTool': {
              'AdditionalOptions': ['/FORCE:MULTIPLE'],
            },
          }
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            "CLANG_CXX_LIBRARY": 'libc++',
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            # TODO: This should be the same as the one used for libmediasoup
            # Is it really needed?
            'MACOSX_DEPLOYMENT_TARGET': '14'
          }
        }]
      ]
    }
  ]
}
