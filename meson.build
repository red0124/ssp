project('ssp', 'cpp', 
  default_options : 
  ['warning_level=3', 
    'cpp_std=c++17', 
    'buildtype=debugoptimized'])

fast_float_sub = subproject('fast_float')
fast_float_dep = fast_float_sub.get_variable('fast_float_dep')

ssp_dep = declare_dependency(
  include_directories: include_directories('include'),
  dependencies: fast_float_dep
  )

subdir('test')
