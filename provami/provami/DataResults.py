# -*- coding: UTF-8 -*-
import wx
import wx.grid
from dballe import Level, TimeRange
from provami.Model import ModelListener, DateUtils, datetimeFromRecord
from provami.ResultGrid import ResultTable, ResultGrid
from provami.DataMenu import DataMenu

# Tooltips per grid cell
# http://www.archivesat.com/wxPython/thread379326.htm
#
# wxGrid introduction
# http://wiki.wxpython.org/index.cgi/wxGrid

def val_compare(a, b):
    vara = a.enqvar(a.enqc("var"))
    varb = b.enqvar(b.enqc("var"))
    isstra = vara.info().is_string()
    isstrb = varb.info().is_string()
    if isstra and isstrb:
        return cmp(vara.enqc(), varb.enqc())
    elif isstra and not isstrb:
        return 1
    elif not isstra and isstrb:
        return -1
    else:
        return cmp(vara.enqd(), varb.enqd())

class DataTable(ResultTable):
    def __init__(self, model):
        ResultTable.__init__(self)
        self.model = model
        
        self.appendColumn("Ana", \
                  renderer = lambda x: x.enqc("ana_id"), 
                  sorter = lambda x, y: cmp(x.enqi("ana_id"), y.enqi("ana_id")))

        self.appendColumn("Network", \
                  renderer = lambda x: x.enqc("rep_memo"), \
                  sorter = lambda x, y: cmp(x.enqc("rep_memo"), y.enqc("rep_memo")))

        self.appendColumn("Date", \
                  renderer = lambda x: x.enqdate(), \
                  sorter = lambda x, y: cmp(x.enqdate(), y.enqdate()))

        self.appendColumn("Level", \
                  renderer = lambda x: "%d,%d,%d,%d" % x.enqlevel(), \
                  sorter = lambda x, y: cmp(x.enqlevel(), y.enqlevel()))

        self.appendColumn("Time range", \
                  renderer = lambda x: "%d,%d,%d" % x.enqtimerange(), \
                  sorter = lambda x, y: cmp(x.enqtimerange(), y.enqtimerange()))

        self.appendColumn("Variable", \
                  renderer = lambda x: x.enqc("var"), \
                  sorter = lambda x, y: cmp(x.enqc("var"), y.enqc("var")))

        self.appendColumn("Value", \
                  renderer = lambda x: x.enqvar(x.enqc("var")).format(), \
                  sorter = val_compare,
                  editable = True)

    def SetValue(self, row, col, value):
        if row >= len(self.items): return
        if col != 6: return

        try:
            record = self.items[row]
            varcode = record.enqc("var")
            var = record.enqvar(varcode)
            if var.info().is_string():
                record.setc(varcode, str(value))
            else:
                record.setd(varcode, float(value))
            self.model.writeRecord(record)
        except ValueError:
            pass

    def rowByContextAndVar(self, context, var):
        "Return the row number of a result given its context id and varcode"
        for idx, i in enumerate(self.items):
            if i.enqi("context_id") == context and i.enqc("var") == var:
                return idx
        return None

    def getRow(self, data):
        if data == None: return None
        context_id = data.enqi("context_id")
        var = data.enqc("var")
        for row, d in enumerate(self.items):
            if d.enqi("context_id") == context_id and d.enqc("var") == var:
                return row
        return None

    def update(self):
        for record in self.model.data():
            self.items.append(record.copy())
        self.sort()
        view = self.GetView()
        if view != None:
            view.ProcessTableMessage(
                wx.grid.GridTableMessage(self, wx.grid.GRIDTABLE_NOTIFY_ROWS_APPENDED, len(self.items)))
            # FIXME: wxwidgets bug workaround to have the
            # scrollbars to resize
            # See: http://wiki.wxpython.org/index.cgi/Frequently_Asked_Questions#head-43f3f79f739a4c503584c4fb9620d40bf273e418
            view.FitInside()

class DataPanel(wx.Panel, ModelListener):
    def __init__(self, parent, model, statusbar):
        wx.Panel.__init__(self, parent)
        #wx.Panel.__init__(self, parent, id, wx.TAB_TRAVERSAL)
        self.model = model
        self.model.registerUpdateListener(self)

        self.current = None

        self.statusBar = statusbar

        self.dataMenu = DataMenu()
        self.results = ResultGrid(self)
        self.results.SetTable(DataTable(model))
        self.results.setPopupMenu(self.dataMenu)

        box = wx.BoxSizer(wx.VERTICAL)
        box.Add(self.results, 1, flag=wx.EXPAND)
        self.SetSizerAndFit(box)

        self.dataMenu.Bind(wx.EVT_MENU, self.onDataMenu)
        self.results.Bind(ResultGrid.EVT_FLYOVER, self.onFlyOver)

        self.invalidate()
        self.hasData("data")

    def invalidate (self):
        self.current = self.results.saveCurrent()
        #self.results.GetTable().clear()

    def hasData (self, what):
        if what == "data":
            self.results.updating = True
            self.results.GetTable().clear()
            self.results.GetTable().update()
            self.results.restoreCurrent(self.current)
            self.results.updating = False

    def onFlyOver(self, event):
        row, col = event.GetCell()
        record = event.GetData()
        if record != None:
            if col == 0:
                ident = record.enqc("ident")
                if ident == None:
                    info = "Fixed station"
                else:
                    info = "Mobile station " + ident
                info = info + ", lat %f, lon %f" % (record.enqd("lat"), record.enqd("lon"))
            elif col == 3:
                info = str(record.enqlevel())
            elif col == 4:
                info = str(record.enqtimerange())
            else:
                info = record.enqvar(record.enqc("var")).info()
                info = "%s (%s)" % (info.desc(), info.unit())

            self.statusBar.SetStatusText(info, 0)

    def onDataMenu(self, event):
        if event.GetId() == DataMenu.ACTION_SELECT_SAME_ANA_ID:
            record = self.dataMenu.getData()
            self.model.setStationFilter(record.enqi("ana_id"))
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_IDENT:
            record = self.dataMenu.getData()
            ident = record.enqc("ident")
            self.model.setIdentFilter(ident != None, ident)
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_LEVEL:
            record = self.dataMenu.getData()
            self.model.setLevelFilter(record.enqlevel())
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_TRANGE:
            record = self.dataMenu.getData()
            self.model.setTimeRangeFilter(record.enqtimerange())
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_VAR:
            record = self.dataMenu.getData()
            self.model.setVarFilter(record.enqc("var"))
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_REPCOD:
            record = self.dataMenu.getData()
            self.model.setReportFilter(record.enqi("rep_cod"))
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_DATEMIN:
            dt = datetimeFromRecord(self.dataMenu.getData(), DateUtils.EXACT)
            self.model.setDateTimeFilter(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, DateUtils.MIN)
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_DATEMAX:
            dt = datetimeFromRecord(self.dataMenu.getData(), DateUtils.EXACT)
            self.model.setDateTimeFilter(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, DateUtils.MAX)
        elif event.GetId() == DataMenu.ACTION_DELETE_CURRENT:
            record = self.dataMenu.getData()
            context, id = record.enqi("context_id"), record.enqc("var")
            self.model.deleteValues(((context, id),))
        elif event.GetId() == DataMenu.ACTION_DELETE_SELECTED:
            grid = self.dataMenu.getGrid()
            self.model.deleteValues([(r.enqi("context_id"), r.enqc("var")) for r in grid.getSelectedData()])
        else:
            event.Skip()

