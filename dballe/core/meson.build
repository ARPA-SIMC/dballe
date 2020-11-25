shortcuts_h = custom_target('shortcuts.h', output: 'shortcuts.h', input: '../vars.csv', command: [mkvars, '-t', 'shortcuts.h', '-o', '@OUTPUT@'])
shortcuts_cc = custom_target('shortcuts.cc', output: 'shortcuts.cc', input: '../vars.csv', command: [mkvars, '-t', 'shortcuts.cc', '-o', '@OUTPUT@'])
shortcuts_access_in_cc = custom_target('shortcuts-access.in.cc', output: 'shortcuts-access.in.cc', input: '../vars.csv', command: [mkvars, '-t', 'shortcuts-access.in.cc', '-o', '@OUTPUT@'])

libdballe_sources += files(
        'error.cc',
        'cursor.cc',
        'aliases.cc',
        'benchmark.cc',
        'defs.cc',
        'var.cc',
        'values.cc',
        'file.cc',
        'arrayfile.cc',
        'csv.cc',
        'data.cc',
        'query.cc',
        'structbuf.cc',
        'matcher.cc',
        'match-wreport.cc',
        'varmatch.cc',
        'json.cc',
        'string.cc',
)

foreach f: [[shortcuts_access_in_cc, 'shortcuts-access.cc'],
                ['data-access.in.cc', 'data-access.cc'],
                ['query-access.in.cc', 'query-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach

libdballe_sources += [
        shortcuts_cc,
]
