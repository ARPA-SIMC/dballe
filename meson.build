project('dballe', 'cpp', 'fortran', version: '8.13', license : 'GPL-2.0-or-later')

conf_data = configuration_data()
configure_file(output: 'config.h', configuration: conf_data)

toplevel_inc = include_directories('.')

# Dependencies
libwreport_dep = dependency('libwreport', version: '>= 3.22')
sqlite3_dep = dependency('sqlite3')
libpq_dep = dependency('libpq', required: false)
mariadb_dep = dependency('mariadb', required: false)
xapian_dep = dependency('xapian-core', version: '>= 1.4', required: false)
popt_dep = dependency('popt')

subdir('dballe')
