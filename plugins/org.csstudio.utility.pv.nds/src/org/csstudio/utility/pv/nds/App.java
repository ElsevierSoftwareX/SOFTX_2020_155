package org.csstudio.utility.pv.nds;

//App.java
//Top Model Test

//import java.applet.*;
import java.awt.*;
import java.net.*;
import java.util.*;
import java.lang.System.*;
import java.io.*;
import java.awt.image.*;
import java.awt.event.*;

import java.io.File;
import org.w3c.dom.*;
import com.sun.xml.parser.Resolver;
import com.sun.xml.tree.XmlDocument;
import com.sun.xml.tree.XmlDocumentBuilder;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;


public class App extends Panel implements ActionListener {
    static AppFrame frame;
    protected URL add; //url address for images
    protected String base; //base directory
    protected Hashtable imageTable;
    protected Image image_top, image_act;      
    protected Vector top_rects, lower_rects; 
    protected int xpos0=10, ypos0=40, xpos=300, ypos=40, wpos=700, hpos=460;
    protected int firsttime = 0, level=0;      
    protected ActiveRect active_rect, //used for third mouse clicking
              highlight_rect=null, //used for toplevel highlighting
              last_rect; //for mouse-clicking, highlighting
    protected int xshift, yshift; //for mouse-clicking image-shifting   
    protected PopupMenu popup;
    protected MenuItem m0,m1,m2,m3;   
    String[] labels = new String[] {
      "Get All Signals",
      "View Signal List",
      "Next Level",
      "Previous Level"
    };

    URL passedUrl = null;
    XmlDocument	doc;


    public App(AppFrame fr, URL passedUrl) {
      frame = fr;
      this.passedUrl = passedUrl;
    }


    public void init() {
        //get the image directory
        base =
	  (passedUrl == null
	   ? "file:" + System.getProperty("user.dir") + System.getProperty("file.separator")
	   : passedUrl.toString ());

        //String base = "file:/home/hding/Try/Java/Ligo/";
        //String base = "http://www.ligo.caltech.edu/~hding/Java/Ligo/";
	System.out.println("Path is " + base);
	imageTable = new Hashtable();

	//initial rectangular file, fix the base path
        try {
	  // turn the filename into an input source
	  InputSource input = Resolver.getSource (new URL(base+"rectfile.xml"), true);
	  // turn it into an in-memory object
	  // ... the "false" flag says not to validate
	  doc = XmlDocumentBuilder.createXmlDocument (input, false);
	}
	catch(SAXParseException err) {
	  System.out.println ("** Parsing error" 
			      + ", line " + err.getLineNumber()
			      + ", uri " + err.getSystemId());
	  System.out.println("   " + err.getMessage());
	  // print stack trace as below
	} 
	catch (SAXException e) {
	  Exception  x = e.getException();
	  ((x == null) ? e : x).printStackTrace();
	} 
	catch (Throwable t) {
	  t.printStackTrace();
	  base = "file:" + System.getProperty("user.dir") 
	    + System.getProperty("file.separator");
	  System.out.println("Cann't get rectfile.xml from above path. Try path " + base+"rectfile.xml");
	  try {
	    InputSource input = Resolver.getSource (new URL(base+"rectfile.xml"), true);
	    doc = XmlDocumentBuilder.createXmlDocument (input, false);
	  }
	  catch(SAXParseException err) {
	    System.out.println ("** Parsing error" 
			      + ", line " + err.getLineNumber()
			      + ", uri " + err.getSystemId());
	    System.out.println("   " + err.getMessage());
	  } 
	  catch (SAXException e) {
	    Exception  x = e.getException();
	    ((x == null) ? e : x).printStackTrace();
	  } 
	  catch (Throwable t1) {
	    t1.printStackTrace();
	    System.out.println("Cann't get file: " + base+"rectfile.xml... Quit");
	    frame.dispose();
	  }
	}

	//initial rectangular areas, load the toplevel image
	try{
	  ActiveRect rect = new ActiveRect("Top Level", doc);
	  String tempname = rect.getImage();
	  add = new URL(base + tempname);
	  image_top =  createImage((ImageProducer) add.getContent());
	  top_rects =  rect.getNextLevelChildren();
	  rect = rect.getFirstChildRect();
	  //show the initial message
	  frame.showMessage("You are now at the area: " + rect.getName());
	  tempname = rect.getNextImage();
	  lower_rects = rect.getNextLevelChildren();
	  add = new URL(base + tempname);
	  image_act =  createImage((ImageProducer) add.getContent());
	  imageTable.put(tempname, image_act);
	}
	catch(Exception e){
	    System.err.println("Image Creation:" + e);
	}
	//Add a popup menu
	popup = new PopupMenu();
        m0 = new MenuItem(labels[0]);   
	m0.setActionCommand(labels[0]);        
	m0.addActionListener(this);              
	popup.add(m0);
        m1 = new MenuItem(labels[1]);   
	m1.setActionCommand(labels[1]);        
	m1.addActionListener(this);              
	popup.add(m1);
        m2 = new MenuItem(labels[2]);   
	m2.setActionCommand(labels[2]);        
	m2.addActionListener(this);              
	popup.add(m2);
        m3 = new MenuItem(labels[3]);   
	m3.setActionCommand(labels[3]);        
	m3.addActionListener(this);              
	popup.add(m3);
	this.add(popup);

        this.enableEvents(AWTEvent.MOUSE_EVENT_MASK);

        Graphics g = this.getGraphics();
	update(g);
    }
    
