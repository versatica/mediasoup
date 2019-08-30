{
  'targets': 
  [
    {
      'target_name': 'getopt',
      'type': 'static_library',
      'sources':
      [
        'getopt.h',
        'getopt.cpp'
      ],
      'direct_dependent_settings':
      {
        'include_dirs': ['.']
      },
    }
  ]
}