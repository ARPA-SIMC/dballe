foreach f: [['cursor-access.in.cc', 'cursor-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach
