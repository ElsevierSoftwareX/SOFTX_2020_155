#!/usr/bin/env python

import epics
from Tkinter import *
import Tkconstants, tkFileDialog
from epics import PV
import sys, argparse
import time
    
class TkFileDialogExample(Frame):
    
	def __init__(self, root,site,ifo,model,dcuid):
     		self.dir = '/opt/rtcds/' + site + '/' + ifo + '/target/' + model + '/' + model + 'epics/burt/' 
		self.defaultfile = model + '_safe.snap'
		self.dcuid = 'H1:FEC-' + dcuid
    
		Frame.__init__(self, root)
    
		self.root = root
		title = model + ' BURT'
		self.root.title(title)
		# options for buttons
		#button_opt = {'fill': Tkconstants.BOTH, 'padx': 5, 'pady': 5}
		button_opt = {'padx': 20, 'pady': 10}
		frame_opt = {'padx': 20, 'pady': 30}
   
		# Configure the various selection buttones
		lftitle = model.upper() + ' BURT - Select File'
		self.mlabel1 = LabelFrame(self,text=lftitle)
		self.mlabel1.pack(fill="both", expand="yes", **frame_opt)
		self.mlabel1.configure(bg = 'light grey')
		self.b1 = Button(self.mlabel1, text='Select BURT File', command=self.askopenfilename)
		self.b1.pack(**button_opt)
		self.b1.configure(bg = 'wheat')

		lftitle = model.upper() + ' BURT - Load Settings'
		self.mlabel2 = LabelFrame(self,text=lftitle)
		self.mlabel2.pack(fill="both", expand="yes", **frame_opt)
		self.mlabel2.configure(bg = 'light grey')
		self.b2 = Button(self.mlabel2, text='Load BURT File', command=self.loadfile)
		self.b2.pack(**button_opt)
		self.b2.configure(bg = 'wheat')

		lftitle = model.upper() + ' BURT - Read Only'
		self.mlabel3 = LabelFrame(self,text=lftitle)
		self.mlabel3.pack(fill="both", expand="yes", **frame_opt)
		self.b3 = Button(self.mlabel3, text='Read BURT File', command=self.readfile)
		self.b3.pack(**button_opt)
		self.b3.configure(bg = 'wheat')

		lftitle = model.upper() + ' BURT - RESET'
		self.mlabel4 = LabelFrame(self,text=lftitle)
		self.mlabel4.pack(fill="both", expand="yes", **frame_opt)
		self.b4 = Button(self.mlabel4, text='BURT Reset', command=self.resetsettings)
		self.b4.pack(**button_opt)
		self.b4.configure(bg = 'wheat')

		ename = self.dcuid + '_SDF_LOADED'
		print 'Loaded file is ',ename
		e = PV(ename)
		pf = e.value
   
		# Add file to be loaded to LOAD button text
		self.b2.configure(text = pf)
		# define options for opening or saving a file
		self.file_opt = options = {}
		options['defaultextension'] = '.snap'
		options['filetypes'] = [('burt files', '.snap'),('all files', '.*')]
		options['initialdir'] = self.dir
		options['parent'] = root
		options['title'] = 'Select BURT File'
   
	   
		# defining options for opening a directory
		self.dir_opt = options = {}
		options['initialdir'] = self.dir
		options['mustexist'] = False
		options['parent'] = root
		options['title'] = 'Select BURT File '

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
   
	def loadfile(self):
   
	       """Sends load signal to FE code and exits BURT GUI"""
	       ename = self.dcuid + '_SDF_RELOAD'
	       print 'Sending Load Command ',ename
	       e = PV(ename)
	       e.value = 1
	       time.sleep(2)
	       self.quit()
   
	def readfile(self):
   
	       """Sends Read Only command to FE and closes BURT GUI"""
	       ename = self.dcuid + '_SDF_RELOAD'
	       print 'Sending Read command in ',ename
	       e = PV(ename)
	       e.value = 2
	       time.sleep(2)
	       self.quit()
   
	def resetsettings(self):
   
	       """Sends Reset command to FE and closes BURT GUI"""
	       ename = self.dcuid + '_SDF_RELOAD'
	       print 'Sending Reset ',ename
	       e = PV(ename)
	       e.value = 3
	       time.sleep(2)
	       self.quit()
   
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
   
# Main Routine
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
     root.destroy()