    // Called when the applet is being unloaded from the system.
    // We use it here to "flush" the image. This may result in memory 
    // and other resources being freed quicker than they otherwise would.
    public void destroy() {
      System.out.println( "destroy() called" );
      image_act.flush(); 
      image_top.flush(); 
    }
    
    // Display the image.
    public void paint(Graphics g) {
      int i;
      Font font1 = new Font("myfont", Font.BOLD, 15);
      g.setFont(font1);
      g.drawString("Top Level Map", 82, 15);
      g.drawString("More Detailed Map", 340, 15);
      g.drawImage(image_top, xpos0, ypos0, this);
      g.drawImage(image_act, xpos, ypos, this);
      g.setPaintMode();
      if (firsttime == 0) {
	highlight_rect = (ActiveRect)top_rects.elementAt(0);
	firsttime = 1;
      }
      g.setColor(Color.red);
      g.drawRect(highlight_rect.x+xpos0, highlight_rect.y+ypos0, highlight_rect.width, highlight_rect.height);
    }
    
    // Display the image.
    public void showGraph(Graphics g, String graphname) {
      Image image = (Image)imageTable.get(graphname);
      if(image == null) {
	System.out.println("creating new image "+ graphname );
	try{
	  add = new URL(base + graphname);
	  image_act =  createImage((ImageProducer) add.getContent());
	  imageTable.put(graphname, image_act);
	}
	catch(Exception e){
	  System.err.println("showGraph:Image " + graphname + " Creation:" + e);
	}
      }
      else 
	image_act = image; 
      g.setPaintMode();
      g.drawImage(image_act, xpos, ypos, this);
    }


    // We override this method so that it doesn't clear the background
    // before calling paint().  Makes for less flickering in some situations.
    public void update(Graphics g) { 
      paint(g); 
    }
    
    // find the rectangle we're inside
    private ActiveRect findrect(int x, int y) {
        int i;
        ActiveRect r = null;
        for(i = 0; i < top_rects.size(); i++)  {
            r = (ActiveRect)top_rects.elementAt(i);
            if (r.contains(x-xpos0, y-ypos0)) {
	      break;
	    }
        }
        if (i < top_rects.size()) {
	  level = 0; 
	  return r;
	}
	else {
	  for(i = 0; i < lower_rects.size(); i++)  {
            r = (ActiveRect)lower_rects.elementAt(i);
            if (r.contains(x-xpos, y-ypos)) {
	      break;
	    }
	  }
	  if (i < lower_rects.size()) {
	    level = 1; 
	    return r;
	  }

	}
	return null;
    }
    


    //This is the ActionListener method invoked by the popup menu items 
     public void actionPerformed(ActionEvent event) {
       String command = event.getActionCommand();
       Graphics g = this.getGraphics();
       ActiveRect ar = null;
       Vector temp_rects;
       if (command.equals(labels[0])) { //Get All Signals
         if (active_rect == null) {
	   for (int i=0; i < lower_rects.size(); i++) {
	     ar = (ActiveRect)lower_rects.elementAt(i);
	     frame.addSig(ar.getSigList());
	   }
	 }
	 else if (level == 1){
	   frame.addSig(active_rect.getSigList());
	 }
	 else if (level == 0){
	   temp_rects = active_rect.getNextLevelChildren(); 
	   for ( int i=0; i<temp_rects.size(); i++ ) {
	     ar = (ActiveRect)temp_rects.elementAt(i);
	     frame.addSig(ar.getSigList());
	   }
	 }
       }
       else if (command.equals(labels[1])) { //View Signal List
	 frame.removeList();
         if (active_rect == null) {
	   for (int i=0; i < lower_rects.size(); i++) {
	     ar = (ActiveRect)lower_rects.elementAt(i);
	     frame.addList(ar.getSigList());
	   }
	 }
	 else if (level == 1){
	   frame.addList(active_rect.getSigList());
	 }
	 else if (level == 0){
	   temp_rects = active_rect.getNextLevelChildren(); 
	   for (int i=0; i < temp_rects.size(); i++) {
	     ar = (ActiveRect)temp_rects.elementAt(i);
	     frame.addList(ar.getSigList());
	   }
	 }
	 frame.showList();
       }
       else if (command.equals(labels[2])) { //Next Level
	 temp_rects = active_rect.getNextLevelChildren();
	 if ( temp_rects == null || temp_rects.size() == 0) 
	   System.out.println("this is in final level");
	 else {
	   lower_rects = temp_rects;
	   String nextgraph = active_rect.getNextImage();
	   showGraph(g, nextgraph); 
	   System.out.println("Next Level: " + nextgraph);
	 }
       }
       else if (command.equals(labels[3])) { //Previous Level
         if (active_rect == null) {
	   ar = (ActiveRect)lower_rects.elementAt(0);
	 }
	 else {
	   ar = active_rect;
	 }
	 if ( ar.getLevel() <= 1)
	   System.out.println("these is no previous level");
	 else {
	   ar = ar.getParent();
	   lower_rects = (ar.getParent()).getNextLevelChildren();
	   showGraph(g, ar.getImage()); 
	   System.out.println("Previous Level: " + ar.getImage());
	 }
       }
     }


