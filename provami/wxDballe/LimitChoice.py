import wx
from wxDballe.Model import ModelListener
from wxDballe.QueryButton import QueryButton

class LimitChoice(wx.Panel, ModelListener):
	def __init__(self, parent, model, id=-1):
		wx.Panel.__init__(self, parent, id)
		self.model = model
		self.model.registerUpdateListener(self)

		self.checkBox = wx.CheckBox(self, -1, "Limit results", style=wx.ALIGN_RIGHT)
		self.moreButton = wx.Button(self, -1, "More")
		self.lessButton = wx.Button(self, -1, "Less")
		self.queryButton = QueryButton(self, model)
		self.info = wx.StaticText(self, -1, "")

		self.Bind(wx.EVT_CHECKBOX, self.onCheckBox, self.checkBox)
		self.Bind(wx.EVT_BUTTON, self.onClick, self.lessButton)
		self.Bind(wx.EVT_BUTTON, self.onClick, self.moreButton)

		box = wx.BoxSizer(wx.HORIZONTAL)
		box.Add(self.info, 0, wx.ALIGN_CENTER)
		box.Add(wx.StaticText(self, -1, ""), 1)
		box.Add(self.checkBox, 0, wx.ALIGN_CENTER)
		box.Add(self.lessButton)
		box.Add(self.moreButton)
		box.Add(self.queryButton)

		self.SetSizerAndFit(box)

		self.invalidate()
		self.hasData("data")

	def onClick(self, event):
		button = event.GetEventObject()
		if button == self.lessButton:
			if self.model.resultsMax / 2 < self.model.lowerTruncateThreshold:
				self.model.setResultLimit(self.model.lowerTruncateThreshold)
			else:
				self.model.setResultLimit(self.model.resultsMax / 2)
		elif button == self.moreButton:
			self.model.setResultLimit(self.model.resultsMax * 2)
		self.model.update()

	def onCheckBox(self, event):
		cb = event.GetEventObject()
		if cb.GetValue():
			self.model.setResultLimit(self.model.resultsMax)
		else:
			self.model.setResultLimit(None)
		self.model.update()

	def invalidate(self):
		self.checkBox.Enable(False)
		self.lessButton.Enable(False)
		self.moreButton.Enable(False)

	def hasData(self, what):
		if what == "data":
			self.checkBox.SetValue(self.model.truncateResults)

			if self.model.resultsTruncated:
				self.info.SetLabel("%d+ results" % self.model.resultsCount)
				self.moreButton.Enable()
			else:
				self.info.SetLabel("%d results" % self.model.resultsCount)
				self.moreButton.Disable()

			if self.model.resultsCount < self.model.lowerTruncateThreshold:
				self.checkBox.Disable()
				self.lessButton.Disable()
				self.moreButton.Disable()
			else:
				self.checkBox.Enable()
				if self.model.resultsMax <= self.model.lowerTruncateThreshold:
					self.lessButton.Disable()
				else:
					self.lessButton.Enable()
