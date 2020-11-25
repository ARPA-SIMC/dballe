project('dballe', 'cpp', 'fortran', version: '8.13', license : 'GPL-2.0-or-later')

conf_data = configuration_data()

toplevel_inc = include_directories('.')

# Dependencies
libwreport_dep = dependency('libwreport', version: '>= 3.22')
sqlite3_dep = dependency('sqlite3')
libpq_dep = dependency('libpq', required: false)
conf_data.set('HAVE_LIBPQ', libpq_dep.found())
mariadb_dep = dependency('mariadb', required: false)
conf_data.set('HAVE_MYSQL', mariadb_dep.found())
xapian_dep = dependency('xapian-core', version: '>= 1.4', required: false)
conf_data.set('HAVE_XAPIAN', xapian_dep.found())
popt_dep = dependency('popt')
gperf = find_program('gperf')

table_dir = get_option('datadir') / 'wreport'

# Generate config.h
configure_file(output: 'config.h', configuration: conf_data)

subdir('dballe')

