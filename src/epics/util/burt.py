#!/usr/bin/env python

import epics
from Tkinter import *
import Tkconstants, tkFileDialog
from epics import PV
import sys, argparse
    
class TkFileDialogExample(Frame):
    
	def __init__(self, root,site,ifo,model,dcuid):
     		self.dir = '/opt/rtcds/' + site + '/' + ifo + '/target/' + model + '/' + model + 'epics/burt/' 
		self.defaultfile = model + '_safe.snap'
		self.dcuid = 'H1:FEC-' + dcuid
    
		Frame.__init__(self, root)
    
		# options for buttons
		#button_opt = {'fill': Tkconstants.BOTH, 'padx': 5, 'pady': 5}
		button_opt = {'padx': 10, 'pady': 10}
		frame_opt = {'padx': 10, 'pady': 30}
   
		# define buttons
		self.mlabel = LabelFrame(self,text='FE BURT - File Loaded')
		self.mlabel.pack(fill="both", expand="yes", **frame_opt)

		# Label(self.mlabel, text='PRESENT BURT FILE').pack(**button_opt)
		self.pbf = Label(self.mlabel, text='')
		self.pbf.pack(fill="both", expand="yes", **button_opt)
		ename = self.dcuid + '_SDF_LOADED'
		print 'Loaded file is ',ename
		e = PV(ename)
		pf = e.value
		self.pbf.config(text = pf)

		self.mlabel1 = LabelFrame(self,text='FE BURT - Select File')
		self.mlabel1.pack(fill="both", expand="yes", **frame_opt)
		# self.b1 = Button(self.mlabel, text='askopenfile', command=self.askopenfile).pack(**button_opt)
		self.b2 = Button(self.mlabel1, text='Open CDF', command=self.askopenfilename)
		self.b2.pack(**button_opt)

		self.mlabel2 = LabelFrame(self,text='FE BURT - Load Settings')
		self.mlabel2.pack(fill="both", expand="yes", **frame_opt)
		self.b3 = Button(self.mlabel2, text='Load BURT File', command=self.asksaveasfile).pack(**button_opt)

		self.mlabel3 = LabelFrame(self,text='FE BURT - Read Only')
		self.mlabel3.pack(fill="both", expand="yes", **frame_opt)
		self.b3 = Button(self.mlabel3, text='Read BURT File', command=self.readfile).pack(**button_opt)

		self.mlabel4 = LabelFrame(self,text='FE BURT - Reset')
		self.mlabel4.pack(fill="both", expand="yes", **frame_opt)
		self.b4 = Button(self.mlabel4, text='BURT Reset', command=self.resetsettings).pack(**button_opt)

		ename = self.dcuid + '_SDF_LOADED'
		print 'Loaded file is ',ename
		e = PV(ename)
		pf = e.value
   
		# self.b2.configure(text = "Close CDF")
		self.b2.configure(text = pf)
		# define options for opening or saving a file
		self.file_opt = options = {}
		options['defaultextension'] = '.snap'
		options['filetypes'] = [('burt files', '.snap'),('all files', '.*')]
		options['initialdir'] = self.dir
		# options['initialfile'] = model + '_safe.snap'
		options['parent'] = root
		options['title'] = 'CDF GUI'
   
		# This is only available on the Macintosh, and only when Navigation Services are installed.
		#options['message'] = 'message'
	   
		# if you use the multiple file version of the module functions this option is set automatically.
		#options['multiple'] = 1
	   
		# defining options for opening a directory
		self.dir_opt = options = {}
		options['initialdir'] = self.dir
		options['mustexist'] = False
		options['parent'] = root
		options['title'] = 'CDF GUI'
	def yview(self, *args):
		apply(self.lb1.yview, args)
		#apply(self.lb2.yview, args)
	def askopenfile(self):
   
	       """Returns an opened file in read mode."""
   
	       return tkFileDialog.askopenfile(mode='r', **self.file_opt)
   
	def askopenfilename(self):
   
	       """Returns an opened file in read mode.
	       This time the dialog just returns a filename and the file is opened by your own code.
	       """
   
	       # get filename
	       filename = tkFileDialog.askopenfilename(**self.file_opt)
   
	       # open file on your own
	       if filename:
		 # return open(filename, 'r')
		 	file = open(filename, 'r')
			data = file.read()
			file.close()
			print "I got %d bytes from file." % len(data)
			word = filename.split('/')
			mp = len(word)
			print "I got %d file params. %s %s" % (len(word), word[mp - 2], word[mp-1])
			self.b2.config(text = word[mp-1])
			ename = self.dcuid + '_SDF_NAME'
			e = PV(ename)
			fn = word[mp-1].split('.')
			e.value = fn[0]
			print 'Putting new file name in ',ename,' = ',fn[0]
   
	def asksaveasfile(self):
   
	       """Returns an opened file in write mode."""
	       ename = self.dcuid + '_SDF_RELOAD'
	       print 'Sending new file name in ',ename
	       e = PV(ename)
	       e.value = 1
   
	       # return tkFileDialog.asksaveasfile(mode='w', **self.file_opt)
   
	def readfile(self):
   
	       """Returns an opened file in write mode."""
	       ename = self.dcuid + '_SDF_RELOAD'
	       print 'Sending new file name in ',ename
	       e = PV(ename)
	       e.value = 2
   
	def resetsettings(self):
   
	       """Commands reset of settings marked as care."""
	       ename = self.dcuid + '_SDF_RELOAD'
	       print 'Sending new file name in ',ename
	       e = PV(ename)
	       e.value = 3
   
	def asksaveasfilename(self):
   
	       """Returns an opened file in write mode.
	       This time the dialog just returns a filename and the file is opened by your own code.
	       """
   
	       # get filename
	       filename = tkFileDialog.asksaveasfilename(**self.file_opt)
   
	       # open file on your own
	       if filename:
		 return open(filename, 'w')
   
	def askdirectory(self):
   
	       """Returns a selected directoryname."""
   
	       return tkFileDialog.askdirectory(**self.dir_opt)
   
if __name__=='__main__':
     parser = argparse.ArgumentParser(description='Input Model Info')
     parser.add_argument('-s',dest='site')
     parser.add_argument('-i',dest='ifo')
     parser.add_argument('-m',dest='model')
     parser.add_argument('-d',dest='dcuid')
     args = parser.parse_args()
     root = Tk()
     TkFileDialogExample(root,args.site,args.ifo,args.model,args.dcuid).pack()
     root.mainloop()
