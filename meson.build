project('read-sensors', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'optimization=3',
    'buildtype=release',
  ])

  add_project_arguments(
  [
    '-Wno-unused-parameter',
    '-Wno-unused-value',
    '-Wno-missing-field-initializers',
    '-Wno-narrowing',
    '-Wno-pointer-arith',
  ],
  language: 'cpp')

sources = files(
    'src/read-sensors.hpp', 
    'src/read-sensors.cpp',
    'src/log.hpp',
    'src/log.cpp',
    'src/sensor.hpp',
    'src/sensor.cpp'
  )

exe = executable('read_sensors', sources, install : false)

