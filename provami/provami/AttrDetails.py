import dballe
import wx
import wx.grid
import traceback
from provami.Model import ModelListener
from provami.ResultGrid import ResultTable, ResultGrid

def val_compare(a, b):
    isstra = a.info.is_string
    isstrb = b.info.is_string
    if isstra and isstrb:
        return cmp(a.enqc(), b.enqc())
    elif isstra and not isstrb:
        return 1
    elif not isstra and isstrb:
        return -1
    else:
        return cmp(a.enqd(), b.enqd())

class AttrMenu(wx.Menu):
    ACTION_DELETE_CURRENT = wx.NewId()
    ACTION_DELETE_SELECTED = wx.NewId()

    def __init__(self):
        wx.Menu.__init__(self)
        self.row, self.col, self.data, self.grid = None, None, None, None

    def clearMenu(self):
        "Removes all items from the menu"
        while self.GetMenuItemCount() > 0:
            item = self.FindItemByPosition(0)
            self.DestroyItem(item)

    def getCell(self):
        return self.row, self.col

    def getData(self):
        return self.data

    def getGrid(self):
        return self.grid

    def configureCell(self, grid, row, col, data):
        "Reconfigure the menu to pop up on the given grid cell"

        hasVoices = False
        self.clearMenu()

        if data is not None:
            self.Append(AttrMenu.ACTION_DELETE_CURRENT, "Delete value")
            hasVoices = True

        selCount = grid.countSelectedRows()
        if selCount > 1:
            self.Append(AttrMenu.ACTION_DELETE_SELECTED, "Delete %d selected values" % (selCount))
            hasVoices = True;

        if hasVoices:
            self.row, self.col, self.data, self.grid = row, col, data, grid

        # If there are no entries, we do not pop up
        return hasVoices


class AttributeTable(ResultTable):
    def __init__(self, model):
        ResultTable.__init__(self)

        self.model = model

        self.appendColumn("Variable", \
                  renderer = lambda x: x.code, \
                  sorter = lambda x, y: cmp(x.code, y.code))

        self.appendColumn("Value", \
                  renderer = lambda x: str(x), \
                  sorter = val_compare,
                  editable = True)

        self.appendColumn("Unit", \
                  renderer = lambda x: x.info.unit, \
                  sorter = val_compare)

        self.appendColumn("Description", \
                  renderer = lambda x: x.info.desc, \
                  sorter = val_compare)

        self.context = None
        self.varcode = None

    def SetValue(self, row, col, value):
        if row >= len(self.items): return
        if col != 1: return

        try :
            var = self.items[row]
            if var.info.is_string:
                var.set(str(value))
            else:
                var.set(float(value))
            self.model.updateAttribute(self.context, self.varcode, var)
        except ValueError:
            pass

    def display (self, context, var):
        count = len(self.items)
        self.items = []
        self.context = context
        self.varcode = var

        if context is not None:
            attrs = self.model.db.query_attrs(var, context)

            for var in attrs:
                self.items.append(attrs.var(var))

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
        var = data.code
        for row, d in enumerate(self.items):
            if d.code == var:
                return row
        return None

class AttrResults(wx.Frame, ModelListener):
    def __init__(self, parent, model):
        wx.Frame.__init__(self, parent, title = "Attributes", size = (400, 400),
                style=wx.DEFAULT_FRAME_STYLE | wx.NO_FULL_REPAINT_ON_RESIZE)

        self.SetSizeHints(500, 300)

        self.parent = parent
        self.model = model
        self.model.registerUpdateListener(self)

        self.currentID = (None, None)

        self.statusBar = self.CreateStatusBar()

        self.dataMenu = AttrMenu()
        self.dataMenu.Bind(wx.EVT_MENU, self.onDataMenu)

        self.data = ResultGrid(self)
        self.data.SetTable(AttributeTable(model))
        self.data.setPopupMenu(self.dataMenu)
        self.data.Bind(ResultGrid.EVT_FLYOVER, self.onFlyOver)

        self.details = wx.StaticText(self)

        box = wx.BoxSizer(wx.VERTICAL)
        box.Add(self.details, 0, wx.EXPAND)
        box.Add(self.data, 1, wx.EXPAND)
        self.SetSizerAndFit(box)

        self.Bind(wx.EVT_CLOSE, self.onClose)

    def invalidate (self):
        self.current = self.data.saveCurrent()
        #self.data.GetTable().clear()

    def hasData (self, what):
        if what == "all":
            self.data.updating = True
            if self.model.hasVariable(self.currentID[0], self.currentID[1]):
                self.displayID(self.currentID[0], self.currentID[1])
            else:
                self.displayID(None, None)
            self.data.restoreCurrent(self.current)
            self.data.updating = False

    def displayID(self, context, var):
        #print "DISPLAY ID", context, var
        #traceback.print_stack()
        current = self.data.saveCurrent()
        if context is None or var is None:
            self.details.SetLabel("No variable is currently selected")
        else:
            self.details.SetLabel("Variable %s in context %d" % (var, context))
        self.data.GetTable().display(context, var)
        self.currentID = (context, var)
        self.data.restoreCurrent(current)

    def display(self, record):
        #print "ATTR Got record", record["context_id"], record["var"]
        if record is not None:
            self.displayID(record["context_id"], record["var"])
        else:
            self.displayID(None, None)

    def onFlyOver(self, event):
        row, col = event.GetCell()
        var = event.GetData()
        if var is not None:
            info = var.info
            info = "%s (%s)" % (info.desc, info.unit)
            self.statusBar.SetStatusText(info, 0)

    def onClose(self, event):
        # Hide the window
        self.Hide()
        # Don't destroy the window
        event.Veto()
        # Notify parent that we've been closed
        self.parent.attrHasClosed()

    def onDataMenu(self, event):
        if event.GetId() == AttrMenu.ACTION_DELETE_CURRENT:
            table = self.data.GetTable()
            context, id = table.context, table.varcode
            var = self.dataMenu.getData()
            self.model.deleteAttrs(context, id, (var.code,))
        elif event.GetId() == AttrMenu.ACTION_DELETE_SELECTED:
            table = self.data.GetTable()
            context, id = table.context, table.varcode
            self.model.deleteAttrs(context, id, [v.code for v in self.data.getSelectedData()])
        else:
            event.Skip()
