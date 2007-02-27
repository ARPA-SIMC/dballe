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
	parser.add_option("--dsn", default="test", help="DSN to use to connect to the database (default: %default)")
	parser.add_option("--user", default=os.environ['USER'], help="User name to use to connect to the database (default: %default)")
	parser.add_option("--pass", default="", dest="password", help="Password to use to connect to the database (default: none)")

	(options, args) = parser.parse_args()

	import dballe
	import string
	import re
	import wx

	from provami.Model import *
	#from provami.ModelSingleQuery import Model
	from provami.MapCanvas import MapCanvas
	from provami.MapChoice import MapChoice
	#from provami.MapChoiceSingleQuery import MapChoice
	#from provami.DateCanvas import DateCanvas

	from provami.ProvamiArtProvider import ProvamiArtProvider
	from provami.Navigator import Navigator

	app = wx.PySimpleApp()
        model = Model(options.dsn, options.user, options.password)

	for q in args:
		model.filter.setFromString(q)

	wx.ArtProvider.PushProvider(ProvamiArtProvider())

	navigator = Navigator(None, model, "Provami - DB-ALLe Navigator")
	navigator.Show()
	model.update()
	app.MainLoop()
