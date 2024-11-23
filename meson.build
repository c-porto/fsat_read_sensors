project('read-sensors', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'cpp_std=c++20',
    'optimization=3',
    'buildtype=debug',
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
    'src/sensor-manager.hpp', 
    'src/sensor-manager.cpp',
    'src/log.hpp',
    'src/log.cpp',
    'src/sensor.hpp',
    'src/sensor.cpp',
    'src/daemon.hpp',
    'src/daemon.cpp',
    'src/main.cpp',
    'src/db.hpp',
    'src/db.cpp',
  )

sqlite3_dep = dependency('sqlite3')

exe = executable('read_sensors', sources, dependencies: sqlite3_dep)
