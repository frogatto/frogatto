#!/usr/bin/env python
# -*- coding: utf-8 -*-
# -> htmhtm - the synched .htm file viewing utility
# -> To run this you will require wxPython.
# -> Put the name of the htm or html files you want to compare in the LeftView and RightView variables.
# -> The right scrollbars effect only their window, the left scrollbars effect both windows.

LeftView = "/home/david/Documents/pots/it.po.html"
RightView = "/home/david/Documents/pots/pt_BR.po.html"



import wx
import  wx.html as  html

class MySplitter(wx.SplitterWindow):
	def __init__(self, parent, ID):
		wx.SplitterWindow.__init__(self, parent, ID,
								   style = wx.SP_LIVE_UPDATE
								   )
		
		self.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGED, self.OnSashChanged)
		self.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGING, self.OnSashChanging)

	def OnSashChanged(self, evt):
		#print "sash changed to %s" % str(evt.GetSashPosition())
		pass

	def OnSashChanging(self, evt):
		#print "sash changing to %s" % str(evt.GetSashPosition())
		# uncomment this to not allow the change
		#evt.SetSashPosition(-1)
		pass
	
		
		
# This shows how to catch the OnLinkClicked non-event.  (It's a virtual
# method in the C++ code...)
class MyHtmlWindow(html.HtmlWindow):
	def __init__(self, parent, id):
		html.HtmlWindow.__init__(self, parent, id, style=wx.NO_FULL_REPAINT_ON_RESIZE)
		if "gtk2" in wx.PlatformInfo:
			self.SetStandardFonts()
		
		self.Bind(wx.EVT_SCROLLWIN, self.OnScroll)
		
	def OnScroll(self, evt):
		#print "scroll " + str(evt.GetPosition())
		if(evt.GetOrientation() == 8):
			self.Scroll(self.GetViewStart()[0], evt.GetPosition())
		else:
			self.Scroll(evt.GetPosition(), self.GetViewStart()[1])
			
	def SetSynch(self, target):
		print "set target " + str(target)
		self.synch = target
		
class MyHtmlSynchWindow(html.HtmlWindow):
	def __init__(self, parent, id):
		html.HtmlWindow.__init__(self, parent, id, style=wx.NO_FULL_REPAINT_ON_RESIZE)
		if "gtk2" in wx.PlatformInfo:
			self.SetStandardFonts()
		
		self.Bind(wx.EVT_SCROLLWIN, self.OnScroll)
		
	def OnScroll(self, evt):
		#print "scroll " + str(evt.GetPosition())
		if(evt.GetOrientation() == 8):
			delta = self.GetViewStart()[1] - evt.GetPosition()
			self.Scroll(self.GetViewStart()[0], self.GetViewStart()[1]-delta)
			self.synch.Scroll(self.synch.GetViewStart()[0], self.synch.GetViewStart()[1]-delta)
		else:
			delta = self.GetViewStart()[0] - evt.GetPosition()
			self.Scroll(self.GetViewStart()[0]-delta, self.GetViewStart()[1])
			self.synch.Scroll(self.synch.GetViewStart()[0]-delta, self.synch.GetViewStart()[1])
			
	def SetSynch(self, target):
		print "set target " + str(target)
		self.synch = target


class Example(wx.Frame):
	def __init__(self, parent, title):
		super(Example, self).__init__(parent, title=title, 
			size=(1200, 800))
			
		self.InitUI()
		self.Show()	 
		
	def InitUI(self):
		splitter = MySplitter(self, -1)
		sty = wx.BORDER_SUNKEN
		

		p1 = wx.Panel(splitter, style=sty)
		p1.SetBackgroundColour("pink")
		#wx.StaticText(p1, -1, "Panel One", (5,5))

		p2 = wx.Panel(splitter, style=sty)
		p2.SetBackgroundColour("sky blue")
		wx.StaticText(p2, -1, "Panel Two", (5,5))
		
		self.html = MyHtmlSynchWindow(p1, 10)
		self.html.LoadPage(LeftView)

		self.box1 = wx.BoxSizer(wx.VERTICAL)
		self.box1.Add(self.html, 1, wx.EXPAND | wx.ALL, 2)
		p1.SetSizer(self.box1)
		
		self.html_ = MyHtmlWindow(p2, 11)
		self.html_.LoadPage(RightView)

		self.box2 = wx.BoxSizer(wx.VERTICAL)
		self.box2.Add(self.html_, 1, wx.EXPAND | wx.ALL, 2)
		p2.SetSizer(self.box2)
		
		self.html.SetSynch(self.html_)
		self.html_.SetSynch(self.html)
		
		splitter.SetMinimumPaneSize(500)
		splitter.SplitVertically(p1, p2, -100)
		
		


if __name__ == '__main__':
  
	app = wx.App()
	Example(None, title='HtmHtm')
	app.MainLoop()