     public void processMouseEvent(MouseEvent e) {
       if (e.isPopupTrigger()) {    // If popup trigger,
         active_rect = findrect(e.getX(), e.getY());
	 if (active_rect == null) {
	   Rectangle r = new Rectangle(xpos,ypos,wpos,hpos);
	   if (r.contains(e.getX(), e.getY())) {
	     m2.setEnabled(false);
	     popup.show(this, e.getX(), e.getY()); 
	   }
	   return;
	 }
	 if (level == 0) {
	   m2.setEnabled(false);
	   m3.setEnabled(false);
	 }
	 else {
	   if (active_rect.getNextLevelChildren() == null)
	     m2.setEnabled(false);
	   else
	     m2.setEnabled(true);
	   m3.setEnabled(true);
	 }
	 popup.show(this, e.getX(), e.getY());   
       }
       else if (e.getID() == MouseEvent.MOUSE_PRESSED) {
	    ActiveRect r = findrect(e.getX(), e.getY());
	    if (r == null) return;
	    Graphics g = this.getGraphics();
	    g.setColor(Color.red);
	    if (level == 0) {
	      xshift = xpos0;  yshift = ypos0;
	    }
	    else {
	      xshift = xpos;  yshift = ypos;
	    }
	    g.drawRect(r.x+xshift, r.y+yshift, r.width, r.height);
	    last_rect = r;
       }
       else if (e.getID() == MouseEvent.MOUSE_RELEASED) {
            if (last_rect != null) {
	      Graphics g = this.getGraphics();
	      g.setColor(Color.yellow);
	      g.drawRect(last_rect.x+xshift, last_rect.y+yshift, last_rect.width, last_rect.height);
	      ActiveRect r = findrect(e.getX(), e.getY());
	      if ((r != null) && (r == last_rect)) {
		g.setColor(Color.red);
		g.drawRect(r.x+xshift, r.y+yshift, r.width, r.height);
		if (level == 0) {
		  if ((highlight_rect != null) && (highlight_rect != r)) {
		    g.setColor(Color.yellow);
		    g.drawRect(highlight_rect.x+xpos0, highlight_rect.y+ypos0, highlight_rect.width, highlight_rect.height);
		  }
		  highlight_rect = r;
		  lower_rects = highlight_rect.getNextLevelChildren();
		  showGraph(g, highlight_rect.getNextImage());
		  frame.showMessage("You are now at the area: "+highlight_rect.getName());
		}
		else if (level == 1) {
		  Vector temp_rects = r.getNextLevelChildren();
		  if ( temp_rects == null || temp_rects.size() == 0) 
		    System.out.println("this is in final level");
                  else {
		    lower_rects = temp_rects;
		    String nextgraph = r.getNextImage();
		    showGraph(g, nextgraph); 
		    System.out.println("Next Level: " + nextgraph);
                  }
		}
	      }
	      last_rect = null;
	      xshift = 0;
	      yshift = 0;
	    }
       }
       else super.processMouseEvent(e);  // Pass other event types on.
  }

}


//*******Class presents active rectanglers******
class ActiveRect extends Rectangle {
    Node rectNode;
    int level;

    static Node checkedNode;
    static int breakPt;

    public ActiveRect(Node rect) {
      //super(getRectX(rect),getRectY(rect),getRectW(rect),getRectH(rect));
      x = getRectX(rect);
      y = getRectY(rect);
      width = getRectW(rect);
      height = getRectH(rect);
      level = getRectLevel(rect);
      rectNode = rect;
    }
    
