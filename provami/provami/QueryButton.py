import wx
from provami.Model import ModelListener

class QueryButton(wx.Button, ModelListener):
	def __init__(self, parent, model, id=-1):
		wx.Button.__init__(self, parent, id, "Query")
		self.model = model
		self.model.registerUpdateListener(self)

		self.Enable(model.filterDirty)

		self.Bind(wx.EVT_BUTTON, self.onClick)

	def filterDirty(self, isDirty):
		self.Enable(isDirty)

	def hasData(self, what):
		self.Enable(self.model.filterDirty)

	def onClick(self, event):
		self.Enable(False)
		self.model.update()
