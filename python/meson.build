python_sources = [
    'dballe/dbacsv.py',
    'dballe/__init__.py',
    'dballe/volnd.py',
]

python_sources_stamp = custom_target('python_stamp',
    input: python_sources,
    output: ['python.stamp'],
    command: ['touch', '@OUTPUT@'],
    install: false,
)

python3.install_sources(
    python_sources,
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
