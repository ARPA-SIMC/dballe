sphinx_doc_sources += files(
    'index.rst',
    'concepts.rst',
    'parms.rst',
    'parms/input.rst',
    'parms/output.rst',
    'parms/query.rst',
    'parms/attrs.rst',
    'parms/filters.rst',
    'env.rst',
    'connect.rst',
    'json.rst',
)

sphinx_doc_sources += custom_target('btable_rst',
    input: '../../tables/dballe.txt',
    output: 'btable.rst',
    command: [mksnippet, 'btable', 'rst', '@INPUT@'],
    capture: true,
)

sphinx_doc_sources += custom_target('ltypes_rst',
    input: '../../dballe/msg/ltypes.txt',
    output: 'ltypes.rst',
    command: [mksnippet, 'levels', 'rst', '@INPUT@'],
    capture: true,
)

sphinx_doc_sources += custom_target('tranges_rst',
    output: 'tranges.rst',
    command: [mksnippet, 'tranges', 'rst'],
    capture: true,
)

sphinx_doc_sources += custom_target('aliases_rst',
    input: '../../dballe/core/aliases.gperf',
    output: 'aliases.rst',
    command: [mksnippet, 'alias', 'rst', '@INPUT@'],
    capture: true,
)

sphinx_doc_sources += custom_target('msg_vars_rst',
    input: '../../dballe/vars.csv',
    output: 'msg_vars.rst',
    command: [mksnippet, 'msg_vars', 'rst', '@INPUT@'],
    capture: true,
)
