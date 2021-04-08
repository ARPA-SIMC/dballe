foreach tool: ['dbatbl', 'dbamsg', 'dbadb']
    e = executable(tool, tool + '.cc',
               link_with: [libdballe],
               include_directories: toplevel_inc,
               dependencies: [libwreport_dep, popt_dep],
               install: true,
    )
    # We cannot currently use install_man with custom_target.
    # See: https://github.com/mesonbuild/meson/issues/1550
    custom_target(tool + '.1',
                  output: tool + '.1',
                  command: [e, 'help', 'manpage'],
                  capture: true,
                  install: true,
                  install_dir: get_option('mandir') / 'man1')
endforeach

install_data('dbaexport', install_dir: get_option('bindir'))

help2man = find_program('help2man')

custom_target('dbaexport.1',
              input: 'dbaexport',
              output: 'dbaexport.1',
              command: [help2man, '--name=export data from DB-All.e', '--section=1', '--no-info', '--version-string=' + meson.project_version(), '@INPUT@'],
              capture: true,
              install: true,
              install_dir: get_option('mandir') / 'man1')
