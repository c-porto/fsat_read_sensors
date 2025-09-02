project(
  'read-sensors',
  'cpp',
  version: '0.3.0',
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
  'src/parser.hpp',
  'src/sensor-manager.cpp',
  'src/sensor-manager.hpp',
  'src/sensor.hpp',
  'src/ina219.hpp',
  'src/ina219.cpp',
  'src/tmp112.hpp',
  'src/tmp112.cpp',
)

rs_dependencies = []

install_src = get_option('install_src')

sqlite3_dep = dependency('sqlite3')

libsystemd = dependency('libsystemd')

libiio = dependency('libiio')

nlohmann_json = dependency('nlohmann_json')

rs_dependencies += sqlite3_dep
rs_dependencies += libsystemd
rs_dependencies += libiio
rs_dependencies += nlohmann_json

read_sensors = executable('read_sensors', sources, dependencies: rs_dependencies, install: true)

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
