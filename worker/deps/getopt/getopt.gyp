{
  'targets': 
  [
    {
      'target_name': 'getopt',
      'type': 'static_library',
      'sources':
      [
        'getopt/src/getopt.h',
        'getopt/src/getopt.c'
      ],
      'include_dirs': 
      [
        'getopt/src/'
      ],
      'direct_dependent_settings':
      {
        'include_dirs': ['getopt/src/']
      },
    }
  ]
}
