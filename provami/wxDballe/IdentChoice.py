import wx
from wxDballe.QueryChoice import QueryChoice

class IdentChoice(QueryChoice):
	def __init__(self, parent, model):
		QueryChoice.__init__(self, parent, model, "stations", "idents")
		self.hasData("idents")

	def readFilterFromRecord(self, rec):
		mobile = rec.enqi("mobile")
		ident = rec.enqc("ident")
		if mobile == None:
			return None
		if ident == None:
			return "__fixed__"
		return ident

	def readOptions(self):
		res = []
		res.append(("All stations", None))
		for v in self.model.idents():
			if v == None:
				res.append(("Only fixed stations", "__fixed__"))
			else:
				res.append((v, v))
		return res


	def selected(self, event):
		if self.updating: return
		sel = self.GetClientData(self.GetSelection())
		if sel == None:
			self.model.setIdentFilter(None, None)
		elif sel == "__fixed__":
			self.model.setIdentFilter(False, None)
		else:
			self.model.setIdentFilter(True, sel)
