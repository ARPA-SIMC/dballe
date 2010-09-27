import dballe
import wx
from provami.QueryChoice import QueryChoice

class VarNamesChoice(QueryChoice):
	def __init__(self, parent, model):
		QueryChoice.__init__(self, parent, model, "var", "vartypes")
		self.hasData("vartypes")

	def readFilterFromRecord(self, record):
		return record.get("var", None)

	def readOptions(self):
		res = []
		res.append(("All variables", None))
		for v in self.model.variableTypes():
			info = dballe.varinfo(v)
			res.append(("%s: %s (%s)" % (v, info.desc.lower(), info.unit), v))
		return res

	def selected(self, event):
		if self.updating: return
		sel = self.GetSelection()
		self.model.setVarFilter(self.GetClientData(sel))
