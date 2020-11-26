libdballe_sources += files(
    'utils.cc',
    'trace.cc',
    'transaction.cc',
    'batch.cc',
    'cache.cc',
    'repinfo.cc',
    'station.cc',
    'levtr.cc',
    'data.cc',
    'driver.cc',
    'sqlite/repinfo.cc',
    'sqlite/station.cc',
    'sqlite/levtr.cc',
    'sqlite/data.cc',
    'sqlite/driver.cc',
    'db.cc',
    'cursor.cc',
    'qbuilder.cc',
    'import.cc',
    'export.cc',
)

install_headers(
    'fwd.h',
    'utils.h',
    'trace.h',
    'transaction.h',
    'batch.h',
    'cache.h',
    'internals.h',
    'repinfo.h',
    'station.h',
    'levtr.h',
    'data.h',
    'driver.h',
    'sqlite/repinfo.h',
    'sqlite/station.h',
    'sqlite/levtr.h',
    'sqlite/data.h',
    'sqlite/driver.h',
    'db.h',
    'cursor.h',
    'qbuilder.h',
    subdir: 'dballe/db/v7',
)
if libpq_dep.found()
    install_headers(
	'postgresql/repinfo.h',
	'postgresql/station.h',
	'postgresql/levtr.h',
	'postgresql/data.h',
	'postgresql/driver.h',
        subdir: 'dballe/db/v7/postgresql',
    )
endif
if mariadb_dep.found()
    install_headers(
	'mysql/repinfo.h',
	'mysql/station.h',
	'mysql/levtr.h',
	'mysql/data.h',
	'mysql/driver.h',
        subdir: 'dballe/db/v7/mysql',
    )
endif

foreach f: [['cursor-access.in.cc', 'cursor-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach
