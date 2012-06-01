package org.csstudio.utility.pv.nds;

//AppFrame.java

import java.awt.*;
import java.io.*;
import java.awt.event.*;
import java.net.*;

//create a frame
class AppFrame extends Frame implements ActionListener {
    Font font1 = new Font("outlet", Font.BOLD, 15); 
    Font font2 = new Font("message", Font.ITALIC, 15); 
    Font font3 = new Font("list", Font.PLAIN, 15);
    List list_selected;
    Button button_save, button_delete, button_clear, button_kill;
    Panel panel1;      // Sub-containers for list and buttons
    TextArea  textarea; // A message window
    FileDialog file_dialog; // file selection box
    ListSelect list_dialog; //self-defined: box showing signal list
    Frame parent;
    URL passedUrl = null;
    App applet;

    public AppFrame(String title, Frame parent_f, int width, int height, URL passedUrl) { 
    // create the Frame with the specified title.
    super(title); 
    parent = parent_f;
    this.passedUrl = passedUrl;

    this.setFont(font1);

    // Create a file selection dialog box
    file_dialog = new FileDialog(this, "File Selection", FileDialog.SAVE);
    
    // Create the list dialog box
    list_dialog = new ListSelect(this, "List Of Signals");

    // Create a multi-line text area
    Label messagelabel = new Label("Messages");
    textarea = new TextArea(6, 40);
    textarea.setEditable(false);

    // Create the cancel button
    button_kill = new Button("Cancel");
    button_kill.setActionCommand("KillGraphics");        
    button_kill.addActionListener(this);              
    /*
    // Add a menubar, with a File menu, with a Quit button.
    MenuBar menubar = new MenuBar();
    Menu menu_main = new Menu("Main", true);
    menubar.add(menu_main);
    MenuItem mi = new MenuItem("Cancel");
    mi.setActionCommand("KillGraphics");        
    mi.addActionListener(this);              
    menu_main.add(mi);
    this.setMenuBar(menubar);
    */
    //create  lists and buttons
    list_selected = new List(20, true);
    button_delete = new Button("Delete");
    button_delete.setActionCommand("Delete");        
    button_delete.addActionListener(this);              
    button_save = new Button("Save");
    button_save.setActionCommand("Save");        
    button_save.addActionListener(this);              
    button_clear = new Button("Clear");
    button_clear.setActionCommand("Clear");        
    button_clear.addActionListener(this);              
    //Add components to the panel1 (list of selected signals) 
    panel1 = new Panel();
    panel1.setLayout(null);
    Label label = new Label("List of Selection");
    label.setBounds(70,5,200,30);
    panel1.add(label);
    list_selected.setBounds(10,50,248,280);
    list_selected.setFont(font3);
    panel1.add(list_selected);
    button_save.setBounds(32,345,65,25);
    panel1.add(button_save);
    button_delete.setBounds(102,345,65,25);
    panel1.add(button_delete);
    button_clear.setBounds(172,345,65,25);
    panel1.add(button_clear);
  
    //Final layout
    this.setLayout(null);
    panel1.setBounds(20,310,260,480);
    this.add(panel1);
    messagelabel.setBounds(330,540,150,20);
    this.add(messagelabel);
    textarea.setBounds(330,560,600,120);
    this.add(textarea);
    button_kill.setBounds(940,600,65,25);
    this.add(button_kill);

 
    // Add the applet to the window. Set the window size. Pop it up.
    applet = new App(this, passedUrl);
    applet.setBounds(0,35,1030,750);
    this.add(applet);
    //this.add("Center", applet);
    this.setSize(width, height);
    this.show();
        
    // Start the applet.
    applet.init();
    //applet.start();
    }

    //Display messages
    public void showMessage(String msg) {
      textarea.setFont(font2);
      textarea.setForeground(Color.white);
      textarea.setBackground(Color.gray);
      String constmsg = "Active regions are shaded in yellow color\nLeft mouse button goes to the next level map, right mouse button shows the menu \nwhere you may choose different actions\n\nYou may go to another area from the top level map by using left mouse";
      textarea.setText(msg + "\n\n" + constmsg);
      return;
    }

    //Show the list dialog
    public void showList() {
      list_dialog.show();
      return;
    }

