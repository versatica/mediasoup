{
  'targets':
  [
    {
      'target_name': 'jsoncpp',
      'type': 'static_library',
      'sources':
      [
        'jsoncpp/bundled/jsoncpp.cpp'
      ],
      'direct_dependent_settings':
      {
        'include_dirs':
        [
          'jsoncpp/bundled'
        ]
      },
      'conditions':
      [
        [ 'OS != "win"', {
          'cflags': [ '-std=c++11', '-Wall', '-Wextra', '-Wno-unused-parameter' ]
        }],
        [ 'OS == "mac"', {
          'xcode_settings':
          {
            'OTHER_CPLUSPLUSFLAGS' : [ '-std=c++11' ],
            'WARNING_CFLAGS': [ '-Wall', '-Wextra', '-Wno-unused-parameter' ],
          }
        }]
      ]
    }
  ]
}
