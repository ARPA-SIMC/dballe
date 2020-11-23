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

libdballe_sources += [
        shortcuts_cc,
        mklookup.process(shortcuts_access_in_cc),
        mklookup.process('data-access.in.cc'),
        mklookup.process('query-access.in.cc'),
]