    //Show the list dialog
    public void addList(String[] newnames) {
      for(int i = 0; i < newnames.length; i++) {
	list_dialog.list_choice.addItem(newnames[i]);
      }
      return;
    }

    //Remove all items from the list dialog
    public void removeList() {
      list_dialog.list_choice.removeAll();
      return;
    }

    //Add new signals to the list
    public void addSig(String[] sigstring) {
      int listsize, addit;
      for (int i=0; i<sigstring.length; i++) {
	addit = 1;
	listsize = list_selected.getItemCount();
	if (listsize > 0) {
	  for (int j=0; j<listsize; j++) {
	    if (sigstring[i].compareTo(list_selected.getItem(j)) == 0) {
	      addit = 0;
	      break;
	    }
	  }
	}
	if (addit == 1)
	  list_selected.addItem(sigstring[i]);
      }
      return;
    }

    //Delete selected signals from the list
    public void deleteSig() {
      int sigidx[];
      sigidx = list_selected.getSelectedIndexes();
      while (sigidx.length > 0) {
	list_selected.delItem(sigidx[0]);
	sigidx = list_selected.getSelectedIndexes();
      }
      return;
    }

    //Save selected signals to a file
    public void saveSig() {
      //file_dialog.pack();  
      file_dialog.show();  
      if (file_dialog.getFile() == null)
	return;
      if (file_dialog.getFile().compareTo("") == 0) {
	System.out.println("No File Is Selected");
	return;
      }
      String filename = file_dialog.getDirectory() + file_dialog.getFile();
      try{
	FileWriter savefile = new FileWriter(filename);
	String line;
	for (int i=0; i<list_selected.getItemCount(); i++) {
	  line = list_selected.getItem(i);
	  savefile.write(line, 0, line.length());
	  savefile.write("\n", 0, 1);
	}
	savefile.close();
      }
      catch(Exception e){
	System.err.println("!!!Cann't get filewriter:" + e);
      }
      return;
    }


    // Handle the events in Frame
    public void actionPerformed(ActionEvent event) {
      String command = event.getActionCommand();
      if (command.equals("Delete")) { 
	deleteSig();
      }
      else if (command.equals("Save")) {
	saveSig();
      }
      else if (command.equals("Clear")) {
	list_selected.removeAll();
      }
      else if (command.equals("KillGraphics")) {
	dispose();
	//this.setVisible(false);
	//this.dispose(); --only dispose the menubar -- why?
	//parent.frame_graphics.removeAll();
	//System.exit(0);
      }
    }

  /*
  public static void main(String[] args) {
    Frame frame = new AppFrame("Graphic Selection Panel",1030,850);
    frame.pack();
    frame.show();
  }
  */
}


//Class of the list selection
class ListSelect extends Dialog implements ActionListener{
    protected Button button_add, button_cancel;
    protected List list_choice;
    AppFrame parentFrame;
    
    public ListSelect(Frame parent, String title)
    {
        // Create the window.
        super(parent, title, true);
	parentFrame = (AppFrame)parent;
	//create  lists and buttons
	list_choice = new List(12, true);
	button_add = new Button("Add to List");
	button_add.setActionCommand("Add to List");        
	button_add.addActionListener(this);              
	button_cancel = new Button("Cancel");
	button_cancel.setActionCommand("Cancel");        
	button_cancel.addActionListener(this);              

	//Layout
	Font font3 = new Font("list", 0, 15);
	this.setLayout(null);
	list_choice.setBounds(10,30,250,400);
	list_choice.setFont(font3);
	this.add(list_choice);
	button_add.setBounds(40,450,90,25);
	this.add(button_add);
	button_cancel.setBounds(150,450,70,25);
	this.add(button_cancel);
	this.setSize(270, 495);

    }

    //Add selected signals to the list
    public void addSelected() {
      String sigstring[] = list_choice.getSelectedItems();
      parentFrame.addSig(sigstring);
      return;
    }

    //Event handeling
    public void actionPerformed(ActionEvent event) {
      String command = event.getActionCommand();
      if (command.equals("Add to List")) { 
	addSelected();
      }
      else if (command.equals("Cancel")) {
	this.setVisible(false);
      }
    }


}
//end of class ListSelect

