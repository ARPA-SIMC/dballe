import wx, os, re, sys
from provami.ProgressDisplay import ProgressDisplay
from provami.Model import Model, ModelListener, ProgressListener
from provami.MapChoice import MapChoice
from provami.MapCanvas import MapCanvas
from provami.QueryPanel import QueryPanel
from provami.LimitChoice import LimitChoice
from provami.DataResults import DataPanel
from provami.ResultGrid import ResultGrid
from provami.AnaDetails import AnaResults
from provami.AttrDetails import AttrResults
from provami.FilterWindow import FilterWindow

class Navigator(wx.Frame, ProgressListener, ModelListener):
	ACTION_MAP_WINDOW = wx.NewId()
	ACTION_ANA_WINDOW = wx.NewId()
	ACTION_ATTR_WINDOW = wx.NewId()
	ACTION_FILTERS_WINDOW = wx.NewId()
	ACTION_QUIT = wx.NewId()
	ACTION_DELETE_SELECTED = wx.NewId()
	ACTION_DELETE_ORPHANS = wx.NewId()
	ACTION_EXPORT = wx.NewId()
	ACTION_REFRESH = wx.NewId()

	ICON_PROVAMI = "provami_icon"
	ICON_MAP_WINDOW = "provami_map_window"
	ICON_ANA_WINDOW = "provami_ana_window"
	ICON_ATTR_WINDOW = "provami_attr_window"
	ICON_FILTERS_WINDOW = "provami_filters_window"

	def __init__(self, parent, model, title):
		wx.Frame.__init__(self, parent, -1, title, size = (600, 600),
				style=wx.DEFAULT_FRAME_STYLE | wx.NO_FULL_REPAINT_ON_RESIZE)

		self.model = model
		self.model.registerProgressListener(self)
		self.model.registerUpdateListener(self)

		icon = wx.EmptyIcon()
		icon.CopyFromBitmap(wx.ArtProvider.GetBitmap(Navigator.ICON_PROVAMI, wx.ART_TOOLBAR, (16, 16)))
		self.SetIcon(icon)

		# Create progress display manager
		self.progressDisplay = ProgressDisplay(self, model)

		# Create toolbar
		self.tb = self.CreateToolBar(wx.TB_HORIZONTAL | wx.NO_BORDER | wx.TB_FLAT | wx.TB_TEXT)
		bmp =  wx.ArtProvider.GetBitmap(Navigator.ICON_MAP_WINDOW, wx.ART_TOOLBAR, (16,16))
                self.tb.AddCheckTool(Navigator.ACTION_MAP_WINDOW, bmp, shortHelp="Open/close map", \
					longHelp="Open/close map window")

		bmp =  wx.ArtProvider.GetBitmap(Navigator.ICON_ANA_WINDOW, wx.ART_TOOLBAR, (16,16))
                self.tb.AddCheckTool(Navigator.ACTION_ANA_WINDOW, bmp, shortHelp="Open/close pseudoana details", \
					longHelp="Open/close pseudoana details window")

		bmp =  wx.ArtProvider.GetBitmap(Navigator.ICON_ATTR_WINDOW, wx.ART_TOOLBAR, (16,16))
                self.tb.AddCheckTool(Navigator.ACTION_ATTR_WINDOW, bmp, shortHelp="Open/close attribute details", \
					longHelp="Open/close attribute details window")
		self.Bind(wx.EVT_TOOL, self.onToolClick)

		bmp =  wx.ArtProvider.GetBitmap(Navigator.ICON_FILTERS_WINDOW, wx.ART_TOOLBAR, (16,16))
                self.tb.AddCheckTool(Navigator.ACTION_FILTERS_WINDOW, bmp, shortHelp="Open/close extra filters", \
					longHelp="Open/close extra filters window")
		self.Bind(wx.EVT_TOOL, self.onToolClick)

		self.tb.ToggleTool(Navigator.ACTION_MAP_WINDOW, True)

		# Create menu bar
		self.menuBar = wx.MenuBar()

		mFile = wx.Menu()
		mFile.Append(Navigator.ACTION_EXPORT, "&Export...", "Export all selected data")
		mFile.AppendSeparator()
		mFile.Append(Navigator.ACTION_QUIT, "&Quit", "Quit wxdballe")
		self.menuBar.Append(mFile, "&File")

		mView = wx.Menu()
		mView.Append(Navigator.ACTION_MAP_WINDOW, "&Map", "Map window", wx.ITEM_CHECK)
		mView.Append(Navigator.ACTION_ANA_WINDOW, "&Pseudoana", "Pseudoana window", wx.ITEM_CHECK)
		mView.Append(Navigator.ACTION_ATTR_WINDOW, "&Attributes", "Attributes window", wx.ITEM_CHECK)
		mView.Append(Navigator.ACTION_FILTERS_WINDOW, "&Extra filters", "Extra filters window", wx.ITEM_CHECK)
		mView.AppendSeparator()
		mView.Append(Navigator.ACTION_REFRESH, "&Refresh", "Reload data from database")
		self.menuBar.Append(mView, "&View")
		self.menuBar.Check(Navigator.ACTION_MAP_WINDOW, True)

		mEdit = wx.Menu()
		mEdit.Append(Navigator.ACTION_DELETE_SELECTED, "&Delete displayed values", "Delete all values currently displayed")
		mEdit.Append(Navigator.ACTION_DELETE_ORPHANS, "&Delete empty stations", "Delete all stations with no data")
		self.menuBar.Append(mEdit, "&Edit")

		self.SetMenuBar(self.menuBar)
		self.Bind(wx.EVT_MENU, self.onToolClick)

		# Create status bar
		self.statusBar = self.CreateStatusBar()

		# Create extra windows
		self.mapWindow = MapChoice(self, model);
		self.mapWindow.SetIcon(icon)
		self.mapWindow.Show()
		self.mapWindow.Bind(MapCanvas.EVT_CURRENT_STATION_CHANGED, self.onChangedCurrentStation)

		self.query = QueryPanel(self, model)
		self.dataPanel = DataPanel(self, model, self.statusBar)
		self.dataPanel.Bind(ResultGrid.EVT_CHANGED_CURRENT_ROW, self.onChangedCurrentRow)

		box = wx.BoxSizer(wx.VERTICAL)
		box.Add(self.query, 0, flag=wx.EXPAND)
		box.Add(LimitChoice(self, model), 0, flag=wx.EXPAND)
		box.Add(self.dataPanel, 1, flag=wx.EXPAND)
		self.SetSizerAndFit(box)

		self.anaWindow = AnaResults(self, model)
		self.anaWindow.SetIcon(icon)
		self.anaWindow.Bind(ResultGrid.EVT_CHANGED_CURRENT_ROW, self.onChangedCurrentRow)
		#self.dataPanel.results.registerRecordListener(self.anaWindow)

		self.attrWindow = AttrResults(self, model)
		self.attrWindow.SetIcon(icon)
		#self.attrWindow.registerUpdatesWith(self.dataPanel.results)
		#self.attrWindow.registerUpdatesWith(self.anaWindow)

		self.filtersWindow = FilterWindow(self, model)
		self.filtersWindow.SetIcon(icon)

		self.last_context_id = None
		self.last_var = None
		self.updating = False

	def invalidate(self):
		self.updating = True

	def hasData(self, what):
