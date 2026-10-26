# The generator writes a single header per schema, with the same base name and
# a .h extension since the empty --filename-suffix removes the default one.
flatbuffers_headers = []

foreach schema : flatbuffers_schemas
  flatbuffers_headers += schema.replace('.fbs', '.h')
endforeach

flatc = find_program('flatc')

flatbuffers_generator = custom_target('flatbuffers-generator',
  output: flatbuffers_headers,
  input: flatbuffers_schema_files,
  command : [
    flatc,
    '--cpp',
    '--cpp-field-case-style', 'lower',
    '--reflect-names',
    '--scoped-enums',
    '--filename-suffix', '',
    '-o', '@OUTDIR@',
    '@INPUT@'
  ],
  build_by_default: true,
)
