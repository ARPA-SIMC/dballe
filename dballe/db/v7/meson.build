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

foreach f: [['cursor-access.in.cc', 'cursor-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach
