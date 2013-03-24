import dballe
import wx
import wx.grid
from provami.Model import ModelListener
from provami.ResultGrid import ResultTable, ResultGrid
from provami.DataMenu import DataMenu

def val_compare(a, b):
    vara = a.var()
    varb = b.var()
    isstra = vara.info.is_string
    isstrb = varb.info.is_string
    if isstra and isstrb:
        return cmp(vara.enqc(), varb.enqc())
    elif isstra and not isstrb:
        return 1
    elif not isstra and isstrb:
        return -1
    else:
        return cmp(vara.enqd(), varb.enqd())

class AnaTable(ResultTable):
    def __init__(self, model):
        ResultTable.__init__(self)

        self.model = model

        self.appendColumn("Network", \
                  renderer = lambda x: x["rep_memo"], \
                  sorter = lambda x, y: cmp(x["rep_memo"], y["rep_memo"]))

        self.appendColumn("Variable", \
                  renderer = lambda x: x["var"], \
                  sorter = lambda x, y: cmp(x["var"], y["var"]))

        self.appendColumn("Value", \
                  renderer = lambda x: x.var().format(), \
                  sorter = val_compare,
                  editable = True)

        self.appendColumn("Unit", \
                  renderer = lambda x: x.var().info.unit, \
                  sorter = val_compare)

        self.appendColumn("Description", \
                  renderer = lambda x: x.var().info.desc, \
                  sorter = val_compare)

    def SetValue(self, row, col, value):
        if row >= len(self.items): return
        if col != 2: return

        try :
            record = self.items[row]
            var = record.var()
            if var.info.is_string:
                record[var.code] = str(value)
            else:
                record[var.code] = float(value)
            self.model.writeRecord(record)
        except ValueError:
            pass

    def display (self, id):
        count = len(self.items)
        self.items = []

        if id is not None:
            query = dballe.Record()
            query["ana_id"] = id
            query.set_ana_context()
            del query["rep_cod"]
            for record in self.model.db.query_data(query):
                self.items.append(record.copy())

            self.sort()

        view = self.GetView()
        if view is not None:
            view.ProcessTableMessage(
                wx.grid.GridTableMessage(self, wx.grid.GRIDTABLE_NOTIFY_ROWS_DELETED, 0, count))
            view.ProcessTableMessage(
                wx.grid.GridTableMessage(self, wx.grid.GRIDTABLE_NOTIFY_ROWS_APPENDED, len(self.items)))
            # FIXME: wxwidgets bug workaround to have the
            # scrollbars to resize
            # See: http://wiki.wxpython.org/index.cgi/Frequently_Asked_Questions#head-43f3f79f739a4c503584c4fb9620d40bf273e418
            view.FitInside()

    def getRow(self, data):
        if data is None: return None
        ana_id = data["ana_id"]
        rep_cod = data["rep_cod"]
        for row, d in enumerate(self.items):
            if d["ana_id"] == ana_id and d["rep_cod"]  == rep_cod:
                return row
        return None


