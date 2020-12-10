libdballef = both_libraries('dballef',
    'error.cc', 'binding.cc', 'dballef.f90',
    version: libdballef_so_version,
    install: true,
    include_directories: toplevel_inc,
    link_with: [libdballe],
    dependencies: [
        libwreport_dep,
    ],
)

install_headers(
    'dballef.h',
    'dballeff.h',
    subdir: 'dballe',
)

fc = meson.get_compiler('fortran')

dballef_mod = custom_target('dballef.mod',
    input: 'dballef.f90', output: 'dballef.mod', command: fc.cmd_array() + ['-c', '@INPUT@', '-J', '@OUTDIR@'],
    install: true,
    install_dir: get_option('includedir') / 'dballe',
)


#
# Unit testing
#

test_sources = [
    'check_real0',
    'check_range',
    'check_fdballe',
    'check_fdballe_oldapi',
    'check_attrs',
    'check_set',
    'check_missing',
    'check_missing_msg',
    'check_segfault1',
    'check_multiplehandler',
    'check_spiegab',
    'check_messages',
    'check_messages_json',
    'check_transactions1',
    'check_connect_wipe',
]

common_test_sources = ['test.f90', 'dbtest.f90']

runtest = find_program('../extra/runtest')

foreach s: test_sources
    e = executable(s, [s + '.f90'] + common_test_sources, link_with: [libdballef])
    test(s, runtest, args: [e])
endforeach
