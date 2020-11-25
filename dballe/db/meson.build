libdballe_sources += files(
        'db.cc',
        'v7/utils.cc',
        'v7/trace.cc',
        'v7/transaction.cc',
        'v7/batch.cc',
        'v7/cache.cc',
        'v7/repinfo.cc',
        'v7/station.cc',
        'v7/levtr.cc',
        'v7/data.cc',
        'v7/driver.cc',
        'v7/sqlite/repinfo.cc',
        'v7/sqlite/station.cc',
        'v7/sqlite/levtr.cc',
        'v7/sqlite/data.cc',
        'v7/sqlite/driver.cc',
        'v7/db.cc',
        'v7/cursor.cc',
        'v7/qbuilder.cc',
        'v7/import.cc',
        'v7/export.cc',
        'summary.cc',
        'summary_utils.cc',
        'summary_memory.cc',
)

foreach f: [['summary-access.in.cc', 'summary-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach

subdir('v7')