class AnaResults(wx.Frame, ModelListener):
    def __init__(self, parent, model):
        wx.Frame.__init__(self, parent, title = "Pseudoana details", size = (400, 400),
                style=wx.DEFAULT_FRAME_STYLE | wx.NO_FULL_REPAINT_ON_RESIZE)

        self.SetSizeHints(500, 300)

        self.parent = parent
        self.model = model
        self.model.registerUpdateListener(self)

        self.currentID = None

        self.statusBar = self.CreateStatusBar()

        self.dataMenu = DataMenu()
        self.dataMenu.Bind(wx.EVT_MENU, self.onDataMenu)

        self.data = ResultGrid(self)
        self.data.SetTable(AnaTable(model))
        self.data.setPopupMenu(self.dataMenu)
        self.data.Bind(ResultGrid.EVT_FLYOVER, self.onFlyOver)

        detailsPanel = wx.Panel(self)
        sizer = wx.GridBagSizer()

        self.st_lat = wx.StaticText(detailsPanel)
        self.st_lon = wx.StaticText(detailsPanel)
        self.st_type = wx.StaticText(detailsPanel)

        sizer.Add(wx.StaticText(detailsPanel, -1, "Latitude: "), (0, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self.st_lat, (0, 1), (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(wx.StaticText(detailsPanel, -1, "Longitude: "), (1, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self.st_lon, (1, 1), (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(wx.StaticText(detailsPanel, -1, "Type: "), (2, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self.st_type, (2, 1), (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)

        detailsPanel.SetSizer(sizer)

        box = wx.BoxSizer(wx.VERTICAL)
        box.Add(detailsPanel, 1, wx.EXPAND)
        box.Add(self.data, 3, wx.EXPAND)
        self.SetSizerAndFit(box)

        self.Bind(wx.EVT_CLOSE, self.onClose)

    def invalidate (self):
        self.current = self.data.saveCurrent()
        #self.data.GetTable().clear()

    def filterChanged(self, what):
        if what == "station":
            id = self.model.filter.get("ana_id", None)
            if id is not None:
                self.displayID(id);

    def hasData (self, what):
        if what == "all":
            self.data.updating = True
            if self.model.hasStation(self.currentID):
                self.displayID(self.currentID)
            else:
                self.displayID(None)
            self.data.restoreCurrent(self.current)
            self.data.updating = False

    def onFlyOver(self, event):
        row, col = event.GetCell()
        record = event.GetData()
        if record is not None:
            info = record.var().info
            info = "%s (%s)" % (info.desc, info.unit)
            self.statusBar.SetStatusText(info, 0)

    def displayID(self, id):
        current = self.data.saveCurrent()
        if id is None:
            self.data.GetTable().display(None)
            self.st_lat.SetLabel("")
            self.st_lon.SetLabel("")
            self.st_type.SetLabel("")
        else:
            self.data.GetTable().display(id)
            id, lat, lon, ident = self.model.stationByID(id)
            self.st_lat.SetLabel(str(lat))
            self.st_lon.SetLabel(str(lon))
            if ident is None:
                self.st_type.SetLabel("fixed station")
            else:
                self.st_type.SetLabel("mobile station " + ident)
        self.currentID = id
        #self.data.restoreCurrent(current)

    def displayRecord(self, record):
        current = self.data.saveCurrent()
        if record is None:
            self.data.GetTable().display(None)
            self.st_lat.SetLabel("")
            self.st_lon.SetLabel("")
            self.st_type.SetLabel("")
            self.currentID = None
        else:
            self.currentID = record["ana_id"]
            self.data.GetTable().display(self.currentID)
            self.st_lat.SetLabel(str(record["lat"]))
            self.st_lon.SetLabel(str(record["lon"]))
            ident = record.get("ident", None)
            if ident is None:
                self.st_type.SetLabel("fixed station")
            else:
                self.st_type.SetLabel("mobile station " + ident)
        self.data.restoreCurrent(current)

    def onClose(self, event):
        # Hide the window
        self.Hide()
        # Don't destroy the window
        event.Veto()
        # Notify parent that we've been closed
        self.parent.anaHasClosed()

    def onDataMenu(self, event):
        if event.GetId() == DataMenu.ACTION_SELECT_SAME_ANA_ID:
            record = self.dataMenu.getData()
            self.model.setStationFilter(record["ana_id"])
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_IDENT:
            record = self.dataMenu.getData()
            ident = record["ident"]
            self.model.setIdentFilter(ident is not None, ident)
        elif event.GetId() == DataMenu.ACTION_SELECT_SAME_REPCOD:
            record = self.dataMenu.getData()
            self.model.setReportFilter(record["rep_cod"])
        elif event.GetId() == DataMenu.ACTION_DELETE_CURRENT:
            record = self.dataMenu.getData()
            context, id = record["context_id"], record["var"]
            self.model.deleteValues(((context, id),))
        elif event.GetId() == DataMenu.ACTION_DELETE_SELECTED:
            grid = self.dataMenu.getGrid()
            self.model.deleteValues([(r["context_id"], r["var"]) for r in grid.getSelectedData()])
        else:
            event.Skip()
