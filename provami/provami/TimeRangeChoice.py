import wx
from wxDballe.QueryChoice import QueryChoice

class TimeRangeChoice(QueryChoice):
	def __init__(self, parent, model):
		QueryChoice.__init__(self, parent, model, "trange", "tranges")
		self.hasData("tranges")

	def readFilterFromRecord(self, rec):
		return self.model.getTimeRangeFilter(rec)

	def readOptions(self):
		res = []
		res.append(("All time ranges", None))
		for tr in self.model.timeranges():
			res.append((tr.format(), tr))
		return res

	def selected(self, event):
		if self.updating: return
		sel = self.GetSelection()
		tr = self.GetClientData(sel)
		if tr == None:
			self.model.setTimeRangeFilter(None, None, None)
		else:
			self.model.setTimeRangeFilter(tr.type, tr.p1, tr.p2)
