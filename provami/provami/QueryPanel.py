import wx
from provami.LevelsChoice import LevelsChoice
from provami.ReportChoice import ReportChoice
from provami.TimeRangeChoice import TimeRangeChoice
from provami.VarNamesChoice import VarNamesChoice
from provami.DateChoice import MinDateChoice, MaxDateChoice

class QueryPanel(wx.Panel):
	def __init__(self, parent, model, id=-1):
		wx.Panel.__init__(self, parent, id)
		#wx.Panel.__init__(self, parent, id, wx.TAB_TRAVERSAL)
		self.model = model

		gbs = wx.GridBagSizer()
		gbs.Add(wx.StaticText(self, -1, "Variable: "), (0, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(VarNamesChoice(self, self.model), (0, 1), (1, 1), flag=wx.EXPAND)

		gbs.Add(wx.StaticText(self, -1, "Level: "), (1, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(LevelsChoice(self, self.model), (1, 1), (1, 1), flag=wx.EXPAND)

		gbs.Add(wx.StaticText(self, -1, "Time range: "), (2, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(TimeRangeChoice(self, self.model), (2, 1), (1, 1), flag=wx.EXPAND)

		gbs.Add(wx.StaticText(self, -1, "Report: "), (3, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(ReportChoice(self, self.model), (3, 1), (1, 1), flag=wx.EXPAND)

		timePanel = wx.Panel(self)
		box = wx.BoxSizer(wx.HORIZONTAL)
		box.Add(wx.StaticText(timePanel, -1, "min: "), 0, flag = wx.ALIGN_CENTER_VERTICAL)
		box.Add(MinDateChoice(timePanel, model), 1, flag=wx.EXPAND)
		box.Add(wx.StaticText(timePanel, -1, " max: "), 0, flag = wx.ALIGN_CENTER_VERTICAL)
		box.Add(MaxDateChoice(timePanel, model), 1, flag=wx.EXPAND)
		timePanel.SetSizer(box)

		gbs.Add(wx.StaticText(self, -1, "Time: "), (4, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
		gbs.Add(timePanel, (4, 1), (1, 1), flag=wx.EXPAND)

#		gbs.Add(wx.StaticText(self, -1, "Min time: "), (4, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
#		gbs.Add(MinDateChoice(self, self.model), (4, 1), (1, 1), flag=wx.EXPAND)
#
#		gbs.Add(wx.StaticText(self, -1, "Max time: "), (5, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
#		gbs.Add(MaxDateChoice(self, self.model), (5, 1), (1, 1), flag=wx.EXPAND)

#		gbs.Add(wx.StaticText(self, -1, "Timeline: "), (6, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
#		gbs.Add(DateCanvas(self, self.model), (6, 1), (1, 1), flag=wx.EXPAND)

		gbs.AddGrowableCol(1)

		self.SetSizerAndFit(gbs)
