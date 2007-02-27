import string
import wx
import datetime
from wxDballe.Model import Model, ModelListener, TTracer

def dtToMinutes(datetime):
	days = datetime.date().toordinal()
	time = datetime.time()
	return days * (24*60) + time.hour * 60 + time.minute

def minutesToDT(minutes):
	date = datetime.date.fromordinal(minutes / (24*60))
	time = datetime.time((minutes % (24*60)) / 60, minutes % 60)
	return datetime.datetime.combine(date, time)

class DateTimeCanvas(wx.Window, ModelListener):
	def __init__(self, parent, model):
		wx.Window.__init__(self, parent, style=wx.FULL_REPAINT_ON_RESIZE)

		# Hook with the model
		self.model = model
		self.model.registerUpdateListener(self)

		# Initialize mouse tracking structures
		self.mouseDown = False

		# Bind events
		self.Bind(wx.EVT_PAINT, self.OnPaint)
		self.Bind(wx.EVT_SIZE, self.OnResize)
		self.Bind(wx.EVT_MOTION, self.OnMouseMoved)
		#self.Bind(wx.EVT_MOUSEWHEEL, self.OnMouseWheel)
		self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
		self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)

		# Zoom to fit all the stations
		#self.zoomToFit()

	def timeToOffset (self, datetime):
		"Map a time to the corresponding offset in the widget"
		if datetime == None: return None
		mintime, maxtime = self.model.daterange()
		width, height = self.GetClientSizeTuple()
		if datetime < mintime:
			return 0
		if datetime >= maxtime:
			return width - 1
		min = dtToMinutes(mintime)
		max = dtToMinutes(maxtime)
		cur = dtToMinutes(datetime)
		return (cur - min) * width / (max - min)

	def offsetToTime (self, offset):
		"Map an offset in the widget to the corresponding time"
		if offset == None: return None
		mintime, maxtime = self.model.daterange()
		width, height = self.GetClientSizeTuple()
		if offset < 0:
			return mintime
		if offset >= width:
			return maxtime
		min = dtToMinutes(mintime)
		max = dtToMinutes(maxtime)
		return minutesToDT(min + offset * (max - min) / width)

	def selectPixelRange (self, offset1, offset2):
		sel_min = min(offset1, offset2)
		sel_max = max(offset1, offset2)
		self.model.setMinDateTimeFilter(self.offsetToTime(sel_min))
		self.model.setMaxDateTimeFilter(self.offsetToTime(sel_max))

	def OnMouseDown (self, event):
		self.mouseDown = True
		x, y = event.GetPosition()
		self.mouseDownOffset = x

		self.selectPixelRange(self.mouseDownOffset, x)

	def OnMouseUp (self, event):
		self.mouseDown = False
		x, y = event.GetPosition()
		self.selectPixelRange(self.mouseDownOffset, x)

	def OnMouseMoved (self, event):
		if self.mouseDown:
			x, y = event.GetPosition()
			self.selectPixelRange(self.mouseDownOffset, x)

	def OnResize (self, event):
		self.resize()
	
	def resize (self):
		# Invalidate the pre-rendered background image when the size
		# changes
		self.backgroundImage = None
		self.Refresh()

	def OnPaint (self, event):
		if self.backgroundImage == None:
			self.renderBackgroundImage()
		wx.BufferedPaintDC(self, self.backgroundImage, wx.BUFFER_VIRTUAL_AREA)

	def filterChanged(self, what):
		if what == "datetime":
			self.backgroundImage = None
			self.Refresh()

	def invalidate(self):
		self.backgroundImage = None
		self.Refresh()

	def hasData(self, what):
		if what == "dtimes":
			self.backgroundImage = None
			self.Refresh()

	def renderBackgroundImage (self):
		tracer = TTracer("regenerate background image")

		# Datetime extremes
		mintime, maxtime = self.model.daterange()

		# Widget size
		width, height = self.GetClientSizeTuple()

		# Selection extremes
		sel_mintime = self.model.getMinDateTimeFilter()
		sel_maxtime = self.model.getMaxDateTimeFilter()

		# Selection extremes as offsets
		minsel = self.timeToOffset(sel_mintime)
		maxsel = self.timeToOffset(sel_maxtime)
		# If there is a partial selection, make it open ended
		if minsel != None or maxsel != None:
			if minsel == None: minsel = 0
			if maxsel == None: maxsel = width - 1
		else:
			minsel = width
			maxsel = -1

		# Compute histogram
		hist = [0] * width
		maxCount = 0
		for dt in self.model.datetimes():
			x = self.timeToOffset(dt)
			hist[x] = hist[x] + 1
			if hist[x] > maxCount:
				maxCount = hist[x]

		self.backgroundImage = wx.EmptyBitmap(width, height)
		dc = wx.BufferedDC(None, self.backgroundImage)

		dc.BeginDrawing()

		dc.SetBackground(wx.Brush("WHITE"))
		dc.Clear()

		normalPen = wx.Pen("#FF0000")
		selectedPen = wx.Pen("#00FF00")

		# Draw the histogram
		for i in range(width):
			if i >= minsel and i <= maxsel:
				dc.SetPen(selectedPen)
			else:
				dc.SetPen(normalPen)
			dc.DrawLine(i, height, i, height - (hist[i] * height / maxCount))

		dc.EndDrawing()
