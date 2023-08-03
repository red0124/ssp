doctest_dep = dependency('doctest')
add_project_arguments('-DDOCTEST_CONFIG_IMPLEMENT_WITH_MAIN', language: 'cpp')

tests = [
  'parser',
  'splitter',
  'converter',
  'extractions',
  'parser2_segment1',
  'parser2_segment2',
  'parser2_segment3',
  'parser2_segment4',
  'extractions_without_fast_float',
]

foreach name : tests

  exe = executable(
    name,
    'test_' + name + '.cpp',
    dependencies: [doctest_dep, ssp_dep]
    )

  test('test_' + name, exe, timeout: 60)
endforeach

