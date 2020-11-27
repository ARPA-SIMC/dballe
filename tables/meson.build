
crexbtables = files('B990203.txt')
crexdtables = files('D990203.txt')
bufrbtables = files('B000000002001200.txt')
bufrdtables = files('D000000002001200.txt', 'D000000002001400.txt', 'D0000000000200014001.txt')

install_data(
    'repinfo.csv',
    'dballe.txt',
    crexbtables,
    crexdtables,
    bufrbtables,
    bufrdtables,
    install_dir: table_dir
)

cp = find_program('cp')
cut = find_program('cut')

foreach g: ['B000000002001400.txt', 'B0000000000200014001.txt']
    custom_target(g, input: 'dballe.txt', output: g, command: [cut, '-b', '1-119', '@INPUT@'], capture: true, install: true, install_dir: table_dir)
endforeach

foreach g: ['B990204.txt', 'B000299.txt']
    custom_target(g, input: 'dballe.txt', output: g, command: [cp, '@INPUT@', '@OUTPUT@'], install: true, install_dir: table_dir)
endforeach
