import re
import wx
import wx.calendar
import datetime
from provami.Model import *

#def datetime_from_controls(wxdate, wxtime):
#	date = wxdate.GetValue()
#	year = date.GetYear()
#	month = date.GetMonth() + 1
#	day = date.GetDay()
#	date = wxtime.GetValue(as_wxDateTime=True)
#	hour = date.GetHour()
#	min = date.GetMinute()
#	sec = date.GetSecond()
#	print year, month, day, hour, min, sec
#	return datetime.datetime(year, month, day, hour, min, sec)

def int_or_none(x):
    try:
        return int(x)
    except:
        return None

class DateChoice(wx.TextCtrl):
	def __init__(self, parent, model, type=DateUtils.EXACT):
		wx.TextCtrl.__init__(self, parent)

		self.model = model
		self.model.registerUpdateListener(self)

		# Prevent change hooks to take place when we are updating after
		# a model change
		self.updating = False

		self.min = None
		self.max = None
		self.date = None
		self.tip = wx.ToolTip("")
		self.SetToolTip(self.tip)

		# Time value to use 
		self.type = type

		# Matchers for all the allowed date formats
		self.emptyMatch = re.compile(r'^\s*$')
		self.dateMatch = [
			re.compile(r'^\s*(?P<year>\d{4})(?:[/-](?P<month>\d{1,2})(?:[/-](?P<day>\d{1,2})(?:\s+(?P<hour>\d{1,2})(?::(?P<min>\d{1,2})(?::(?P<sec>\d{1,2}))?)?)?)?)?\s*$'),
			re.compile(r'^\s*(?:(?:(?P<day>\d{1,2})[/-])?(?P<month>\d{1,2})[/-])?(?P<year>\d{4})(?:\s+(?P<hour>\d{1,2})(?::(?P<min>\d{1,2})(?::(?P<sec>\d{1,2}))?)?)?\s*$')
		]

		self.defaultBackground = self.GetBackgroundColour()
		self.invalidBackground = wx.Colour(0xff, 0xbb, 0xbb)

		self.Bind(wx.EVT_TEXT, self.changed)

		self.invalidate()
		self.hasData("dtimes")
		self.filterChanged("datetime")

	def isValid(self, str):
		"""
		Check if the date entered is a valid date in the accepted range
		
		Returns a 2-tuple: a boolean that is true if the string is
		valid or false if it is not valid, and an array with the 6
		parsed values (some of which may be None) with the parsed date
		(or is None if the string is not a valid date).
		"""
		# Se if it's empty
		if self.emptyMatch.match(str):
			return True, None

		# Try out all the matchers
		m = None
		for matcher in self.dateMatch:
			m = matcher.match(str)
			if m is not None:
				break

		# If none succeeds, return false
		if m is None: return False, None

		# Get the values out of the group
		values = [int_or_none(m.group(x)) for x in ('year', 'month', 'day', 'hour', 'min', 'sec')]

		# If the year is empty, we can return a valid, empty date
		if values[0] is None: return True, None

		# If there is a partial date matching bits of minimum and maximum values, use those for completion
		if self.min is None or self.max is None:
			vmin = [0, 0, 0, 0, 0, 0]
		else:
			vmin = [self.min.year, self.min.month, self.min.day, self.min.hour, self.min.minute, self.min.second]
		if self.max is None:
			vmax = [9999, 12, 31, 24, 59, 59]
		else:
			vmax = [self.max.year, self.max.month, self.max.day, self.max.hour, self.max.minute, self.max.second]
		vvalues = [x for x in values]
		if self.type == DateUtils.MAX:
			for i in range(1,6):
				if vvalues[i - 1] != vmax[i - 1]: break
				if vvalues[i] is None: vvalues[i] = vmax[i]
			for i in range(1,6):
				if vvalues[i - 1] != vmin[i - 1]: break
				if vvalues[i] is None: vvalues[i] = vmin[i]
		else:
			for i in range(1,6):
				if vvalues[i - 1] != vmin[i - 1]: break
				if vvalues[i] is None: vvalues[i] = vmin[i]
			for i in range(1,6):
				if vvalues[i - 1] != vmax[i - 1]: break
				if vvalues[i] is None: vvalues[i] = vmax[i]

		try:
			dt = completeDate(vvalues, self.type)
		except ValueError:
			return False, None

		if dt is None:
			return True, None
		if (self.min is None or dt >= self.min) and (self.max is None or dt <= self.max):
			return True, values
		return False, values

	def changed(self, event):
		if self.updating: return
		valid, values = self.isValid(self.GetValue())
		self.updating = True
		if valid:
			self.SetBackgroundColour(self.defaultBackground)
			if values is not None:
				self.model.setDateTimeFilter(values[0], values[1], values[2], values[3], values[4], values[5], filter=self.type)
			else:
				self.model.setDateTimeFilter(None)
		else:
			self.SetBackgroundColour(self.invalidBackground)
			self.model.setDateTimeFilter(None, filter = self.type)
		self.updating = False

	def filterChanged(self, what):
		if self.updating: return
		if what == "datetime":
			self.updating = True
			year, month, day, hour, min, sec = self.model.getDateTimeFilter(self.type)
			if year is None:
				self.SetValue('')
			elif month is None:
				self.SetValue("%04d" % (year))
			elif day is None:
				self.SetValue("%04d-%02d" % (year, month))
			elif hour is None:
				self.SetValue("%04d-%02d-%02d" % (year, month, day))
			elif min is None:
				self.SetValue("%04d-%02d-%02d %02d" % (year, month, day, hour))
			elif sec is None:
				self.SetValue("%04d-%02d-%02d %02d:%02d" % (year, month, day, hour, min))
			else:
				self.SetValue("%04d-%02d-%02d %02d:%02d:%02d" % (year, month, day, hour, min, sec))
			self.updating = False

	def hasData(self, what):
		if what == "dtimes":
			self.min, self.max = self.model.daterange()
			self.tip.SetTip("Minimum value: " + str(self.min) + "\nMaximum value: " + str(self.max))
			self.filterChanged("datetime")
			self.changed(None)

