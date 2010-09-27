# -*- coding: UTF-8 -*-
import wx
import wx.grid

# Tooltips per grid cell
# http://www.archivesat.com/wxPython/thread379326.htm
#
# wxGrid introduction
# http://wiki.wxpython.org/index.cgi/wxGrid
#
# Custom event type examples
# http://archives.devshed.com/forums/python-122/wxstatictext-164649.html

class ResultTable(wx.grid.PyGridTableBase):
	def __init__(self):
		wx.grid.PyGridTableBase.__init__(self)
		
		self.items = []
		self.renderer = []
		self.sorter = []
		self.colNames = []
		self.colEditable = []

		self.sortColumn = None
		self.sortDescending = False

	def appendColumn(self, title, renderer = None, sorter = None, editable = False):
		self.colNames.append(title)
		if renderer is not None:
			self.renderer.append(renderer)
		else:
			self.renderer.append(lambda x: str(x))
		if sorter is not None:
			self.sorter.append(sorter)
		else:
			self.sorter.append(lambda x, y: cmp(x, y))
		self.colEditable.append(editable)

	def getData(self, row):
		if row < 0 or row >= len(self.items):
			return None
		return self.items[row]

	def getRow(self, data):
		"""Return the row number for the given data, or None if no row
		contains the given data"""
		for row, d in enumerate(self.items):
			if d == data:
				return row
		return None

	def GetNumberCols(self):
		return len(self.colNames)

	def GetNumberRows(self):
		return len(self.items)

	def GetColLabelValue(self, col):
		"Get column titles"
		# ˄˅↑↓↥↧⇑⇓⇧⇩∇∆
		if col == self.sortColumn:
			if self.sortDescending:
				return self.colNames[col] + " ∆"
			else:
				return self.colNames[col] + " ∇"
		else:
			return self.colNames[col]

	def GetValue(self, row, col):
		"Get cell values"
		if row < 0 or row >= self.GetNumberRows():
			return "(error)"
		else:
			return self.renderer[col](self.items[row])

	def GetAttr(self, row, col, kind):
		"""
		Get cell attributes.

		It will return the standard attribute if the value is different
		than the cell above, else it will return a grayed attribute.
		"""
		attr = wx.grid.GridCellAttr()
		provider = self.GetAttrProvider()
		if provider is not None:
			defattr = provider.GetAttr(row, col, kind)
			if defattr is not None:
				attr.MergeWith(defattr)

		if row >= len(self.items):
			attr.SetReadOnly(True)
		else:
			attr.SetReadOnly(not self.colEditable[col])

		if row != 0:
			this = self.renderer[col](self.items[row])
			sup = self.renderer[col](self.items[row - 1])
			if sup == this: attr.SetTextColour("GRAY50")
		return attr

	def setSortColumn(self, col):
		"Change the sorting column"

		if col == self.sortColumn:
			self.sortDescending = not self.sortDescending
		else:
			self.sortColumn = col
		if self.sortDescending:
			self.items.sort(lambda a, b: self.sorter[self.sortColumn](b, a))
		else:
			self.items.sort(self.sorter[self.sortColumn])

		# This triggers a redraw
		view = self.GetView()
		if view is not None:
			view.ProcessTableMessage(
				wx.grid.GridTableMessage(self, wx.grid.GRIDTABLE_REQUEST_VIEW_GET_VALUES))
		self.GetView().Refresh()

	def sort(self):
		"Resort using the current sorting column"

		if self.sortColumn is not None:
			if self.sortDescending:
				self.items.sort(lambda a, b: self.sorter[self.sortColumn](b, a))
			else:
				self.items.sort(self.sorter[self.sortColumn])

		# This triggers a redraw
		view = self.GetView()
		if view is not None:
			view.ProcessTableMessage(
				wx.grid.GridTableMessage(self, wx.grid.GRIDTABLE_REQUEST_VIEW_GET_VALUES))

	def clear(self):
		count = len(self.items)
		self.items = []
		view = self.GetView()
		if view is not None:
			view.ProcessTableMessage(
				wx.grid.GridTableMessage(self, wx.grid.GRIDTABLE_NOTIFY_ROWS_DELETED, 0, count))
			# FIXME: wxwidgets bug workaround to have the
			# scrollbars to resize
			# See: http://wiki.wxpython.org/index.cgi/Frequently_Asked_Questions#head-43f3f79f739a4c503584c4fb9620d40bf273e418
			view.FitInside()


