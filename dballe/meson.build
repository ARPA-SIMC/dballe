#
# Autobuilt files
#

mklookup = find_program('./mklookup')
mkvars = find_program('./mkvars')

libdballe_sources = [
        'file.cc',
        'importer.cc',
        'exporter.cc',
        'message.cc',
        'types.cc',
        'value.cc',
        'values.cc',
        'data.cc',
        'query.cc',
        'var.cc',
        'cursor.cc',
        'db.cc',
        'sql/sql.cc',
        'sql/querybuf.cc',
        'sql/sqlite.cc',
        'db/explorer.cc',
        'cmdline/cmdline.cc',
        'cmdline/processor.cc',
        'cmdline/conversion.cc',
        'cmdline/dbadb.cc',
]

subdir('core')
subdir('msg')
subdir('db')
subdir('fortran')

if libpq_dep.found()
        libdballe_sources += [
                'sql/postgresql.cc',
                'db/v7/postgresql/repinfo.cc',
                'db/v7/postgresql/station.cc',
                'db/v7/postgresql/levtr.cc',
                'db/v7/postgresql/data.cc',
                'db/v7/postgresql/driver.cc',
        ]
endif

if mariadb_dep.found()
        libdballe_sources += [
                'sql/mysql.cc',
                'db/v7/mysql/repinfo.cc',
                'db/v7/mysql/station.cc',
                'db/v7/mysql/levtr.cc',
                'db/v7/mysql/data.cc',
                'db/v7/mysql/driver.cc',
        ]
endif

if xapian_dep.found()
        libdballe_sources += [
                'db/summary_xapian.cc',
        ]
endif


libdballe = both_libraries('dballe',
        libdballe_sources,
        install: true,
        include_directories: toplevel_inc,
        dependencies: [
                libwreport_dep,
                sqlite3_dep,
                libpq_dep,
                mariadb_dep,
                xapian_dep,
                popt_dep,
        ])


#
# Unit testing
#

test_dballe_sources = [
        'tests-main.cc',
        'core/tests.cc',
        'core/shortcuts-test.cc',
        'file-test.cc',
        'importer-test.cc',
        'exporter-test.cc',
        'message-test.cc',
        'types-test.cc',
        'value-test.cc',
        'values-test.cc',
        'data-test.cc',
        'query-test.cc',
        'var-test.cc',
        'cursor-test.cc',
        'db-test.cc',
        'core/cursor-test.cc',
        'core/aliases-test.cc',
        'core/defs-test.cc',
        'core/var-test.cc',
        'core/values-test.cc',
        'core/file-test.cc',
        'core/data-test.cc',
        'core/query-test.cc',
        'core/structbuf-test.cc',
        'core/csv-test.cc',
        'core/matcher-test.cc',
        'core/match-wreport-test.cc',
        'core/smallset-test.cc',
        'core/varmatch-test.cc',
        'core/json-test.cc',
        'core/string-test.cc',
        'msg/tests.cc',
        'msg/bulletin-test.cc',
        'msg/context-test.cc',
        'msg/msg-test.cc',
        'msg/cursor-test.cc',
        'msg/json_codec-test.cc',
        'msg/wr_codec_generic-test.cc',
        'msg/wr_codec-test.cc',
        'msg/wr_import-test.cc',
        'msg/wr_export-test.cc',
        'sql/querybuf-test.cc',
        'sql/sqlite-test.cc',
        'db/tests.cc',
        'db/v7/utils-test.cc',
        'db/v7/trace-test.cc',
        'db/v7/batch-test.cc',
        'db/v7/cache-test.cc',
        'db/v7/repinfo-test.cc',
        'db/v7/station-test.cc',
        'db/v7/levtr-test.cc',
        'db/v7/data-test.cc',
        'db/db-test.cc',
        'db/db-basic-test.cc',
        'db/db-misc-test.cc',
        'db/db-query-station-test.cc',
        'db/db-query-data-test.cc',
        'db/db-query-summary-test.cc',
        'db/db-import-test.cc',
        'db/db-export-test.cc',
        'db/summary-test.cc',
        'db/explorer-test.cc',
        'fortran/traced-test.cc',
        'fortran/commonapi-test.cc',
        'fortran/msgapi-test.cc',
        'fortran/dbapi-test.cc',
        'cmdline/processor-test.cc',
        'cmdline/dbadb-test.cc',
]

if libpq_dep.found()
        test_dballe_sources += [
                'sql/postgresql-test.cc',
        ]
endif

if mariadb_dep.found()
        test_dballe_sources += [
                'sql/mysql-test.cc',
        ]
endif

if xapian_dep.found()
        libdballe_sources += [
                'db/summary_xapian.cc',
        ]
endif

test_dballe = executable('test-dballe', test_dballe_sources,
        include_directories: toplevel_inc,
        link_with: [
                libdballe,
        ],
        dependencies: [
                libwreport_dep,
                sqlite3_dep,
                libpq_dep,
                mariadb_dep,
                xapian_dep,
                popt_dep,
        ])

runtest = find_program('../extra/runtest')

test('dballe', runtest, args: [test_dballe])
