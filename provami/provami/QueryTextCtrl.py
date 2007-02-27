import wx
from provami.Model import ModelListener

class QueryTextCtrl(wx.TextCtrl, ModelListener):
	def __init__(self, parent, model, filterTag, updateFunc = None, isValid = None):
		wx.TextCtrl.__init__(self, parent)
		self.model = model
		self.model.registerUpdateListener(self)
		# tag to identify the model filter changes to which we react
		self.filterTag = filterTag
		self.updateFunc = updateFunc
		if isValid == None:
			self.isValid = lambda x: True
		else:
			self.isValid = isValid

		self.defaultBackground = self.GetBackgroundColour()
		self.invalidBackground = wx.Colour(0xff, 0xbb, 0xbb)
		self.dirtyBackground = wx.Colour(0xff, 0xff, 0xbb)

		self.Bind(wx.EVT_TEXT, self.onChanged)
		self.Bind(wx.EVT_CHAR, self.onChar)
		self.updating = False

	def updateColour(self):
		if not self.isValid(self.GetValue()):
			self.SetBackgroundColour(self.invalidBackground)
		elif self.readValue() != self.model.filter.enqc(self.filterTag):
			self.SetBackgroundColour(self.dirtyBackground)
		else:
			self.SetBackgroundColour(self.defaultBackground)

	def readValue(self):
		res = str(self.GetValue()).strip()
		if res == "": return None
		return res

	def onChanged(self, event):
		if self.updating: return
		self.updateColour()

	def onChar(self, event):
		c = event.GetKeyCode()
		if c == 13:
			self.activated(event)
		else:
			event.Skip()

	def activated(self, event):
		#print "Activated", self.GetValue()
		if self.updateFunc != None:
			self.updateFunc(self.readValue())

	def filterChanged(self, what):
		if what == self.filterTag:
			self.updating = True
			text = self.model.filter.enqc(self.filterTag)
			if text != None:
				self.SetValue(text)
			else:
				self.SetValue("")
			self.updateColour()
			self.updating = False

	def invalidate(self):
		self.Enable(False)

	def hasData(self, what):
		if what == "all":
			self.Enable(True)
