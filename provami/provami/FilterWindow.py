import dballe
import wx
import wx.html
import re
from provami.Model import *
from provami.QueryTextCtrl import QueryTextCtrl

class FilterWindow(wx.Frame):
    def __init__(self, parent, model):
        wx.Frame.__init__(self, parent, title = "Extra filters", size = (400, 300),
                style=wx.DEFAULT_FRAME_STYLE | wx.NO_FULL_REPAINT_ON_RESIZE)

        self.parent = parent
        self.model = model

        #self.statusBar = self.CreateStatusBar()

        self.validateExpression1 = re.compile(r"^(?:([^<=>]+)(?:<=|==|=|>=|<|>|<>)[^<=>]+|)$")
        self.validateExpression2 = re.compile(r"^[^<=>]+<=([^<=>]+)<=[^<=>]+$")
        self.bcodeExpression = re.compile(r"^B([0-9]{2})([0-9]{3})$")

        self.intro = wx.html.HtmlWindow(self)
        self.intro.SetSizeHints(500, 300)
        self.intro.SetBorders(3)
        self.intro.SetFonts("", "", [4, 6, 8, 10, 11, 12, 13])
        self.intro.SetPage("""
<html><body>
<p>
Here you can enter some extra filters for your data.
</p>

<p>
The ana filter restricts the data to only those stations for which there is a
<em>station variable</em> matching the filter.
</p>

<p>
The data filter restricts the data to only those stations for which there is a
<em>data variable</em> matching the filter.
</p>

<p>
The attribute filter restricts the data to only those values for which there is
<em>an attribute</em> matching the filter.
</p>

<p>
The format of the filter is like <tt>B12001>=10</tt>, and consists of a
variable code, a comparison operator (&lt;, &lt;=, =, &gt;=, &gt;, &lt;&gt;), and a value.  
</p>

<p>
It is also possible to specify a range with the syntax: <tt>10&lt;=B12001&lt;=20</tt>.
</p>

<p>
Aliases can be used instead of the variable codes.  For example:
<tt>1000&lt;=height&lt;=2000</tt>.
</p>
""");

        filterPanel = wx.Panel(self)

        self.anaFilter = QueryTextCtrl(filterPanel, model, "ana_filter", lambda x: model.setAnaFilter(x), self.validateFilter)
        self.dataFilter = QueryTextCtrl(filterPanel, model, "data_filter", lambda x: model.setDataFilter(x), self.validateFilter)
        self.attrFilter = QueryTextCtrl(filterPanel, model, "attr_filter", lambda x: model.setAttrFilter(x), self.validateFilter)

        sizer = wx.GridBagSizer()

        sizer.Add(wx.StaticText(filterPanel, -1, "Ana filter: "), (0, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self.anaFilter, (0, 1), (1, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)
        sizer.Add(wx.StaticText(filterPanel, -1, "Data filter: "), (1, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self.dataFilter, (1, 1), (1, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)
        sizer.Add(wx.StaticText(filterPanel, -1, "Attr filter: "), (2, 0), (1, 1), flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self.attrFilter, (2, 1), (1, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)
        sizer.AddGrowableCol(1)

        filterPanel.SetSizerAndFit(sizer)

        box = wx.BoxSizer(wx.VERTICAL)
        box.Add(filterPanel, 0, flag = wx.EXPAND)
        box.Add(self.intro, 1, flag = wx.EXPAND)
        self.SetSizerAndFit(box)


        self.Bind(wx.EVT_CLOSE, self.onClose)

    def validateFilter(self, str):
        # Accept the empty string
        if len(str.strip()) == 0: return True
        # Match the filter format
        match = self.validateExpression1.match(str)
        if not match:
            match = self.validateExpression2.match(str)
        if not match: return False
        # See if the B codes are in the valid range
        bmatch = self.bcodeExpression.match(match.groups()[0])
        if bmatch:
            x, y = map(int, bmatch.groups())
            if x > 63: return False
            if y > 255: return False
        else:
            try:
                info = dballe.Varinfo.create(match.groups()[0])
            except:
                return False
        return True

    def onClose(self, event):
        # Hide the window
        self.Hide()
        # Don't destroy the window
        event.Veto()
        # Notify parent that we've been closed
        self.parent.filtersHasClosed()
