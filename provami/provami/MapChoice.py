import wx
from provami.Model import *
from provami.MapCanvas import MapCanvas, MapCanvasListener
from provami.IdentChoice import IdentChoice
from provami.QueryButton import QueryButton

class MapChoice(wx.Frame, ModelListener, MapCanvasListener):

	# Toolbar IDs
	TB_ZOOM_IN=10
	TB_ZOOM_OUT=20
	TB_MOVE=30
	TB_SELECT_AREA=40
	TB_SELECT_STATION=50
	TB_SELECT_IDENT=60

	ICON_MAP_MOVE = "provami-map-move"
	ICON_MAP_SELECT_AREA = "provami-select-area"
	ICON_MAP_SELECT_STATION = "provami-select-station"
	ICON_MAP_SELECT_IDENT = "provami-select-ident"

	def __init__(self, parent, model):
		wx.Frame.__init__(self, parent, -1, "Map", size = (500, 500), style=wx.DEFAULT_FRAME_STYLE | wx.NO_FULL_REPAINT_ON_RESIZE)

		self.points = []
		self.userScale = 1.0
		self.mouseDown = False

		# 0: move
		# 1: select area
		# 2: select station
		self.mode = 0

		self.model = model
		self.model.registerUpdateListener(self)

		# When closing the window, notify parent so that it can enable
		# the button to open us again
		self.parent = parent
		self.Bind(wx.EVT_CLOSE, self.onClose)

		self.map = MapCanvas(self, model)
		self.map.loadShoreLines(filePath('world.dat'))
		#self.map.loadShoreLines(filePath('nations.dat'))
		self.map.Bind(MapCanvas.EVT_MODE_CHANGED, self.modeChanged)

		# Flag set to true when the input fields are updated internally
		# insted of on user input
		self.updating = True

		self.tb = self.CreateToolBar(wx.TB_HORIZONTAL | wx.NO_BORDER | wx.TB_FLAT | wx.TB_TEXT)
		        
		bmp =  wx.ArtProvider.GetBitmap("gtk-zoom-in", wx.ART_TOOLBAR, (16,16))
		self.tb.AddSimpleTool(MapChoice.TB_ZOOM_IN, bmp, "Zoom in", "Zoom in")
		self.Bind(wx.EVT_TOOL, self.onToolClick, id=MapChoice.TB_ZOOM_IN)
		bmp =  wx.ArtProvider.GetBitmap("gtk-zoom-out", wx.ART_TOOLBAR, (16,16))
		self.tb.AddSimpleTool(MapChoice.TB_ZOOM_OUT, bmp, "Zoom out", "Zoom out")
		self.Bind(wx.EVT_TOOL, self.onToolClick, id=MapChoice.TB_ZOOM_OUT)

		bmp =  wx.ArtProvider.GetBitmap(MapChoice.ICON_MAP_MOVE, wx.ART_TOOLBAR, (16,16))
		self.tb.AddRadioTool(MapChoice.TB_MOVE, bmp, shortHelp="Move", longHelp="Move the map")
		self.Bind(wx.EVT_TOOL, self.onToolClick, id=MapChoice.TB_MOVE)
		bmp =  wx.ArtProvider.GetBitmap(MapChoice.ICON_MAP_SELECT_AREA, wx.ART_TOOLBAR, (16,16))
		self.tb.AddRadioTool(MapChoice.TB_SELECT_AREA, bmp, shortHelp="Select area", longHelp="Select area")
		self.Bind(wx.EVT_TOOL, self.onToolClick, id=MapChoice.TB_SELECT_AREA)
		bmp =  wx.ArtProvider.GetBitmap(MapChoice.ICON_MAP_SELECT_STATION, wx.ART_TOOLBAR, (16,16))
		self.tb.AddRadioTool(MapChoice.TB_SELECT_STATION, bmp, shortHelp="Select station", longHelp="Select station")
		self.Bind(wx.EVT_TOOL, self.onToolClick, id=MapChoice.TB_SELECT_STATION)
		bmp =  wx.ArtProvider.GetBitmap(MapChoice.ICON_MAP_SELECT_IDENT, wx.ART_TOOLBAR, (16,16))
		self.tb.AddRadioTool(MapChoice.TB_SELECT_IDENT, bmp, shortHelp="Select same ident", longHelp="Select same ident")
		self.Bind(wx.EVT_TOOL, self.onToolClick, id=MapChoice.TB_SELECT_IDENT)
		self.tb.ToggleTool(MapChoice.TB_MOVE, True)

		buttonPanel = wx.Panel(self)
		bpsizer = wx.GridBagSizer()

		self.timedUpdater = None
		self.latminSpin = wx.TextCtrl(buttonPanel)
		self.Bind(wx.EVT_TEXT, self.onSpinChanged, self.latminSpin)
		#self.latminSpin.SetRange(-90, 90)
		self.latminSpin.SetValue('')
		self.latmaxSpin = wx.TextCtrl(buttonPanel)
		self.Bind(wx.EVT_TEXT, self.onSpinChanged, self.latmaxSpin)
		#self.latmaxSpin.SetRange(-90, 90)
		self.latmaxSpin.SetValue('')
		self.lonminSpin = wx.TextCtrl(buttonPanel)
		self.Bind(wx.EVT_TEXT, self.onSpinChanged, self.lonminSpin)
		#self.lonminSpin.SetRange(-180, 180)
		self.lonminSpin.SetValue('')
		self.lonmaxSpin = wx.TextCtrl(buttonPanel)
		self.Bind(wx.EVT_TEXT, self.onSpinChanged, self.lonmaxSpin)
		#self.lonmaxSpin.SetRange(-180, 180)
		self.lonmaxSpin.SetValue('')
		self.idField = wx.TextCtrl(buttonPanel)
		self.Bind(wx.EVT_TEXT, self.onSpinChanged, self.idField)

		for i in self.latminSpin, self.latmaxSpin, self.lonminSpin, self.lonmaxSpin, self.idField:
			i.Enable(False)

		self.updating = False

		bpsizer.Add(wx.StaticText(buttonPanel, -1, "Lat: "), (0, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		bpsizer.Add(self.latminSpin, (0, 1), (1, 1))
		bpsizer.Add(wx.StaticText(buttonPanel, -1, " to "), (0, 2), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		bpsizer.Add(self.latmaxSpin, (0, 3), (1, 1))
		bpsizer.Add(wx.StaticText(buttonPanel, -1, "Lon: "), (1, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		bpsizer.Add(self.lonminSpin, (1, 1), (1, 1))
		bpsizer.Add(wx.StaticText(buttonPanel, -1, " to "), (1, 2), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		bpsizer.Add(self.lonmaxSpin, (1, 3), (1, 1))

		bpsizer.Add(IdentChoice(buttonPanel, self.model), (0, 4), (1, 2), flag=wx.EXPAND)

		bpsizer.Add(wx.StaticText(buttonPanel, -1, "Id: "), (1, 4), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		bpsizer.Add(self.idField, (1, 5), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)

		self.resetButton = wx.Button(buttonPanel, -1, "Reset filter")
		self.Bind(wx.EVT_BUTTON, self.onResetButton, self.resetButton)
		bpsizer.Add(self.resetButton, (0, 6), (1, 1), flag=wx.ALIGN_RIGHT)

		bpsizer.Add(QueryButton(buttonPanel, model), (1, 6), (1, 1))

		buttonPanel.SetSizer(bpsizer)

		box = wx.BoxSizer(wx.VERTICAL)
		box.Add(self.map, 1, wx.EXPAND)
		box.Add(buttonPanel)
		self.SetSizerAndFit(box)

		self.invalidate()
		self.hasData("stations")

	def modeChanged(self, event):
		"Handle mode change on the map canvas"
		oldMode, mode = event.GetModeChange()

		if mode == MapCanvas.MODE_MOVE:
			self.tb.ToggleTool(MapChoice.TB_MOVE, True)
			for i in self.latminSpin, self.latmaxSpin, self.lonminSpin, self.lonmaxSpin, self.idField:
				i.Enable(False)
		elif mode == MapCanvas.MODE_SELECT_AREA:
			self.tb.ToggleTool(MapChoice.TB_SELECT_AREA, True)
			for i in self.latminSpin, self.latmaxSpin, self.lonminSpin, self.lonmaxSpin:
				i.Enable(True)
			self.idField.Enable(False)
		elif mode == MapCanvas.MODE_SELECT_STATION:
			self.tb.ToggleTool(MapChoice.TB_SELECT_STATION, True)
			for i in self.latminSpin, self.latmaxSpin, self.lonminSpin, self.lonmaxSpin, self.idField:
				i.Enable(True)
		elif mode == MapCanvas.MODE_SELECT_IDENT:
			self.tb.ToggleTool(MapChoice.TB_SELECT_IDENT, True)
			for i in self.latminSpin, self.latmaxSpin, self.lonminSpin, self.lonmaxSpin, self.idField:
				i.Enable(False)

	def filterChanged(self, what):
		if what == "stations":
			# Temporarily disable the changed events from the spins
			self.updating = True

			sel_latmin = self.model.filter.enqd("latmin")
			sel_latmax = self.model.filter.enqd("latmax")
			sel_lonmin = self.model.filter.enqd("lonmin")
			sel_lonmax = self.model.filter.enqd("lonmax")
			sel_id = self.model.filter.enqi("ana_id")

			if sel_latmin == None:
				self.latminSpin.SetValue('')
			elif self.latminSpin.GetValue() != str(sel_latmin):
				self.latminSpin.SetValue(str(sel_latmin))

			if sel_latmax == None:
				self.latmaxSpin.SetValue('')
			elif self.latmaxSpin.GetValue() != str(sel_latmax):
				self.latmaxSpin.SetValue(str(sel_latmax))

			if sel_lonmin == None:
				self.lonminSpin.SetValue('')
			elif self.lonminSpin.GetValue() != str(sel_lonmin):
				self.lonminSpin.SetValue(str(sel_lonmin))

			if sel_lonmax == None:
				self.lonmaxSpin.SetValue('')
			elif self.lonmaxSpin.GetValue() != str(sel_lonmax):
				self.lonmaxSpin.SetValue(str(sel_lonmax))

			if sel_id == None:
				self.idField.SetValue('')
			elif self.idField.GetValue() != str(sel_id):
				self.idField.SetValue(str(sel_id))

			if sel_latmin == None and sel_lonmin == None and \
			   sel_latmax == None and sel_lonmax == None and sel_id == None:
				self.resetButton.Disable()
			else:
				self.resetButton.Enable()

			# Re-enable the changed events from the spins
			self.updating = False

	def onClose(self, event):
		# Hide the window
		self.Hide()
		# Don't destroy the window
		event.Veto()
		# Notify parent that we've been closed
		self.parent.mapHasClosed()

	def onToolClick(self, event):
		#print "Tool", event.GetId()
		if event.GetId() == MapChoice.TB_ZOOM_IN:
			# Zoom in
			self.map.zoom(0.8)
		elif event.GetId() == MapChoice.TB_ZOOM_OUT:
			# Zoom out
			self.map.zoom(1.2)
		elif event.GetId() == MapChoice.TB_MOVE:
			# Pan mode
			self.map.setMode(MapCanvas.MODE_MOVE)
		elif event.GetId() == MapChoice.TB_SELECT_AREA:
			# Select area mode
			self.map.setMode(MapCanvas.MODE_SELECT_AREA)
		elif event.GetId() == MapChoice.TB_SELECT_STATION:
			# Select station mode
			self.map.setMode(MapCanvas.MODE_SELECT_STATION)
		elif event.GetId() == MapChoice.TB_SELECT_IDENT:
			# Select same ident mode
			self.map.setMode(MapCanvas.MODE_SELECT_IDENT)

	def updateFromSpins(self):
		"Delayed update of the filter from the value of the input fields"

		if self.updating: return

		try :
			if self.fieldUpdated == "area":
				# Select the area from the area fields
				self.model.setAreaFilter( \
					latmin = float(self.latminSpin.GetValue()), \
					latmax = float(self.latmaxSpin.GetValue()), \
					lonmin = float(self.lonminSpin.GetValue()), \
					lonmax = float(self.lonmaxSpin.GetValue())  \
					);
			else:
				# Select the station by ID
				self.model.setStationFilter(int(self.idField.GetValue()))
		except ValueError:
			# Do nothing in case there are incorrect numbers in the fields
			pass

	def onSpinChanged(self, event):
		"""
		When a field is edited, trigger a delayed update according to its contents.

		The reason for the delay is to trigger the update only when the
		user seems to have finished typing.
		"""
		if self.updating: return

		if event.GetEventObject() == self.idField:
			self.fieldUpdated = "station"
		else:
			self.fieldUpdated = "area"

		if self.timedUpdater == None:
			self.timedUpdater = wx.FutureCall(300, self.updateFromSpins)
		else:
			self.timedUpdater.Restart(300)

	def onResetButton(self, event):
		self.model.setStationFilter(None)
