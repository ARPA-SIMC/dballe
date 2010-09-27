import wx
from provami.Model import Model, ProgressListener

class ProgressDisplay(ProgressListener):
	def __init__(self, parent, model):
		self.parent = parent
		self.model = model
		self.model.registerProgressListener(self)
		self.dlg = None

	def progress(self, perc, text):
		print "%d%%: %s" % (perc, text)
		if perc == 100:
			if self.dlg is not None:
				self.dlg.Destroy()
				self.dlg = None
		else:
			if self.dlg is None:
				self.dlg = wx.ProgressDialog("Updating data",
						text,
						maximum = 100,
						parent = self.parent,
						style = wx.PD_CAN_ABORT
							| wx.PD_APP_MODAL
							| wx.PD_ELAPSED_TIME
							| wx.PD_ESTIMATED_TIME
							| wx.PD_REMAINING_TIME)
			if not self.dlg.Update(perc, text):
				self.model.cancelUpdate()
		wx.Yield()
