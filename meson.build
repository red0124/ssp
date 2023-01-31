project('ssp', 'cpp', 
  default_options : 
  ['warning_level=3', 
    'cpp_std=c++17', 
    'buildtype=debugoptimized'],
  meson_version:'>=0.54.0')

fast_float_dep = dependency('fast_float')

ssp_dep = declare_dependency(
  include_directories: include_directories('include'),
  dependencies: fast_float_dep
  )

meson.override_dependency('ssp', ssp_dep)

if not meson.is_subproject()
  subdir('test')
endif

