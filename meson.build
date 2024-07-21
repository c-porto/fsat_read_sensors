project('read-sensors', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17'])

sources = files('read-sensors.cpp', 'read-sensors.hpp', 'log.hpp', 'log.cpp', 'sensor.hpp', 'sensor.cpp')

exe = executable('read_sensors', sources,
  install : true)

test('basic', exe)
