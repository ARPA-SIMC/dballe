import Numeric
import string
import wx
from wx.lib.floatcanvas import FloatCanvas, Resources

class MapChoice(wx.Panel):
	def __init__(self, parent, model):
		wx.Panel.__init__(self, parent)
		self.points = []
		self.userScale = 1.0
		self.mouseDown = False
		# 0: move
		# 1: select
		self.mode = 0

		self.model = model
		self.model.registerUpdateListener(self.update)

		self.canvas = FloatCanvas.FloatCanvas(self, Debug = 0)
		self.canvas.SetProjectionFun("FlatEarth")
		self.canvas.SetMode("Mouse")

		FloatCanvas.EVT_LEFT_DOWN(self.canvas, self.onMouseDown)
		FloatCanvas.EVT_LEFT_UP(self.canvas, self.onMouseUp)
		FloatCanvas.EVT_MOTION(self.canvas, self.onMove)
		#self.canvas.Enable()

		# Draw shore lines
		Shorelines = self.readMap()
		for segment in Shorelines:
			self.canvas.AddLine(segment)
		#bb = self.canvas.CalcBoundingBox()

		buttonPanel = wx.Panel(self)
		bpbox = wx.BoxSizer(wx.HORIZONTAL)
		self.smallButton = wx.Button(buttonPanel, -1, "Enlarge")
		self.Bind(wx.EVT_BUTTON, self.onSizeButton, self.smallButton)
		bpbox.Add(self.smallButton)
		self.largeButton = wx.Button(buttonPanel, -1, "Reduce")
		self.Bind(wx.EVT_BUTTON, self.onSizeButton, self.largeButton)
		bpbox.Add(self.largeButton)
		self.selectButton = wx.ToggleButton(buttonPanel, -1, "Select")
		self.Bind(wx.EVT_TOGGLEBUTTON, self.onToggleSelect, self.selectButton)
		bpbox.Add(self.selectButton)
		self.resetButton = wx.Button(buttonPanel, -1, "Reset filter")
		self.Bind(wx.EVT_BUTTON, self.onResetButton, self.resetButton)
		bpbox.Add(self.resetButton)
		buttonPanel.SetSizer(bpbox)

		box = wx.BoxSizer(wx.VERTICAL)
		box.Add(self.canvas, 1, wx.EXPAND)
		box.Add(buttonPanel)
		self.SetSizerAndFit(box)

		self.update()

	def update(self):
		self.userScale = 1.0
		self.largeButton.Enable()
		self.smallButton.Enable()

		if self.model.filter.enqd("latmin") == None and self.model.filter.enqd("lonmin") == None and \
		   self.model.filter.enqd("latmax") == None and self.model.filter.enqd("lonmax") == None:
			self.resetButton.Disable()
		else:
			self.resetButton.Enable()

		# Delete old set of stations
		self.canvas.RemoveObjects(self.points)

		# Draw new set of stations
		self.latmin = 1000.0
		self.lonmin = 1000.0
		self.latmax = -1000.0
		self.lonmax = -1000.0
		self.points = []
		for s in self.model.stations():
			lat = s.enqd("lat")
			if lat < self.latmin: self.latmin = lat
			if lat > self.latmax: self.latmax = lat
			lon = s.enqd("lon")
			if lon < self.lonmin: self.lonmin = lon
			if lon > self.lonmax: self.lonmax = lon
			p = FloatCanvas.Point((lon, lat), Color = "RED", Diameter = 3)
			self.points.append(p)
			self.canvas.AddObject(p)
			#self.canvas.AddPoint((s.enqd("lon"), s.enqd("lat")), Color = "RED", Diameter = 3)

		if self.lonmax - self.lonmin < 12:
			self.lonmax = self.lonmax + 6
			self.lonmin = self.lonmin - 6

		if self.latmax - self.latmin < 6:
			self.latmax = self.latmax + 3
			self.latmin = self.latmin - 3

		self.center = ((self.lonmax + self.lonmin) / 2, (self.latmax + self.latmin) / 2)

		self.rescale()

	def rescale(self):
		scale = min(360/(self.lonmax - self.lonmin), 180/(self.latmax - self.latmin))
		print "Scale: %f %f" % (scale, scale * self.userScale)
		self.canvas.Scale = scale * self.userScale
		self.canvas.Zoom(1, self.center)

	def onToggleSelect(self, event):
		if self.selectButton.GetValue():
			self.mode = 1
		else:
			self.mode = 0

	def onSizeButton(self, event):
		button = event.GetEventObject()
		oldUserScale = self.userScale
		if button == self.smallButton:
			self.largeButton.Enable()
			self.userScale = self.userScale * 1.5
		elif button == self.largeButton:
			self.smallButton.Enable()
			self.userScale = self.userScale / 1.5
		if oldUserScale != self.userScale:
			self.rescale()

	def onResetButton(self, event):
		self.model.setAreaFilter(None, None, None, None)

	def onMouseDown(self, event):
		self.mouseDown = True
		self.mouseDownCoords = tuple(event.GetCoords())
		self.mouseDownCenter = self.center
		if self.mode == 1:
			self.dragRectangle = self.canvas.AddRectangle(self.mouseDownCoords, (1, 1), LineWidth=1)
			self.canvas.Draw()
		print "MD"

	def onMouseUp(self, event):
		self.mouseDown = False
		if self.mode == 1:
			self.canvas.RemoveObject(self.dragRectangle)
			self.canvas.Draw()
			self.mode = 0
			self.selectButton.SetValue(False)
			latmin = min(event.GetCoords()[1], self.mouseDownCoords[1])
			latmax = max(event.GetCoords()[1], self.mouseDownCoords[1])
			lonmin = min(event.GetCoords()[0], self.mouseDownCoords[0])
			lonmax = max(event.GetCoords()[0], self.mouseDownCoords[0])
			print "Coords: lat %f-%f  lon %f-%f" % (latmin, latmax, lonmin, lonmax)
			self.model.setAreaFilter(latmin, latmax, lonmin, lonmax)
		print "MU"

	def onMove(self, event):
		if self.mouseDown:
			print "MDrag"
			coords = tuple(event.GetCoords()) 
			stride = (coords[0] - self.mouseDownCoords[0], coords[1] - self.mouseDownCoords[1])
			if self.mode == 0:
				self.center = (self.mouseDownCenter[0] - stride[0]/2, self.mouseDownCenter[1] - stride[1]/2)
				self.rescale()
			else:
				self.canvas.RemoveObject(self.dragRectangle)
				print "RECT ", self.mouseDownCoords, " ", stride
				self.dragRectangle = self.canvas.AddRectangle(self.mouseDownCoords, stride, LineWidth=1)
				self.canvas.Draw()

		#print event.GetCoords()
		#print "%.2f, %.2f" % tuple(event.GetCoords())

	def readMap(self):
		"""
		This function reads a MapGen Format file, and
		returns a list of NumPy arrays with the line segments in them.

		Each NumPy array in the list is an NX2 array of Python Floats.

		The demo should have come with a file, "world.dat" that is the
		shorelines of the whole world, in MapGen format.
		"""
		file = open('world.dat', 'rt')

		Shorelines = []
		segment = []
	    	for line in map(string.strip, file.readlines()):
			if line:
				if line == "# -b": #New segment beginning
					if segment: Shorelines.append(Numeric.array(segment))
					segment = []
				else:
					segment.append(map(float,string.split(line)))
		if segment: Shorelines.append(Numeric.array(segment))
		
		return Shorelines
