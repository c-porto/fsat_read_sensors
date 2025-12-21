project(
  'read-sensors',
  'cpp',
  version: '0.4.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
    'optimization=3',
    'buildtype=release',
  ],
)

rs_deps = []
rs_srcs = []

rs_inc = include_directories('include')

sqlite3_dep = dependency('sqlite3')

fsatutils = dependency('fsatutils')

rs_deps += sqlite3_dep
rs_deps += fsatutils

subdir('src')

c_args = [
  '-Wno-unused-parameter',
  '-Wno-unused-value',
  '-Wno-missing-field-initializers',
  '-Wno-narrowing',
  '-Wno-pointer-arith',
  '-Wshadow',
  '-Wcast-align',
  '-Wwrite-strings',
]

py = find_program('python3')

version_header_path = meson.current_build_dir() / 'version.hpp'

version_hpp = custom_target(
  'version_hpp',
  output: 'version.hpp',
  input: 'scripts/gen_versioning.py',
  command: [
    py,
    '@INPUT@',
    version_header_path,
    meson.project_version(),
  ],
  build_by_default: true,
)

read_sensors = executable(
  meson.project_name(),
  sources: [rs_srcs, version_hpp],
  include_directories: rs_inc,
  dependencies: rs_deps,
  c_args: c_args,
  install: true,
)

install_data(
  'services/read-sensors.service',
  install_dir: get_option('systemd_unitdir'),
  rename: 'read-sensors.service',
)
