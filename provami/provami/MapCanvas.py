import string
import wx
from wxDballe.Model import Model, ModelListener, TTracer

class MapCanvasListener:
	def modeChanged (self, oldMode, mode):
		pass

class MapCanvas(wx.Window, ModelListener):
	class MapCanvasEvent(wx.PyCommandEvent):
		def __init__(self, eventType, id):
			wx.PyCommandEvent.__init__(self, eventType, id)
			self.modeChange = None
			self.station = None
		def SetStation(self, id): self.station = id
		def GetStation(self): return self.station
		def SetModeChange(self, oldmode, newmode):
			self.modeChange = (oldmode, newmode)
		def GetModeChange(self):
			if self.modeChange == None:
				return None, None
			else:
				return self.modeChange

	MODE_MOVE = 0
	MODE_SELECT_AREA = 1
	MODE_SELECT_STATION = 2
	MODE_SELECT_IDENT = 3

	# Create custom events
	wxEVT_MODE_CHANGED = wx.NewEventType()
	EVT_MODE_CHANGED = wx.PyEventBinder(wxEVT_MODE_CHANGED, 0)

	wxEVT_CURRENT_STATION_CHANGED = wx.NewEventType()
	EVT_CURRENT_STATION_CHANGED = wx.PyEventBinder(wxEVT_CURRENT_STATION_CHANGED, 0)

	def __init__(self, parent, model):
		wx.Window.__init__(self, parent, style=wx.FULL_REPAINT_ON_RESIZE)

		# Hook with the model
		self.model = model
		self.model.registerUpdateListener(self)

		# Initialize handling of listeners
		self.listeners = []

		# Initialize cached bitmaps
		self.shoreLines = []
		self.shoreLinesImage = None
		self.stationsImage = None

		# Initialize the map displacement (in lat/lon values)
		self.latcentre = 0
		self.loncentre = 0

		# Initialize bounding box of visible area (in lat/lon values)
		self.latwidth = 180.0
		self.lonwidth = 360.0

		# Initialize mouse tracking structures
		self.mouseDown = False

		# Interaction mode:
		# 0: move
		# 1: select area
		# 2: select station
		self.mode = None
		self.setMode(MapCanvas.MODE_MOVE)

		# Initialize focus management structures
		self.previousFocus = None

		# Currently selected station, if only one station is selected
		self.selectedID = None

		# Bind events
		self.Bind(wx.EVT_PAINT, self.OnPaint)
		self.Bind(wx.EVT_SIZE, self.OnResize)
		self.Bind(wx.EVT_MOTION, self.OnMouseMoved)
		self.Bind(wx.EVT_MOUSEWHEEL, self.OnMouseWheel)
		self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
		self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
		self.Bind(wx.EVT_CHAR, self.OnKeyChar)
		self.Bind(wx.EVT_ENTER_WINDOW, self.OnEnterWindow)
		self.Bind(wx.EVT_LEAVE_WINDOW, self.OnLeaveWindow)

		# Zoom the empty map to show the whole world
		self.showArea(-180, -90, 180, 90)
		# Schedule a zoom to fit all stations on the next model update
		self.neverZoomed = True
		#self.zoomToFit()

	def distanceToWorld (self, p):
		"Map distances from pixel units to world units"
		return (p[0] / self.xscale, -p[1] / self.yscale)

	def coordsToWorld (self, p):
		"Map coordinates from pixel space to world space"
		return ((p[0] - self.xoffset) / self.xscale - 180, -((p[1] - self.yoffset) / self.yscale) + 90)

	def distanceToPixels (self, p):
		"Map distances from world units to pixel units"
		return (p[0] * self.xscale, -p[1] * self.yscale)
	
	def coordsToPixels (self, p):
		"Map coordinates from world space to pixel space"
		return ((p[0] + 180) * self.xscale + self.xoffset, (-(p[1]-90)) * self.yscale + self.yoffset)

	def segmentToPixels (self, seg):
		"Convert a segment in world space to a segment in pixel space"
		res = []
		for i in seg:
			res.append(self.coordsToPixels(i))
		return res

	def zoom (self, factor):
		"""
		Zoom in or out according to the given factor.

		The resulting size of the displayed world is the previous size
		multiplied by the factor.
		
		For example, a factor of 1.2 zooms out 20%, while a factor of
		0.8 zooms in 20%.
		"""
		if factor == 1: return
		self.latwidth = self.latwidth * factor
		self.lonwidth = self.lonwidth * factor
		# Maintain the map magnification within reasonable values
		if self.latwidth < 1 or self.lonwidth < 1:
			self.latwidth, self.lonwidth = 1, 2
		if self.latwidth > 180 or self.lonwidth > 360:
			self.latwidth, self.lonwidth = 180, 360
		self.resize()
		self.Refresh()

	def showArea (self, lonmin, latmin, lonmax, latmax):
		"Zoom to fit the given area and recentre in the middle of it"
		self.latwidth = (latmax - latmin) * 1.2
		self.lonwidth = (lonmax - lonmin) * 1.2
		if self.latwidth < 1: self.latwidth = 1
		if self.lonwidth < 1: self.lonwidth = 1

		if self.latwidth * 2 > self.lonwidth:
			self.lonwidth = self.latwidth * 2
		else:
			self.latwidth = self.lonwidth / 2
		self.recentre((lonmax + lonmin) / 2, (latmax + latmin) / 2)
		self.resize()
		self.Refresh()

	def zoomToFit (self):
		"Zoom the image to fit all the stations"
		lonmin, latmin, lonmax, latmax = self.stationsBoundingBox()
		self.showArea(lonmin, latmin, lonmax, latmax)

	def zoomToFitSelection (self):
		"Zoom the image to fit all the stations"
		sel_id = self.model.filter.enqd("ana_id")
		latmin = self.model.filter.enqd("latmin")
		latmax = self.model.filter.enqd("latmax")
		lonmin = self.model.filter.enqd("lonmin")
		lonmax = self.model.filter.enqd("lonmax")

		if sel_id != None:
			id, lat, lon, ident = self.model.stationByID(sel_id)
			if id != None: self.showArea(lon - 2, lat - 2, lon + 2, lat + 2)
		elif latmin != None and lonmin != None and latmax != None and lonmax != None:
			self.showArea(lonmin, latmin, lonmax, latmax)

	def recentre (self, lon, lat):
		"Centre the map on the given coordinates"
		if self.loncentre == lon and self.latcentre == lat: return
		self.loncentre = lon
		self.latcentre = lat
		self.resize()
		self.Refresh()

	def centreOnSelection (self):
		sel_id = self.model.filter.enqi("ana_id")
		sel_latmin = self.model.filter.enqd("latmin")
		sel_latmax = self.model.filter.enqd("latmax")
		sel_lonmin = self.model.filter.enqd("lonmin")
		sel_lonmax = self.model.filter.enqd("lonmax")

		if sel_id != None:
			id, lat, lon, ident = self.model.stationByID(sel_id)
			if id != None: self.recentre(lon, lat)
		elif sel_latmin != None and sel_lonmin != None and sel_latmax != None and sel_lonmax != None:
			self.recentre((sel_lonmax + sel_lonmin) /2, (sel_latmax + sel_latmin) / 2)
		else:
			lonmin, latmin, lonmax, latmax = self.stationBoundingBox()
			self.recentre((lonmax + lonmin) /2, (latmax + latmin) / 2)

	def pan (self, xfac, yfac):
		"""
		Pan the image of an amount proportional to the given factors.

		For example, pan(0, -0.2) moves the image up of 20% its size.
		"""
		(width, height) = self.GetClientSizeTuple()
		worldStride = self.distanceToWorld((width * xfac, height * yfac))
		self.recentre(self.loncentre + worldStride[0], self.latcentre + worldStride[1])

	def panTowards (self, lon, lat, maxfactor = 0.1):
		dlon = lon - self.loncentre
		dlat = lat - self.latcentre
		if abs(dlon) > self.lonwidth * maxfactor:
			newlon = self.loncentre + self.lonwidth * maxfactor * dlon/abs(dlon)
		else:
			newlon = lon
		if abs(dlat) > self.latwidth * maxfactor:
			newlat = self.latcentre + self.latwidth * maxfactor * dlat/abs(dlat)
		else:
			newlat = lat
		self.recentre(newlon, newlat)
		#print "panTowards", lon, lat, "->", newlon, newlat

	def nearestPoint(self, plon, plat):
		mindist = 500000.0;
		cand = None
		for (id, lat, lon, ident) in self.model.stations():
			x = lon - plon
			y = lat - plat
			dist = x*x + y*y
			if mindist > dist:
				mindist = dist
				cand = (id, lat, lon, ident)
		return cand

	def selectNearest(self, lon, lat):
		id, lat, lon, ident = self.nearestPoint(lon, lat)
		self.model.setStationFilter(id)
		return id, lat, lon, ident

	def selectArea(self, lon1, lat1, lon2, lat2):
		#print "selectarea", lon1, lat1, lon2, lat2
		#print "filter", min(lat1, lat2), max(lat1, lat2), min(lon1, lon2), max(lon1, lon2)
		self.model.setAreaFilter(min(lat1, lat2), max(lat1, lat2), min(lon1, lon2), max(lon1, lon2))

	def getCurrentStation(self):
		"""
		Return the ID of the currently selected station.

		If there are no stations currently selected, or if there is
		more than one station currently selected, returns None
		"""
		return self.selectedID

	def getMode(self):
		"Return the current interaction mode"
		return self.mode

	def setMode(self, mode):
		if mode == self.mode: return

		oldMode = self.mode
		self.mode = mode

		if self.mode == MapCanvas.MODE_MOVE:
			# Pan mode
			self.SetCursor(wx.StockCursor(wx.CURSOR_SIZENESW))
		elif self.mode == MapCanvas.MODE_SELECT_AREA:
			# Select area mode
			self.SetCursor(wx.StockCursor(wx.CURSOR_CROSS))
		elif self.mode == MapCanvas.MODE_SELECT_STATION:
			# Select station mode
			self.SetCursor(wx.StockCursor(wx.CURSOR_HAND))
		elif self.mode == MapCanvas.MODE_SELECT_IDENT:
			# Select ident mode
			self.SetCursor(wx.StockCursor(wx.CURSOR_HAND))

		# Notify mode change to listeners
		e = MapCanvas.MapCanvasEvent(MapCanvas.wxEVT_MODE_CHANGED, self.GetId())
		e.SetEventObject(self)
		e.SetModeChange(oldMode, self.mode)
		self.GetEventHandler().ProcessEvent(e)


	def stationsBoundingBox (self):
		latmin = 360
		lonmin = 360
		latmax = -360
		lonmax = -360
		for (id, lat, lon, ident) in self.model.stations():
			if lat < latmin: latmin = lat
			if lat > latmax: latmax = lat
			if lon < lonmin: lonmin = lon
			if lon > lonmax: lonmax = lon
		return (lonmin, latmin, lonmax, latmax)

	def OnEnterWindow (self, event):
		self.previousFocus = wx.Window_FindFocus()
		self.SetFocus()

	def OnLeaveWindow (self, event):
		if self.previousFocus != None:
			self.previousFocus.SetFocus()
			self.previousFocus = None

	def OnKeyChar (self, event):
		"Handle panning and zooming in response to key events"
		c = event.GetKeyCode()
		if c == ord('+'):
			# Zoom in
			self.zoom(0.8)
		elif c == ord('-'):
			# Zoom out
			self.zoom(1.2)
		elif c == ord('c'):
			# Centre on selection
			self.centreOnSelection()
		elif c == ord('Z'):
			# Zoom to fit stations
			self.zoomToFit()
		elif c == ord('z'):
			# Zoom to fit selection
			self.zoomToFitSelection()
		elif c == wx.WXK_F1:
			# Switch to move mode
			self.setMode(MapCanvas.MODE_MOVE)
		elif c == wx.WXK_F2:
			# Switch to select area mode
			self.setMode(MapCanvas.MODE_SELECT_AREA)
		elif c == wx.WXK_F3:
			# Switch to select station mode
			self.setMode(MapCanvas.MODE_SELECT_STATION)
		elif c == wx.WXK_F4:
			# Switch to select ident mode
			self.setMode(MapCanvas.MODE_SELECT_IDENT)
		elif c == wx.WXK_UP:
			# Pan up
			self.pan(0, -0.1)
		elif c == wx.WXK_DOWN:
			# Pan down
			self.pan(0, 0.1)
		elif c == wx.WXK_LEFT:
			# Pan left
			self.pan(-0.1, 0)
		elif c == wx.WXK_RIGHT:
			# Pan right
			self.pan(0.1, 0)

	def OnMouseWheel (self, event):
		"Handle zooming in response to mouse wheel events"
		fac = event.GetWheelRotation() / event.GetWheelDelta()

		# Pan towards the wheel point
		# (disabled because it was confusing)
		#wplon, wplat = self.coordsToWorld(tuple(event.GetPosition()))
		#self.panTowards(wplon, wplat, 0.05)
		
		#self.loncentre, self.latcentre = 
		if fac > 0:
			self.zoom(0.8 / abs(fac))
		elif fac < 0:
			self.zoom(1.2 * abs(fac))

	def OnMouseDown (self, event):
		self.mouseDown = True
		self.mouseDownCoords = tuple(event.GetPosition())
		self.mouseDownCentre = (self.loncentre, self.latcentre)

		lon, lat = self.coordsToWorld(event.GetPosition())

		if self.mode == MapCanvas.MODE_SELECT_AREA:
			self.selectArea(lon, lat, lon, lat)
			pass
		elif self.mode == MapCanvas.MODE_SELECT_STATION:
			lon, lat = self.coordsToWorld(event.GetPosition())
			self.selectNearest(lon, lat)
		elif self.mode == MapCanvas.MODE_SELECT_IDENT:
			lon, lat = self.coordsToWorld(event.GetPosition())
			id, lat, lon, ident = self.nearestPoint(lon, lat)
			self.model.setIdentFilter(ident != None, ident)

	def OnMouseUp (self, event):
		self.mouseDown = False
		if self.mode == MapCanvas.MODE_SELECT_AREA:
			lon1, lat1 = self.coordsToWorld(self.mouseDownCoords)
			lon2, lat2 = self.coordsToWorld(event.GetPosition())
			self.selectArea(lon1, lat1, lon2, lat2)
		#	#self.canvas.RemoveObject(self.selectRectangle)
		#	#self.canvas.Draw()
		#	#self.mode = 0
		#	#self.selectButton.SetValue(False)
		#	coords = tuple(event.GetCoords()) 
		#	stride = (coords[0] - self.mouseDownCoords[0], coords[1] - self.mouseDownCoords[1])
		#	self.select(self.mouseDownCoords, stride)
		elif self.mode == MapCanvas.MODE_SELECT_STATION:
			lon, lat = self.coordsToWorld(event.GetPosition())
			id, lat, lon, ident = self.selectNearest(lon, lat)
			if id != None: self.recentre(lon, lat)
		elif self.mode == MapCanvas.MODE_SELECT_IDENT:
			lon, lat = self.coordsToWorld(event.GetPosition())
			id, lat, lon, ident = self.nearestPoint(lon, lat)
			self.model.setIdentFilter(ident != None, ident)

	def OnMouseMoved (self, event):
		#print "Move: ", event.GetPosition()
		if self.mouseDown:
			coords = tuple(event.GetPosition()) 
			stride = (coords[0] - self.mouseDownCoords[0], coords[1] - self.mouseDownCoords[1])
			worldStride = self.distanceToWorld(stride)
			if self.mode == MapCanvas.MODE_MOVE:
				self.recentre( \
					self.mouseDownCentre[0] - worldStride[0], \
					self.mouseDownCentre[1] - worldStride[1])
			elif self.mode == MapCanvas.MODE_SELECT_AREA:
				lon1, lat1 = self.coordsToWorld(self.mouseDownCoords)
				lon2, lat2 = self.coordsToWorld(event.GetPosition())
				self.selectArea(lon1, lat1, lon2, lat2)
			elif self.mode == MapCanvas.MODE_SELECT_STATION:
				lon, lat = self.coordsToWorld(event.GetPosition())
				id, lat, lon, ident = self.selectNearest(lon, lat)
			elif self.mode == MapCanvas.MODE_SELECT_IDENT:
				lon, lat = self.coordsToWorld(event.GetPosition())
				id, lat, lon, ident = self.nearestPoint(lon, lat)
				self.model.setIdentFilter(ident != None, ident)

	def OnResize (self, event):
		self.resize()
	
	def resize (self):
		(width, height) = self.GetClientSizeTuple()
		owidth, oheight = (width, height)
		if width > height*2:
			width = height*2
		else:
			height = width/2
		self.xscale = float(width)/self.lonwidth
		self.yscale = float(height)/self.latwidth
		self.xoffset = 0
		self.yoffset = 0
		self.xoffset, self.yoffset = self.coordsToPixels((self.loncentre - self.lonwidth / 2, self.latcentre + self.latwidth / 2))
		self.xoffset = - self.xoffset + (owidth - width)/2
		self.yoffset = - self.yoffset + (oheight - height)/2
		# Invalidate the pre-rendered shorelines image when the size
		# changes
		self.shoreLinesImage = None
		self.stationsImage = None

	def OnPaint (self, event):
		if self.stationsImage == None:
			self.renderStations()
		wx.BufferedPaintDC(self, self.stationsImage, wx.BUFFER_VIRTUAL_AREA)

	def filterChanged(self, what):
		if what == "stations":
			self.stationsImage = None
			self.Refresh()

	def invalidate(self):
		self.stationsImage = None

	def hasData(self, what):
		if what == "stations":
			if self.neverZoomed:
				self.zoomToFit()
				self.neverZoomed = False
			else:
				self.zoomToFitSelection()
			self.renderStations()

	def renderStations (self):
		tracer = TTracer("regenerate station map")
		if self.shoreLinesImage == None:
			self.renderShoreLines()

		sel_id = self.model.filter.enqi("ana_id")
		sel_mobile = self.model.filter.enqc("mobile")
		sel_ident = self.model.filter.enqc("ident")
		sel_latmin = self.model.filter.enqd("latmin")
		sel_latmax = self.model.filter.enqd("latmax")
		sel_lonmin = self.model.filter.enqd("lonmin")
		sel_lonmax = self.model.filter.enqd("lonmax")

		has_id = sel_id != None
		has_area = sel_latmin != None and sel_latmax != None and \
			     sel_lonmin != None and sel_lonmax != None

		(width, height) = self.GetClientSizeTuple()
		self.stationsImage = wx.EmptyBitmap(width, height)

		# Use the coastlines as background
		dc = wx.BufferedDC(None, self.stationsImage)
		coasts = wx.MemoryDC()
		coasts.SelectObject(self.shoreLinesImage)

		dc.BeginDrawing()
		dc.Blit(0, 0, width, height, coasts, 0, 0)

		normalPen = wx.Pen("#FF0000")
		selectedPen = wx.Pen("#00FF00")
		normalBrush = wx.Brush("#FFBBBB")
		selectedBrush = wx.Brush("#BBFFBB")

		# Add the points
		countSelected = 0
		lastSelectedID = None
		for (id, lat, lon, ident) in self.model.stations():
			selected = False
			if has_id and id == sel_id:
				# Try selecting by ID
				selected = True
			elif (sel_latmin != None and lat >= sel_latmin) and \
			     (sel_latmax != None and lat <= sel_latmax) and \
			     (sel_lonmin != None and lon >= sel_lonmin) and \
			     (sel_lonmax != None and lon <= sel_lonmax):
				# Try selecting by area, intersected with ident
				selected = sel_mobile == None or sel_ident == ident
			else:
				# Try selecting by ident
				selected = not has_area and sel_mobile != None and sel_ident == ident

			# Set the brush according to selection state
			if selected:
				dc.SetPen(selectedPen)
				dc.SetBrush(selectedBrush)
				countSelected = countSelected + 1
				lastSelectedID = id
			else:
				dc.SetPen(normalPen)
				dc.SetBrush(normalBrush)

			x, y = self.coordsToPixels((lon, lat))
			if ident == None:
				dc.DrawCircle(x, y, 2)
			else:
				# Draw a triangle
				dc.DrawPolygon( ( (x-3, y+3), (x, y-3), (x+3, y+3) ) )
				# Draw a cross
				#dc.DrawLine(x-1, y, x+1, y)
				#dc.DrawLine(x, y-1, x, y+1)

		if sel_latmin != None and sel_lonmin != None and sel_latmax != None and sel_lonmax != None:
			dc.SetPen(wx.Pen("BLACK"))
			dc.SetBrush(wx.Brush("WHITE", wx.TRANSPARENT))
			x, y = self.coordsToPixels((sel_lonmin, sel_latmin))
			w, h = self.distanceToPixels((sel_lonmax - sel_lonmin, sel_latmax - sel_latmin))
			dc.DrawRectangle(x, y, w, h)

		dc.EndDrawing()

		oldSID = self.selectedID
		if countSelected == 1:
			self.selectedID = lastSelectedID
		else:
			self.selectedID = None
		if self.selectedID != oldSID:
			# Notify change of currently selected station to listeners
			e = MapCanvas.MapCanvasEvent(MapCanvas.wxEVT_CURRENT_STATION_CHANGED, self.GetId())
			e.SetEventObject(self)
			e.SetStation(self.selectedID)
			self.GetEventHandler().ProcessEvent(e)

	def renderShoreLines (self):
		tracer = TTracer("regenerate shore lines")
		(width, height) = self.GetClientSizeTuple()
		#print "Repaint", width, height
		self.shoreLinesImage = wx.EmptyBitmap(width, height)
		dc = wx.BufferedDC(None, self.shoreLinesImage)
		#dc.SetBackground(wx.Brush(self.GetBackgroundColour()))
		dc.SetBackground(wx.Brush("WHITE"))
		dc.Clear()
		dc.BeginDrawing()
		dc.SetPen(wx.Pen("BLACK"))
		for i in self.shoreLines:
			dc.DrawLines(self.segmentToPixels(i))
		#dc.DrawRectangle(0, 0, width, height)
		#dc.DrawLine(0, 0, width, height)
		dc.EndDrawing()
		#wx.BufferedPaintDC(self, buffer, wx.BUFFER_VIRTUAL_AREA)

	def loadShoreLines (self, file):
		"""
		This function reads a MapGen Format file, and builds the
		self.shoreLines array with it.

		The self.shoreLines array is a list of floats with the line
		segments in them.
		"""
		file = open(file, 'rt')

		segment = []
	    	for line in map(string.strip, file.readlines()):
			if line:
				if line == "# -b": #New segment beginning
					if segment: self.shoreLines.append(segment)
					segment = []
				else:
					segment.append(map(float,string.split(line)))
		if segment: self.shoreLines.append(segment)

		self.shoreLinesImage = None
