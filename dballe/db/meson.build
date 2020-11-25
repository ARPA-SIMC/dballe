libdballe_sources += files(
        'db.cc',
        'summary.cc',
        'summary_utils.cc',
        'summary_memory.cc',
)

foreach f: [['summary-access.in.cc', 'summary-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach

subdir('v7')
