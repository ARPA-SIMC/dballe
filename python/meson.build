toplevel_inc = include_directories('..')

py_dballe_sources = [
    'utils/core.cc',
    'utils/methods.cc',
    'utils/values.cc',
    'utils/wreport.cc',
    'common.cc',
    'types.cc',
    'data.cc',
    'binarymessage.cc',
    'file.cc',
    'message.cc',
    'importer.cc',
    'exporter.cc',
    'dballe.cc',
    'db.cc',
    'cursor.cc',
    'explorer.cc',
]

foreach f: [
               ['data-access.in.tcc', 'data-access.tcc'],
               ['data-access.in.cc', 'data-access.cc'],
               ['query-access.in.cc', 'query-access.cc']
           ]
    py_dballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach

py_dballe = python3.extension_module('_dballe',
    py_dballe_sources,
    include_directories: toplevel_inc,
    dependencies: python3.dependency(),
    link_with: libdballe,
    install: true,
    # Meson does not currently autodetect the right target for python modules:
    # https://github.com/mesonbuild/meson/issues/6331
    install_dir: python3.get_path('platlib'),
)

python3.install_sources(
    'dballe/dbacsv.py',
    'dballe/__init__.py',
    'dballe/volnd.py',
    subdir: 'dballe',
)

python_tests = [
    'test-version.py',
    'test-core.py',
    'testlibmsg.py',
    'test-types.py',
    'test-binarymessage.py',
    'test-file.py',
    'test-message.py',
    'test-importer.py',
    'test-exporter.py',
    'test-db.py',
    'test-explorer.py',
    'test-volnd.py',
    'test-csv.py',
]

runtest = find_program('../extra/runtest')

foreach t: python_tests
    test(t, runtest, args: ['-v', files(t)], env: [
        'PYTHON=' + python3_path,
        'PYTHONPATH=' + meson.current_build_dir(),
    ])
endforeach
