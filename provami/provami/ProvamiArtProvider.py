import wx
from provami.Navigator import Navigator
from provami.Model import filePath
from provami.MapChoice import MapChoice

class ProvamiArtProvider(wx.ArtProvider):
	def __init__(self):
		wx.ArtProvider.__init__(self)

	def CreateBitmap(self, artid, client, size):
		bmp = wx.NullBitmap
		if artid == Navigator.ICON_PROVAMI:
			bmp = wx.Bitmap(filePath("icon-provami.png"), wx.BITMAP_TYPE_PNG)
		elif artid == Navigator.ICON_MAP_WINDOW:
			bmp = wx.Bitmap(filePath("icon-map-window.png"), wx.BITMAP_TYPE_PNG)
		elif artid == Navigator.ICON_ANA_WINDOW:
			bmp = wx.Bitmap(filePath("icon-ana-window.png"), wx.BITMAP_TYPE_PNG)
		elif artid == Navigator.ICON_ATTR_WINDOW:
			bmp = wx.Bitmap(filePath("icon-attr-window.png"), wx.BITMAP_TYPE_PNG)
		elif artid == Navigator.ICON_FILTERS_WINDOW:
			bmp = wx.Bitmap(filePath("icon-filters-window.png"), wx.BITMAP_TYPE_PNG)
		elif artid == MapChoice.ICON_MAP_MOVE:
			bmp = wx.Bitmap(filePath("icon-map-move.png"), wx.BITMAP_TYPE_PNG)
		elif artid == MapChoice.ICON_MAP_SELECT_AREA:
			bmp = wx.Bitmap(filePath("icon-map-select-area.png"), wx.BITMAP_TYPE_PNG)
		elif artid == MapChoice.ICON_MAP_SELECT_STATION:
			bmp = wx.Bitmap(filePath("icon-map-select-station.png"), wx.BITMAP_TYPE_PNG)
		elif artid == MapChoice.ICON_MAP_SELECT_IDENT:
			bmp = wx.Bitmap(filePath("icon-map-select-ident.png"), wx.BITMAP_TYPE_PNG)


		return bmp
