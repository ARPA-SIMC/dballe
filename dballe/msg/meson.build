libdballe_sources += files(
        'bulletin.cc',
        'context.cc',
        'msg.cc',
        'cursor.cc',
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

foreach f: [['msg-cursor-access.in.cc', 'msg-cursor-access.cc']]
    libdballe_sources += custom_target(f[1], output: f[1], input: f[0], command: [mklookup, '@INPUT@', '-o', '@OUTPUT@'])
endforeach
