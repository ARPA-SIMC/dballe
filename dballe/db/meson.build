libdballe_sources += files(
        'db.cc',
        'summary.cc',
        'summary_utils.cc',
        'summary_memory.cc',
)

install_headers(
    'fwd.h',
    'defs.h',
    'db.h',
    'summary.h',
    'summary_utils.h',
    'summary_memory.h',
    'explorer.h',
    subdir: 'dballe/db',
)
if xapian_dep.found()
    install_headers(
        'summary_xapian.h',
        subdir: 'dballe/db',
    )
endif

foreach f: [['summary-access.in.cc', 'summary-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach

subdir('v7')
