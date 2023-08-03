test_sources = files([
      'test_main.cpp',
      'test_splitter.cpp',
      'test_converter.cpp',
      'test_parser.cpp',
      'test_parser2_segment1.cpp',
      'test_parser2_segment2.cpp',
      'test_parser2_segment3.cpp',
      'test_parser2_segment4.cpp',
      'test_extractions.cpp',
      'test_extractions_without_fast_float.cpp',
      ])

doctest_dep = dependency('doctest')

test_exe = executable(
  'test_ssp',
  sources: test_sources,
  dependencies: [doctest_dep, ssp_dep],
  cpp_args: '-lstdc++fs'
  )

test('test_ssp', test_exe)