#		"""
#		Try to reposition on the previously selected items after a data update
#		"""
		if what != "all": return
#			record = self.dataPanel.results.getFirstRow()
#
#		if self.last_context_id == None or self.last_var == None:
#			# Position on the first row
#			record = self.dataPanel.results.getFirstRow()
#		else:
#			# Try to get the previously selected item
#			record = self.model.recordByContextAndVar(self.last_context_id, self.last_var)
#			if record == None:
#				# If it's not in the results anymore, then go
#				# on the first row
#				record = self.dataPanel.results.getFirstRow()
#
#		if record != None:
#			context, var = record.enqi("context_id"), record.enqc("var")
#			print "HDR", context, var
#			row = self.dataPanel.results.GetTable().rowByContextAndVar(context, var)
#			self.anaWindow.displayRecord(record)
#			self.attrWindow.display(record)
#			self.last_context_id = record.enqi("context_id")
#			self.last_var = record.enqc("var")
#		else:
#			print "HDR none"
#			self.last_context_id = None
#			self.last_var = None
#
		self.updating = False

	def queryError(self, message):
		cnf = wx.MessageDialog(self,
			"The DB-ALLe query has failed.  The error is: " + message,
			'Query failed',
			wx.OK | wx.ICON_ERROR
			)
		cnf.ShowModal()
		cnf.Destroy()

	def onChangedCurrentStation(self, event):
		if self.updating: return
		"Handle change of currently selected station in the map"
		id = event.GetStation()
		self.anaWindow.displayID(id)

	def onChangedCurrentRow(self, event):
		if self.updating: return
		self.updating = True
		record = event.GetData()
		self.anaWindow.displayRecord(record)
		self.attrWindow.display(record)
		self.updating = False

	def mapHasClosed(self):
		self.tb.ToggleTool(Navigator.ACTION_MAP_WINDOW, False)
		self.menuBar.Check(Navigator.ACTION_MAP_WINDOW, False)

	def anaHasClosed(self):
		self.tb.ToggleTool(Navigator.ACTION_ANA_WINDOW, False)
		self.menuBar.Check(Navigator.ACTION_ANA_WINDOW, False)

	def attrHasClosed(self):
		self.tb.ToggleTool(Navigator.ACTION_ATTR_WINDOW, False)
		self.menuBar.Check(Navigator.ACTION_ATTR_WINDOW, False)

	def filtersHasClosed(self):
		self.tb.ToggleTool(Navigator.ACTION_FILTERS_WINDOW, False)
		self.menuBar.Check(Navigator.ACTION_FILTERS_WINDOW, False)

	def onToolClick(self, event):
		if event.GetId() == Navigator.ACTION_MAP_WINDOW:
			state = self.mapWindow.IsShown()
			self.tb.ToggleTool(Navigator.ACTION_MAP_WINDOW, not state)
			self.menuBar.Check(Navigator.ACTION_MAP_WINDOW, not state)
			self.mapWindow.Show(not state)
		elif event.GetId() == Navigator.ACTION_ANA_WINDOW:
			state = self.anaWindow.IsShown()
			self.tb.ToggleTool(Navigator.ACTION_ANA_WINDOW, not state)
			self.menuBar.Check(Navigator.ACTION_ANA_WINDOW, not state)
			self.anaWindow.Show(not state)
		elif event.GetId() == Navigator.ACTION_ATTR_WINDOW:
			state = self.attrWindow.IsShown()
			self.tb.ToggleTool(Navigator.ACTION_ATTR_WINDOW, not state)
			self.menuBar.Check(Navigator.ACTION_ATTR_WINDOW, not state)
			self.attrWindow.Show(not state)
		elif event.GetId() == Navigator.ACTION_FILTERS_WINDOW:
			state = self.filtersWindow.IsShown()
			self.tb.ToggleTool(Navigator.ACTION_FILTERS_WINDOW, not state)
			self.menuBar.Check(Navigator.ACTION_FILTERS_WINDOW, not state)
			self.filtersWindow.Show(not state)
		elif event.GetId() == Navigator.ACTION_REFRESH:
			self.model.update()
		elif event.GetId() == Navigator.ACTION_QUIT:
			self.Destroy()
		elif event.GetId() == Navigator.ACTION_DELETE_SELECTED:
			self.model.deleteCurrent()
		elif event.GetId() == Navigator.ACTION_DELETE_ORPHANS:
			self.model.deleteOrphans()
		elif event.GetId() == Navigator.ACTION_EXPORT:
#			dlg = wx.FileDialog(
#					self, message="Save file as...", defaultDir=os.getcwd(), 
#					defaultFile="", style=wx.SAVE
#					)
			dlg = wx.FileDialog(
					self, message="Save file as...", defaultDir=os.getcwd(), 
					#defaultFile="", wildcard="According to file extension|*|BUFR optimal template (*.bufr)|*.bufr|BUFR generic template (*.bufr)|*.bufr|CREX (*.crex)|*.crex|Comma Separated Values (*.csv)|*.csv|GNU R data file (*.Rdata)|*.Rdata|Pickled volNd Python objects (*.volnd)|*.volnd", style=wx.SAVE
					defaultFile="", wildcard="According to file extension|*|BUFR optimal template (*.bufr)|*.bufr|BUFR generic template (*.bufr)|*.bufr|CREX (*.crex)|*.crex|Comma Separated Values (*.csv)|*.csv|GNU R data file (*.Rdata)|*.Rdata", style=wx.SAVE
					)

			# Show the dialog and retrieve the user response. If it is the OK response, 
			# process the data.
			path = None
			eidx = 0
			if dlg.ShowModal() == wx.ID_OK:
				path = dlg.GetPath()
				eidx = dlg.GetFilterIndex()
#				# Check if it exists
#				if os.access(path, os.F_OK):
#					cnf = wx.MessageDialog(self,
#						path + " already exists.  Would you like to overwrite it?",
#						'File exists',
#						wx.OK | wx.ICON_INFORMATION | wx.YES_NO
#						)
#					if cnf.ShowModal() != wx.ID_OK:
#						path = None
#					cnf.Destroy()
			dlg.Destroy()

			if path != None:
				# Determine file type by its extension, defaulting to bufr
				encoding = "BUFR"
				# According to file extension
				if eidx == 0:
					if re.search('.crex$', path, re.IGNORECASE):
						encoding = "CREX"
					elif re.search('.csv$', path, re.IGNORECASE):
						encoding = "CSV"
					elif re.search('.Rdata$', path, re.IGNORECASE):
						encoding = "R"
				# BUFR optimal template (*.bufr)
				elif eidx == 1:
					encoding = "BUFR"
				# BUFR generic template (*.bufr)
				elif eidx == 2:
					encoding = "gBUFR"
				# CREX (*.crex)
				elif eidx == 3:
					encoding = "CREX"
				# Comma Separated Values (*.csv)
				elif eidx == 4:
					encoding = "CSV"
				elif eidx == 5:
					encoding = "R"
				#elif eidx == 6:
				#	encoding = "VOLND"

				# Add extension if missing
				if os.path.basename(path).find(".") == -1:
					extensions = {"BUFR": ".bufr",
						      "gBUFR": ".bufr",
						      "CREX": ".crex",
						      "CSV": ".csv",
						      "R": ".Rdata"}
					path = path + extensions.get(encoding, "")

				pdlg = None
				try:
					try:
						pdlg = wx.ProgressDialog("Saving...",
								"Saving file " + path + "...",
								maximum=100,
								parent=self,
								style=wx.PD_APP_MODAL)
						wx.Yield()
						self.model.exportToFile(str(path), encoding)
						# It seems that it never returns from this function
						#pdlg.Update(100, "Done.")
					except RuntimeError:
						error = ": ".join(map(str, sys.exc_info()[:2]))
						dlg = wx.MessageDialog(self, "Saving " + path + " failed: " + error,
									'Error saving ' + path,
									wx.OK | wx.ICON_ERROR | wx.ICON_INFORMATION
									)
						dlg.ShowModal()
						print >>sys.stderr, error
						dlg.Destroy()
				finally:
					if pdlg:
						pdlg.Destroy()
		else:
			event.Skip()
