import wx
from wxDballe.Model import ModelListener

class QueryChoice(wx.Choice, ModelListener):
	def __init__(self, parent, model, filterTag, dataTag):
		wx.Choice.__init__(self, parent)
		self.model = model
		self.model.registerUpdateListener(self)
		# tag to identify the model filter changes to which we react
		self.filterTag = filterTag
		# tag to identify the data updates to which we react
		self.dataTag = dataTag
		self.Bind(wx.EVT_CHOICE, self.selected, self)
		self.updating = False
		self.invalidate()

	def readFilterFromRecord(self, record):
		"""
		Read a record with query parameters and return its userdata
		equivalent for what concernes this field.
		"""
		return None

	def readOptions(self):
		"""
		Get an array of (label, data) tuples corresponding to the
		options for the Choice
		"""
		return []

	def selected(self, event):
		pass

	def filterChanged(self, what):
		if what == self.filterTag:
			self.updating = True
			sel = self.readFilterFromRecord(self.model.filter)
			for i in range(self.GetCount()):
				if self.GetClientData(i) == sel:
					self.Select(i)
					break
			self.updating = False

	def invalidate(self):
		self.Enable(False)

	def hasData(self, what):
		if what == self.dataTag:
			current = self.readFilterFromRecord(self.model.filter)
			active = self.readFilterFromRecord(self.model.activeFilter)
			options = self.readOptions()

			selected = 0
			count = 0
			self.Clear()
			for label, data in options:
				if data == active:
					label = label + " (*)"
				if current == data:
					selected = count
				self.Append(label, data)
				count = count + 1
			self.Select(selected)
			self.Enable(True)
