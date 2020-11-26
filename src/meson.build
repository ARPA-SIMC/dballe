foreach tool: ['dbatbl', 'dbamsg', 'dbadb']
    executable(tool, tool + '.cc',
               link_with: [libdballe],
               include_directories: toplevel_inc,
               dependencies: [libwreport_dep, popt_dep],
               install: true,
    )
endforeach

install_data('dbaexport', install_dir: get_option('bindir'))
