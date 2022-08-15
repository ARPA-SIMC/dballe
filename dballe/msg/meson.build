libdballe_sources += files(
        'bulletin.cc',
        'context.cc',
        'msg.cc',
        'cursor.cc',
        'domain_errors.cc',
        'json_codec.cc',
        'wr_codec.cc',
        'wr_importers/base.cc',
        'wr_importers/synop.cc',
        'wr_importers/ship.cc',
        'wr_importers/temp.cc',
        'wr_importers/metar.cc',
        'wr_importers/flight.cc',
        'wr_importers/generic.cc',
        'wr_importers/pollution.cc',
        'wr_exporters/common.cc',
        'wr_exporters/synop.cc',
        'wr_exporters/ship.cc',
        'wr_exporters/buoy.cc',
        'wr_exporters/metar.cc',
        'wr_exporters/temp.cc',
        'wr_exporters/flight.cc',
        'wr_exporters/generic.cc',
        'wr_exporters/pollution.cc',
)

install_headers(
    'fwd.h',
    'bulletin.h',
    'context.h',
    'msg.h',
    'cursor.h',
    'domain_errors.h',
    'json_codec.h',
    'wr_codec.h',
    subdir: 'dballe/msg',
)

install_headers(
    'wr_exporters/common.h',
    subdir: 'dballe/msg/wr_exporters',
)

foreach f: [['msg-cursor-access.in.cc', 'msg-cursor-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach

libdballe_sources += custom_target('msg-extravars.h', input: '../vars.csv', output: 'msg-extravars.h',
                                   command: [mkvars, '-t', 'msg-extravars.h', '@INPUT@', '-o', '@OUTPUT@'],
				   install_dir: get_option('includedir') + '/' + 'dballe/msg')
