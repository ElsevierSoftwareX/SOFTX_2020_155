package org.csstudio.utility.pv.nds;

/*
 * Copyright (c) 1995, 1996, 1997 University of Wales College of Cardiff
 *
 * Permission to use and modify this software and its documentation for
 * any purpose is hereby granted without fee provided a written agreement
 * exists between the recipients and the University.
 *
 * Further conditions of use are that (i) the above copyright notice and
 * this permission notice appear in all copies of the software and
 * related documentation, and (ii) the recipients of the software and
 * documentation undertake not to copy or redistribute the software and
 * documentation to any other party.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF WALES COLLEGE OF CARDIFF BE LIABLE
 * FOR ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 */

/** 
 * QuitWindow creates a basic modal window with two buttons, labelled
 * <i>yes</i> and know <i>no</i>.  The user must then enter a choice before
 * he/she may continue.  A specified label indicates the purpose. This window
 * is used to implement Trianas "Really want to Quit" window and hence its name.
 *
 * @version 1.0 16 Jan 1997
 * @author Ian Taylor
 */

import java.awt.*;
import java.awt.event.*;

public class ErrorDialog
  extends ClosableDialog implements ActionListener {

  /**
     * This class creates a basic modal window with two buttons, labelled
     * <i>yes</i> and know <i>no</i> and with a specified title.  It sets
     * itself modal because thats what it will always be.  The reply 
     * variable hold the answer to what the user chose.
     */
  public ErrorDialog (Frame parent, String text) {
    this (parent, "Error!", text);
  }

  /**
   * This class creates a basic modal window for Errors mainly. They
   * display an error message along with a specified title.  It sets
   * itself modal because thats what it will always be.  The user  
   * can specify the name of the window here if the default error! is
   * not approprate.  
     */
  public ErrorDialog(Frame parent, String title, String text) {
    super((parent!=null) ? parent : new Frame(""), title, true);

    setLayout (new BorderLayout());
    Panel buttons = new Panel (); 
    buttons.setLayout (new BorderLayout ());
    Button b = new Button("OK");
    b.addActionListener (this);
    buttons.add ("East", b);

    StringVector split = Str.splitText (text);
        
    int max = 0;

    for (int i=0; i<split.size(); ++i)
      if (split.at(i).length() > max)
	max = split.at(i).length();

    TextArea t = new TextArea(text, split.size(), max, TextArea.SCROLLBARS_NONE);
    t.setEditable(false);

    add("Center", t);
    add("South", buttons);

    pack();
    setResizable(false);
    Display.centralise(this);
    requestFocus();
    show();
  }

  public void actionPerformed(ActionEvent e) { performEnterAction (); }
  public void performEnterAction () { close (); }    
}
