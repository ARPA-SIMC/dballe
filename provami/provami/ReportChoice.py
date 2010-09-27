import wx
from provami.QueryChoice import QueryChoice

class ReportChoice(QueryChoice):
	def __init__(self, parent, model):
		QueryChoice.__init__(self, parent, model, "repinfo", "repinfo")
		self.hasData("repinfo")

	def readFilterFromRecord(self, record):
		return record.get("rep_cod", None)

	def readOptions(self):
		res = []
		res.append(("All reports", None))
		for (cod, memo) in self.model.reports():
			res.append((memo, cod))
		return res

	def selected(self, event):
		if self.updating: return
		self.model.setReportFilter(self.GetClientData(self.GetSelection()))