class MinDateChoice(DateChoice, ModelListener):
	def __init__(self, parent, model):
		DateChoice.__init__(self, parent, model, type=DateUtils.MIN)

class MaxDateChoice(DateChoice, ModelListener):
	def __init__(self, parent, model):
		DateChoice.__init__(self, parent, model, type=DateUtils.MAX)

#class DateChoice(wx.Panel):
#	def __init__(self, parent, model, id=-1):
#		wx.Panel.__init__(self, parent, id)
#
#		self.model = model
#		self.model.registerUpdateListener(self)
#
#		# Prevent change hooks to take place when we are updating after
#		# a model change
#		self.updating = False
#
#		self.activate = wx.CheckBox(self, style=wx.ALIGN_RIGHT)
#		self.date = wx.DatePickerCtrl(self, style=wx.DP_DROPDOWN | wx.DP_SHOWCENTURY | wx.calendar.CAL_SHOW_SURROUNDING_WEEKS )
#		self.timespin = wx.SpinButton(self, style=wx.SP_VERTICAL)
#		self.time = wx.lib.masked.TimeCtrl(self, fmt24hr=True, spinButton = self.timespin)
#		        
#		sizer = wx.BoxSizer(wx.HORIZONTAL)
#		sizer.Add(self.activate)
#		sizer.Add(self.date)
#		sizer.Add(self.time)
#		sizer.Add(self.timespin)
#
#		self.SetSizerAndFit(sizer)
#
#		self.activate.SetValue(False)
#		self.date.Enable(False)
#		self.time.Enable(False)
#
#		self.Bind(wx.EVT_CHECKBOX, self.changed, self.activate)
#		self.Bind(wx.EVT_DATE_CHANGED, self.changed, self.date)
#		self.Bind(wx.lib.masked.EVT_TIMEUPDATE, self.changed, self.time)
#
#		self.invalidate()
#		self.hasData("dtimes")
#		self.filterChanged("datetime")
#
#	def fromDateTime(self, dt, enable = False):
#		if dt is None:
#			self.activate.SetValue(False)
#			self.date.Enable(False)
#			self.time.Enable(False)
#		else:
#			date = wx.DateTime()
#			date.Set(dt.day, dt.month - 1, dt.year, dt.hour, dt.minute, dt.second)
#			self.date.SetValue(date)
#			self.time.SetValue(date)
#			if enable:
#				self.activate.SetValue(True)
#				self.date.Enable(True)
#				self.time.Enable(True)
#
#	def changed(self, event):
#		if self.updating: return
#		if self.activate.GetValue():
#			self.date.Enable(True)
#			self.time.Enable(True)
#		else:
#			self.date.Enable(False)
#			self.time.Enable(False)
#
#	def hasData(self, what):
#		if what == "dtimes":
#			(tmin, tmax) = self.model.daterange()
#			ll = wx.DateTime()
#			ll.Set(tmin.day, tmin.month, tmin.year, tmin.hour, tmin.minute, tmin.second, tmin.microsecond/1000)
#			ul = wx.DateTime()
#			ul.Set(tmax.day, tmax.month, tmax.year, tmax.hour, tmax.minute, tmax.second, tmax.microsecond/1000)
#			self.date.SetRange(ll, ul)
#
#class MinDateChoice(DateChoice, ModelListener):
#	def __init__(self, parent, model):
#		DateChoice.__init__(self, parent, model)
#
#	def filterChanged(self, what):
#		if what == "datetime":
#			self.updating = True
#			self.fromDateTime(self.model.getMinDateTimeFilter(), True)
#			self.updating = False
#
#	def changed(self, event):
#		if self.updating: return
#		DateChoice.changed(self, event)
#
#		if self.activate.GetValue():
#			date = datetime_from_controls(self.date, self.time)
#			self.model.setMinDateTimeFilter(date)
#		else:
#			self.model.setMinDateTimeFilter(None)
#
#class MaxDateChoice(DateChoice, ModelListener):
#	def __init__(self, parent, model):
#		DateChoice.__init__(self, parent, model)
#
#	def filterChanged(self, what):
#		if what == "datetime":
#			self.updating = True
#			self.fromDateTime(self.model.getMaxDateTimeFilter(), True)
#			self.updating = False
#
#	def changed(self, event):
#		if self.updating: return
#		DateChoice.changed(self, event)
#
#		if self.activate.GetValue():
#			date = datetime_from_controls(self.date, self.time)
#			self.model.setMaxDateTimeFilter(date)
#		else:
#			self.model.setMaxDateTimeFilter(None)
