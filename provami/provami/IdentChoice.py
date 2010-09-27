import wx
from provami.QueryChoice import QueryChoice

class IdentChoice(QueryChoice):
	def __init__(self, parent, model):
		QueryChoice.__init__(self, parent, model, "stations", "idents")
		self.hasData("idents")

	def readFilterFromRecord(self, rec):
		mobile = rec.get("mobile", None)
		ident = rec.get("ident", None)
		if mobile is None:
			return None
		if ident is None:
			return "__fixed__"
		return ident

	def readOptions(self):
		res = []
		res.append(("All stations", None))
		for v in self.model.idents():
			if v is None:
				res.append(("Only fixed stations", "__fixed__"))
			else:
				res.append((v, v))
		return res


	def selected(self, event):
		if self.updating: return
		sel = self.GetClientData(self.GetSelection())
		if sel is None:
			self.model.setIdentFilter(None, None)
		elif sel == "__fixed__":
			self.model.setIdentFilter(False, None)
		else:
			self.model.setIdentFilter(True, sel)
