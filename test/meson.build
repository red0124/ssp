test_sources = files([
      'test_main.cpp',
      #'test_splitter.cpp',
      #'test_converter.cpp',
      'test_parser.cpp',
      #'test_extractions.cpp',
      ])

doctest_proj = subproject('doctest')
doctest_dep = doctest_proj.get_variable('doctest_dep')

test_exe = executable(
  'test_ssp',
  sources: test_sources,
  dependencies: [doctest_dep, ssp_dep],
  cpp_args: '-lstdc++fs'
  )

test('test_ssp', test_exe)
