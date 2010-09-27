import wx
import dballe
from provami.QueryChoice import QueryChoice

def intormiss(x):
    if x == dballe.MISSING_INT:
        return "-"
    else:
        return "%d" % x

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
            res.append((",".join([intormiss(x) for x in tr]), tr))
        return res

    def selected(self, event):
        if self.updating: return
        sel = self.GetSelection()
        tr = self.GetClientData(sel)
        if tr is None:
            self.model.setTimeRangeFilter(None)
        else:
            self.model.setTimeRangeFilter(tr)
