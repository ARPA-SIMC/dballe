project('dballe', 'cpp', 'fortran', version: '9.3', license : 'GPL-2.0-or-later', default_options: ['warning_level=3', 'cpp_std=c++11'])

add_global_arguments('-Wextra', '-Wno-unused-parameter', language : 'cpp')

libdballe_so_version = '9.3.0'
libdballef_so_version = '5.0.0'

table_dir = get_option('datadir') / 'wreport'

conf_data = configuration_data()
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set_quoted('TABLE_DIR', get_option('prefix') / table_dir)

toplevel_inc = include_directories('.')

# Dependencies
libwreport_dep = dependency('libwreport', version: '>= 3.22')
sqlite3_dep = dependency('sqlite3')
libpq_dep = dependency('libpq', required: false)
conf_data.set('HAVE_LIBPQ', libpq_dep.found())
mariadb_dep = dependency('mariadb', required: false)
conf_data.set('HAVE_MYSQL', mariadb_dep.found())
xapian_dep = dependency('xapian-core', version: '>= 1.4', required: false)
conf_data.set('HAVE_XAPIAN', xapian_dep.found())
popt_dep = dependency('popt')
gperf = find_program('gperf')

pymod = import('python')
python3 = pymod.find_installation('python3', required: false)

#if test x$enable_dballe_python = xyes
#then
#    PYTHON_CFLAGS=`unset CFLAGS; $PYTHON-config --cflags`
#    AC_SUBST(PYTHON_CFLAGS)
#    AX_PYTHON_MODULE_VERSION(wreport, 3.22)
#    AX_PYTHON_MODULE(numpy, yes)
#fi

if python3.found()
    # FIXME: python3.path() is only available from meson 0.50: this is a workaround
    python3_path = python3.get_variable('BINDIR') / python3.get_variable('PYTHON') + python3.language_version()

    sphinx = find_program('sphinx-build', 'sphinx-build-3', 'sphinx-build-' + python3.language_version(), required: false)

    # TODO: check if breathe is installed
    # TODO AX_PYTHON_MODULE(breathe)
    # See https://gitlab.freedesktop.org/wayland/weston/-/blob/master/doc/sphinx/meson.build

    doxygen = find_program('doxygen', required : false)

    build_docs = sphinx.found() and doxygen.found()
    docdir = get_option('datadir') / 'doc' / meson.project_name()
else
    warning('Documentation disabled, requires doxygen, sphinx, and the breathe python module')
    build_docs = false
endif

compiler = meson.get_compiler('cpp')
if compiler.has_function('strcasestr', prefix : '#include <string.h>')
    # This does not currently work.
    # See https://github.com/mesonbuild/meson/issues/8075
    conf_data.set('HAVE_STRCASESTR', true)
endif
if not compiler.has_function('bswap_32', prefix : '#include <byteswap.h>')
    conf_data.set('USE_OWN_BSWAP', true)
endif
if not compiler.has_function('vasprintf', prefix : '#include <cstdio>')
    conf_data.set('USE_OWN_VASPRINTF', true)
endif

# Generate config.h
configure_file(output: 'config.h', configuration: conf_data)

# Generate the builddir's version of run-local
run_local_cfg = configure_file(output: 'run-local', input: 'run-local.in', configuration: {
    'top_srcdir': meson.source_root(),
    'top_builddir': meson.build_root(),
})
# Just using the configure_file object in a custom_target command gives:
#   'AttributeError: 'File' object has no attribute 'replace'
# Using find_program on the resulting file works around that
# See https://github.com/mesonbuild/meson/issues/8039
run_local = find_program(run_local_cfg)

subdir('dballe')
subdir('fortran')
subdir('src')

if python3.found()
    subdir('python')
endif

if build_docs
    subdir('doc')
endif

subdir('tables')


# Generate pkg-config metadata
pkg = import('pkgconfig')
pkg.generate(libdballe,
    description: 'Database for point-based meteorological data (C++ library)',
    filebase: 'libdballe',
    requires: [libwreport_dep],
)

pkg.generate(libdballef,
    description: 'Database for point-based meteorological data (Fortran library)',
    filebase: 'libdballef',
    requires: [libwreport_dep],
)


# Install old m4 macro files

m4_dir = get_option('datadir') / 'aclocal'
install_data('libdballe.m4', 'libdballef.m4', install_dir: m4_dir)


#    AC_MSG_CHECKING([for wreport documentation dir $WREPORT_DOXYGEN_DIR])
#    AC_MSG_RESULT([$wreport_doxygen_dir_result])
#    AC_MSG_CHECKING([for wreport doxytag file $WREPORT_DOXYGEN_TAGFILE])
#    AC_MSG_RESULT([$wreport_doxygen_tagfile_result])
#    if test x$wreport_doxygen_dir_result = xno
#    then
#        AC_MSG_WARN([WREPORT_DOXYGEN_DIR not found, documentation will not be built.])
#        enable_docs=no
#    fi
#    if test x$wreport_doxygen_tagfile_result = xno
#    then
#        AC_MSG_WARN([WREPORT_DOXYGEN_TAGFILE not found, documentation will not be built.])
#        enable_docs=no
#    fi
#
#    AX_PYTHON_MODULE(breathe)
#    if test x$HAVE_PYMOD_BREATHE != xyes
#    then
#        enable_docs=no
#    fi

#AC_PROG_FC([gfortran pgf90 ifort g95 xlf90 f90])
#
#dnl Enable extra compiler warnings
#AX_CHECK_COMPILE_FLAG([-Wredundant-move], [has_redundant_move=yes], [has_redundant_move=no])
#if test $has_redundant_move = yes
#then
#       AX_APPEND_FLAG([-Wno-error=redundant-move])
#fi