class ResultGrid(wx.grid.Grid):
	class ResultGridEvent(wx.PyCommandEvent):
		def __init__(self, eventType, id):
			wx.PyCommandEvent.__init__(self, eventType, id)
			self.pos = None
			self.data = None

		def SetCell(self, row, col):
			self.pos = (row, col)

		def SetData(self, data):
			self.data = data

		def GetCell(self):
			"Get the (row, column) position of the cell"
			return self.pos

		def GetData(self):
			"Get the data corresponding to the cell row"
			return self.data

	# Create custom events
	wxEVT_FLYOVER = wx.NewEventType()
	EVT_FLYOVER = wx.PyEventBinder(wxEVT_FLYOVER, 0)

	wxEVT_CHANGED_CURRENT_ROW = wx.NewEventType()
	EVT_CHANGED_CURRENT_ROW = wx.PyEventBinder(wxEVT_CHANGED_CURRENT_ROW, 0)

	def __init__(self, parent):
		wx.grid.Grid.__init__(self, parent)

		self.oldFlyoverCell = None
		self.currentRow = None
		self.currentCol = None
		self.popupMenu = None
		self.selection = set()

		self.EnableEditing(True)
		self.EnableDragRowSize(False)
		self.SetSelectionMode(self.wxGridSelectRows)
		self.SetRowLabelSize(0)
		# Allow the grid to become smaller: by default it wants to be too tall
		self.SetSizeHints(100, 100)
		self.AutoSizeColumns()

		self.Bind(wx.grid.EVT_GRID_LABEL_LEFT_CLICK, self.onLabelLeftClicked)
		self.Bind(wx.grid.EVT_GRID_CELL_RIGHT_CLICK, self.onCellRightClicked)
		self.Bind(wx.grid.EVT_GRID_SELECT_CELL, self.onCellSelect)
		self.Bind(wx.grid.EVT_GRID_RANGE_SELECT, self.onRangeSelect)

		self.GetGridWindow().Bind(wx.EVT_MOTION, self.onMouseMotion)
		self.updating = False

	def countSelectedRows(self):
		"Return the number of selected rows"
		return len(self.selection)

	def getSelectedRows(self):
		"Return the (unsorted) sequence of selected row indexes"
		return self.selection

	def getSelectedData(self):
		"Return the (unsorted) sequence of selected data"
		return (self.GetTable().getData(i) for i in self.selection)

	def getFirstRow(self):
		"Return the data corresponding to the first row"
		return self.GetTable().getData(0)

	def setPopupMenu(self, menu):
		"""
		Set a menu to use as a right-click popup menu

		The menu must be a wx.Menu or one of its descendents.  If it
		contains a method like:
			def configureCell(grid, row, col, data):
		then it is called every time before popping up the menu, to
		give a chance to implement custom per-cell menus
		"""
		self.popupMenu = menu

	def onRangeSelect(self, event):
                """Internal update to the selection tracking list"""
                if event.Selecting():
                        # adding to the list...
                        for index in range(event.GetTopRow(), event.GetBottomRow()+1):
				self.selection.add(index)
                else:
                        # removal from list
                        for index in range(event.GetTopRow(), event.GetBottomRow()+1):
				self.selection.discard(index)
                event.Skip()

	def onCellSelect(self, event):
                """
		Internal update to the selection tracking list
		
		Also generate FLYOVER events
		"""

		self.selection.clear()
                self.selection.add(event.GetRow())
	
		row, col = event.GetRow(), event.GetCol()
		self.triggerFlyOver(row, col)
		self.triggerChangedCurrentRow(row, col)
		self.currentCol = col

		event.Skip()

	def onCellRightClicked(self, event):
		"Handle popping up a menu at right click"
		if self.popupMenu is not None:
			if getattr(self.popupMenu, "configureCell", None) is not None:
				# If the menu has a configureCell method, call
				# it to give a chance to offer a different,
				# per-cell menu
				row, col, data = event.GetRow(), event.GetCol(), None
				if row != -1: data = self.GetTable().getData(row)
				if not self.popupMenu.configureCell(self, row, col, data):
					# Don't pop up if the menu does not want to
					return
			self.PopupMenu(self.popupMenu, event.GetPosition())

	def triggerFlyOver(self, row, col):
		if self.updating: return
		if self.oldFlyoverCell != (row, col):
			self.oldFlyoverCell = (row, col)

			e = ResultGrid.ResultGridEvent(ResultGrid.wxEVT_FLYOVER, self.GetId())
			e.SetCell(row, col)
			if row != -1: e.SetData(self.GetTable().getData(row))

			self.GetEventHandler().ProcessEvent(e)

	def triggerChangedCurrentRow(self, row, col):
		if self.updating: return
		#if self.currentRow == row: return
		self.currentRow = row

		e = ResultGrid.ResultGridEvent(ResultGrid.wxEVT_CHANGED_CURRENT_ROW, self.GetId())
		e.SetCell(row, col)
		if row != -1: e.SetData(self.GetTable().getData(row))

		self.GetEventHandler().ProcessEvent(e)

	def onLabelLeftClicked(self, event):
		c = self.saveCurrent()
		row, col = event.GetRow(), event.GetCol()
		if row == -1:
			self.GetTable().setSortColumn(col)
		self.restoreCurrent(c)
		#event.Skip()

	def onMouseMotion(self, event):
		# FIXME: nothing better than going through the scrolled window?
		x, y = self.CalcUnscrolledPosition(event.GetPosition())
		row, col = self.XYToCell(x, y).Get()
		self.triggerFlyOver(row, col)
		event.Skip()
	
	def saveCurrent(self):
		"""
		Return an opaque object that represent the current cell in the
		table.  The object can survive sorting and data updates, and be
		used later with restoreCurrent
		"""
		data = self.GetTable().getData(self.currentRow)
		return (data, self.currentCol)

	def restoreCurrent(self, current):
		if current is None: return
		data, col = current
		row = self.GetTable().getRow(data) or 0
		if col is None: col = 0
		self.SetGridCursor(row, col)

		# Scroll to the cell if it went out of view
		if not self.IsVisible(row, col, True):
			self.MakeCellVisible(row, col)

