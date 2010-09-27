import wx
from provami.Model import *

class DataMenu(wx.Menu):
	ACTION_SELECT_SAME_ANA_ID = wx.NewId()
	ACTION_SELECT_SAME_IDENT = wx.NewId()
	ACTION_SELECT_SAME_LEVEL = wx.NewId()
	ACTION_SELECT_SAME_TRANGE = wx.NewId()
	ACTION_SELECT_SAME_VAR = wx.NewId()
	ACTION_SELECT_SAME_REPCOD = wx.NewId()
	ACTION_SELECT_SAME_DATEMIN = wx.NewId()
	ACTION_SELECT_SAME_DATEMAX = wx.NewId()
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
			isAnaContext = data["year"] == 1000

			self.Append(DataMenu.ACTION_SELECT_SAME_ANA_ID, "Select station %d (lat %f lon %f)" %
					(data["ana_id"], data["lat"], data["lon"]))
			ident = data.get("ident", None)
			if ident is not None:
				self.Append(DataMenu.ACTION_SELECT_SAME_IDENT, "Select all stations " + ident)
			else:
				self.Append(DataMenu.ACTION_SELECT_SAME_IDENT, "Select all fixed stations")

			if not isAnaContext:
				self.Append(DataMenu.ACTION_SELECT_SAME_LEVEL, "Select level " + str(data["level"]))
				self.Append(DataMenu.ACTION_SELECT_SAME_TRANGE, "Select time range " + str(data["trange"]))
				self.Append(DataMenu.ACTION_SELECT_SAME_VAR, "Select variable type " + data["var"])
			self.Append(DataMenu.ACTION_SELECT_SAME_REPCOD, "Select report type " + data["rep_memo"])

			if not isAnaContext:
				mindate = datetimeFromRecord(data, DateUtils.EXACT)
				self.Append(DataMenu.ACTION_SELECT_SAME_DATEMIN, "Select minimum date " + str(mindate))
				maxdate = datetimeFromRecord(data, DateUtils.EXACT)
				self.Append(DataMenu.ACTION_SELECT_SAME_DATEMAX, "Select maximum date " + str(maxdate))
			self.AppendSeparator()
			self.Append(DataMenu.ACTION_DELETE_CURRENT, "Delete value")
			hasVoices = True

		selCount = grid.countSelectedRows()
		if selCount > 1:
			#if hasVoices: mFile.AppendSeparator()
			self.Append(DataMenu.ACTION_DELETE_SELECTED, "Delete %d selected values" % (selCount))
			hasVoices = True;

		if hasVoices:
			self.row, self.col, self.data, self.grid = row, col, data, grid

		# If there are no entries, we do not pop up
		return hasVoices
