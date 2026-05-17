project(
  'read-sensors',
  'cpp',
  version: '2.0',
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

version_hpp = vcs_tag(
  input: 'scripts/version.hpp.in',
  output: 'version.hpp',
  command: [
    py,
    meson.project_source_root() / 'scripts/gen_versioning.py',
    meson.project_version(),
  ],
  fallback: meson.project_version(),
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
