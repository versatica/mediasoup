{
  'targets':
  [
    {
      'target_name': 'netstring',
      'type': 'static_library',
      'sources':
      [
        'netstring-c/netstring.c',
        'netstring-c/netstring.h'
      ],
      'direct_dependent_settings':
      {
        'include_dirs':
        [
          'netstring-c'
        ]
      },
      'conditions':
      [
        [ 'OS != "win"', {
          'cflags': [ '-Wall', '-Wno-sign-compare' ]
        }],
        [ 'OS == "mac"', {
          'xcode_settings':
          {
            'WARNING_CFLAGS': [ '-Wall', '-Wno-sign-compare' ]
          }
        }]
      ]
    }
  ]
}
