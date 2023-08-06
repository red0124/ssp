doctest_dep = dependency('doctest')
add_project_arguments('-DDOCTEST_CONFIG_IMPLEMENT_WITH_MAIN', language: 'cpp')

tests = [
  'parser',
  'splitter',
  'converter',
  'extractions',
  'parser2_1',
  'parser2_2',
  'parser2_3',
  'parser2_4',
  'extractions_without_fast_float',
]

foreach name : tests
  test_name = 'test_' + name

  exe = executable(
    test_name,
    test_name + '.cpp',
    dependencies: [doctest_dep, ssp_dep]
    )

  test(test_name, exe, timeout: 60)
endforeach

