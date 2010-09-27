import wx
import dballe
from provami.QueryChoice import QueryChoice

def intormiss(x):
    if x == dballe.MISSING_INT:
        return "-"
    else:
        return "%d" % x

class LevelsChoice(QueryChoice):
    def __init__(self, parent, model):
        QueryChoice.__init__(self, parent, model, "level", "levels")
        self.hasData("levels")

    def readFilterFromRecord(self, rec):
        return self.model.getLevelFilter(rec)

    def readOptions(self):
        res = []
        res.append(("All levels", None))
        for lev in self.model.levels():
            res.append((",".join([intormiss(x) for x in lev]), lev))
        return res

    def selected(self, event):
        if self.updating: return
        sel = self.GetSelection()
        lev = self.GetClientData(sel)
        if lev is None:
            self.model.setLevelFilter(None)
        else:
            self.model.setLevelFilter(lev)
