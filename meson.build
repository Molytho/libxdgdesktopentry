project(
    'xdgdesktopentry++',
    'cpp',
    version: '0.1',
    meson_version: '>= 1.3.0',
    default_options: ['warning_level=3', 'werror=true', 'cpp_std=c++20'],
)

dependencies = [
    dependency('boost', modules: ['regex', 'lexical_cast']),
]

# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['-DBUILDING_LIBXDGDESKTOPENTRY']

sources = [
    'src/desktop-entry.cpp',
]

includes = [include_directories('include')]

lib = library(
    'xdgdesktopentry',
    sources,
    include_directories: includes,
    install: true,
    dependencies: dependencies,
    cpp_shared_args: lib_args,
    gnu_symbol_visibility: 'hidden',
)

test_exe = executable(
    'xdgdesktopentry_test',
    'src/libxdgdesktopentry_test.cpp',
    include_directories: includes,
    link_with: lib,
)
test('xdgdesktopentry', test_exe)

# Make this library usable as a Meson subproject.
xdgdesktopentry_dep = declare_dependency(
    include_directories: include_directories('.'),
    dependencies: dependencies,
    link_with: lib,
)
meson.override_dependency('xdgdesktopentry++', xdgdesktopentry_dep)

# Make this library usable from the system's
# package manager.
install_headers('include/desktop-entry.h', subdir: 'xdgdesktopentry')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
    lib,
    description: 'Meson sample project.',
    subdirs: 'xdgdesktopentry',
)
