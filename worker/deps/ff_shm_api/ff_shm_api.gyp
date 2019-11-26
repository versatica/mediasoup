{
  'targets':
  [
    {
      'target_name': 'libngxshm',
      'type': 'static_library',
#      'dependencies':
#      [
#        'libngxshm'
#      ],
      'sources':
      [
        'include/ffngxshm.h',
        'include/ffngxshm_av_media.h',
        'include/ffngxshm_log.h',
        'include/ffngxshm_log_header.h',
        'include/ffngxshm_raw_media.h',
        'include/ffngxshm_writer.h',
        'include/sfushm_av_media.h',
        'src/ffngxshm.c',
        'src/ffngxshm_av_media.c',
        'src/ffngxshm_raw_media.c',
        'src/ffngxshm_log.c',
        'src/sfushm_av_media.c'
      ],
      'include_dirs':
      [
        './include',
        '/usr/local/include',
        '/usr/local/include/nginx/core',
        '/usr/local/include/nginx/nginx-shm-module',
        '/usr/local/include/nginx/objs',
        '/usr/local/include/nginx/os/unix'
      ],
      'library_dirs': [
        '.',
        '/usr/local/lib/',
      ],
      'link_settings': {
        'libraries': [
          #'/usr/local/lib/libngxshm.a',
          '/usr/local/lib/libngxshm.a',
          #'-lngxshm',
          '-pthread',
          '-lrt',
          '-lm',
          '-ldl',
          '-lpcre',
          '-lcrypto',
          '-lrt',
          '-lbz2',
          '-lz'
        ],
      },
#      'direct_dependent_settings': {
#        'include_dirs': 
#        [
#          './include'
#        '/usr/local/include',
#        '/usr/local/include/nginx/core',
#        '/usr/local/include/nginx/nginx-shm-module',
#        '/usr/local/include/nginx/objs',
#        '/usr/local/include/nginx/os/unix'
#        ]
#      },
      'conditions':
      [
        [ 'OS != "win"', {
          'cflags': [ '-Wall' ]
        }],
        [ 'OS == "mac"', {
          'xcode_settings':
          {
            'WARNING_CFLAGS': [ '-Wall' ]
          }
        }]
      ]
 #   },
 #   {
 #     'target_name': 'libngxshm',
 #     'type': '<(library)',
 #     'sources': [
 #     ],
    }
  ]
}
