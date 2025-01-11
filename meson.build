project(
  'read-sensors',
  'cpp',
  version: '0.2.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
    'optimization=3',
    'buildtype=release',
  ],
)

add_project_arguments(
  [
    '-Wno-unused-parameter',
    '-Wno-unused-value',
    '-Wno-missing-field-initializers',
    '-Wno-narrowing',
    '-Wno-pointer-arith',
  ],
  language: 'cpp',
)

sources = files(
  'src/daemon.cpp',
  'src/daemon.hpp',
  'src/db.cpp',
  'src/db.hpp',
  'src/log.cpp',
  'src/log.hpp',
  'src/main.cpp',
  'src/sensor-manager.cpp',
  'src/sensor-manager.hpp',
  'src/sensor.cpp',
  'src/sensor.hpp',
)

rs_dependencies = []

install_src = get_option('install_src')

sqlite3_dep = dependency('sqlite3')

libsystemd = dependency('libsystemd')

rs_dependencies += sqlite3_dep
rs_dependencies += libsystemd

binary = executable('read_sensors', sources, dependencies: rs_dependencies, install: true)

install_data(
  'services/read-sensors.service',
  install_dir: get_option('systemd_unitdir'),
  rename: 'read-sensors.service',
)

install_data(
  'services/read-sensors.socket',
  install_dir: get_option('systemd_unitdir'),
  rename: 'read-sensors.socket',
)

if install_src
  install_data(sources, install_dir: get_option('src_install_dir'))
endif
