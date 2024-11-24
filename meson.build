project('read-sensors', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'cpp_std=c++20',
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

install_src = get_option('install_src')

sqlite3_dep = dependency('sqlite3')

binary = executable('read_sensors', sources, dependencies: sqlite3_dep, install: true)

if install_src
  install_subdir('src', install_dir: get_option('src_install_dir'))
endif

install_data('services/read-sensors.service',
             install_dir: get_option('systemd_unitdir'),
             rename: 'read-sensors.service')

install_data('services/read-sensors.socket',
             install_dir: get_option('systemd_unitdir'),
             rename: 'read-sensors.socket')
