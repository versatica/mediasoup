{
  'make_global_settings':
  [
    ['CC.target', '<(openssl_fips)/bin/fipsld'],
    ['LINK', '<(openssl_fips)/bin/fipsld']
  ]
}
