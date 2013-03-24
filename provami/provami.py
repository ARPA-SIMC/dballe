#!/usr/bin/python

from optparse import OptionParser
import sys, os

VERSION="1.0"

###
### Main
###

class Parser(OptionParser):
    def __init__(self, *args, **kwargs):
        OptionParser.__init__(self, *args, **kwargs)

    def error(self, msg):
        sys.stderr.write("%s: error: %s\n\n" % (self.get_prog_name(), msg))
        self.print_help(sys.stderr)
        sys.exit(2)

if __name__ == "__main__":
    parser = Parser(usage="usage: %prog [options]",
            version="%prog "+ VERSION,
            description="Navigate a DB-ALLe database")
    parser.add_option("--dsn", default=os.environ.get("DBA_DB", "test"), help="DSN, or URL-like database definition, to use for connecting to the DB-All.e database (default: %default, can be set in the environment as DBA_DB)"),
    parser.add_option("--user", default=os.environ['USER'], help="User name to use to connect to the database (default: %default)")
    parser.add_option("--pass", default="", dest="password", help="Password to use to connect to the database (default: none)")

    (opts, args) = parser.parse_args()

    import dballe
    import wx

    from provami.Model import *

    from provami.ProvamiArtProvider import ProvamiArtProvider
    from provami.Navigator import Navigator

    app = wx.PySimpleApp()
    if dballe.DB.is_url(opts.dsn):
        db = dballe.DB.connect_from_url(opts.dsn)
    else:
        db = dballe.DB.connect(opts.dsn, opts.user, opts.password)
    model = Model(db)


    for q in args:
        model.filter.setFromString(q)

    wx.ArtProvider.PushProvider(ProvamiArtProvider())

    navigator = Navigator(None, model, "Provami - DB-ALLe Navigator")
    navigator.Show()
    model.update()
    app.MainLoop()
