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
    cpp_pch: '../dballe/pch/dballe_pch.h',
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
    'test_version.py',
    'test_core.py',
    'testlibmsg.py',
    'test_types.py',
    'test_binarymessage.py',
    'test_file.py',
    'test_message.py',
    'test_importer.py',
    'test_exporter.py',
    'test_db.py',
    'test_explorer.py',
    'test_volnd.py',
    'test_csv.py',
]

runtest = find_program('../extra/runtest')

test('python', runtest, args: ['PYTHON'],
     env: [
         'PYTHON=' + python3_path,
         'PYTHONPATH=' + meson.current_source_dir() + ':' + meson.current_build_dir(),
     ],
     depends: [py_dballe] + tables
)