    public ActiveRect(String name, XmlDocument doc) {
      breakPt = 0;
      checkedNode = null;
      checkAll(doc.getDocumentElement(), name);
      rectNode = checkedNode;
      level = getRectLevel(rectNode);
      if ( level >= 0 ) {
	x = getRectX(rectNode);
	y = getRectY(rectNode);
	width = getRectW(rectNode);
	height = getRectH(rectNode);
      }
      else {
	x = 0;
	y = 0;
	width = 1;
	height = 1;
      }
    }
    
    //public functions
    public int getLevel() {
      return level;
    }

    public String getName() {
      Node nn = getChildNode(rectNode, "Id");
      return ((Element)nn).getAttribute("name");
    }

    public String getImage() {
      Node nn = getChildNode(rectNode, "Id");
      return ((Element)nn).getAttribute("image");
    }

    public String getNextImage() {
      Node nn = getChildNode(rectNode, "Rect");
      if (nn == null)
	nn = rectNode;
      nn = getChildNode(nn, "Id");
      return ((Element)nn).getAttribute("image");
    }

    public String[] getSigList() {
      Node nn = getChildNode(rectNode, "SignalList");
      if (nn == null) return null;
      if (nn.hasChildNodes() == false)  return null;
      NodeList list = nn.getChildNodes();
      String[] tempst = new String[list.getLength()];
      for (int i = 0; i < list.getLength(); i++) {
	tempst[i] = ((Element)list.item(i)).getAttribute("name");
      }
      return tempst;
    }

    public ActiveRect getParent() {
      Node parent = rectNode.getParentNode();
      if (parent == null) return null;
      return (new ActiveRect(parent));
    }


    public Vector getAllChildren() {
      NodeList list = ((Element)rectNode).getElementsByTagName("Rect");
      Vector vect = new Vector();
      for (int i = 0; i < list.getLength(); i++) {
	vect.addElement(new ActiveRect(list.item(i)));
      }
      return vect;
    }

    public Vector getNextLevelChildren() {
      NodeList list = ((Element)rectNode).getElementsByTagName("Rect");
      Vector vect = new Vector();
      for (int i = 0; i < list.getLength(); i++) {
	if ( getRectLevel((Node)list.item(i)) == level+1)
	  vect.addElement(new ActiveRect(list.item(i)));
      }
      return vect;
    }

    public ActiveRect getFirstChildRect() {
      Node child = getChildNode(rectNode, "Rect");
      if (child == null) return null;
      return (new ActiveRect(child));
    }


    //internal functions
    //get the first child node with given nodename
    private Node getChildNode(Node node, String nodename) {
      Node child = null;
      NodeList list = node.getChildNodes();
      for (int i = 0; i < list.getLength(); i++) {
	if (list.item(i).getNodeName().compareTo(nodename) == 0) {
	  child = list.item(i);
	  break;
	} 
      }
      //System.out.println("getChildNode: node= " + child.getNodeName() );
      return child;
    }


    private int getRectX(Node rect) {
      Node nn = getChildNode(rect, "Geometry");
      return Integer.parseInt(((Element)nn).getAttribute("x"));
    }
  
    private int getRectY(Node rect) {
      Node nn = getChildNode(rect, "Geometry");
      return Integer.parseInt(((Element)nn).getAttribute("y"));
    }

    private int getRectW(Node rect) {
      Node nn = getChildNode(rect, "Geometry");
      return Integer.parseInt(((Element)nn).getAttribute("width"));
    }

    private int getRectH(Node rect) {
      Node nn = getChildNode(rect, "Geometry");
      return Integer.parseInt(((Element)nn).getAttribute("height"));
    }

    //level of Rect Top Level is -1
    //level of left Rects is 0
    //level of right Rects start at 1
    private int getRectLevel(Node rect) {
      int lev = -4;
      while (rect != null ) {
	lev++;
        rect = rect.getParentNode();
      }
      return lev;
    }
  
    //check all children to get the Node with given name
    private void checkAll(Node currentNode, String name) {
      if (currentNode == null) {
	System.out.println("checkAll: null");  
	return;
      }
      if (currentNode.getNodeName().compareTo("Rect") == 0) {
	Node child = getChildNode(currentNode, "Id");
	if (((Element)child).getAttribute("name").compareTo(name) == 0) {
	  checkedNode = currentNode;
	  breakPt = 1;
	  return;
	}
      }
      if (currentNode.getNodeType() == Node.ELEMENT_NODE) {
	NodeList list = currentNode.getChildNodes();
	for (int i = 0; i < list.getLength(); i++) {
	  checkAll(list.item(i), name);
	  if (breakPt == 1) break;
	}
      }
      return;
    }

}//end of Class ActiveRect definition
