libdballe_sources += files(
        'api.cc',
        'traced.cc',
        'commonapi.cc',
        'msgapi.cc',
        'dbapi.cc',
)

install_headers(
    'api.h',
    'enq.h',
    'dbapi.h',
    'traced.h',
    'commonapi.h',
    'msgapi.h',
    subdir: 'dballe/fortran',
)

foreach f: [['commonapi-access.in.cc', 'commonapi-access.cc'],]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach
