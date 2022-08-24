shortcuts_h = custom_target(
	'shortcuts.h', output: 'shortcuts.h', input: '../vars.csv',
	command: [mkvars, '-t', 'shortcuts.h', '@INPUT@', '-o', '@OUTPUT@'],
	install_dir: get_option('includedir') + '/' + 'dballe/core',
	build_by_default: true)
shortcuts_cc = custom_target(
        'shortcuts.cc', output: 'shortcuts.cc', input: '../vars.csv',
	command: [mkvars, '-t', 'shortcuts.cc', '@INPUT@', '-o', '@OUTPUT@'],
	build_by_default: true)
shortcuts_access_in_cc = custom_target(
        'shortcuts-access.in.cc', output: 'shortcuts-access.in.cc', input: '../vars.csv',
	command: [mkvars, '-t', 'shortcuts-access.in.cc', '@INPUT@', '-o', '@OUTPUT@'],
	build_by_default: true)

libdballe_sources += files(
        'error.cc',
        'cursor.cc',
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

install_headers(
	'fwd.h',
	'error.h',
	'enq.h',
	'cursor.h',
	'aliases.h',
	'benchmark.h',
	'defs.h',
	'var.h',
	'values.h',
	'file.h',
	'arrayfile.h',
	'csv.h',
	'data.h',
	'query.h',
	'structbuf.h',
	'matcher.h',
	'match-wreport.h',
	'smallset.h',
	'varmatch.h',
	'string.h',
	'trace.h',
	'json.h',
        subdir: 'dballe/core',
)

foreach f: [[shortcuts_access_in_cc, 'shortcuts-access.cc'],
                ['data-access.in.cc', 'data-access.cc'],
                ['query-access.in.cc', 'query-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach

libdballe_sources += [
        shortcuts_cc,
]

libdballe_sources += [
    custom_target('aliases.cc', output: 'aliases.cc', input: 'aliases.gperf', command: [gperf, '@INPUT@', '--output-file=@OUTPUT@'])
]
