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

import java.awt.*;

/** 
 * This class provides methods for calculating the size of the 
 * screen which Triana is currently running on. Based on the size
 * Unit Programmers can compensate when creating Unit Windows so that
 * they appear relavive to the size of the screen and not way out
 * of proportion. The ratio's are calculated relative to a machine
 * running a 1024 x 768 computer. If the machine has a better resolution
 * than this then Triana will appear larger so that it occupies the same
 * proportion than it does on an SVGA screen.</p>
 *
 * <p>Triana uses this class extensively when creating its graphical user
 * interface.  </p>
 *
 * @version 1.0 16 Jan 1997
 * @author Ian Taylor
 * @see triana.gui.Run
 * @see triana.ocl.OCL
 */
public class Display implements Debug {
    /**
     * The ratio to scale the screen in the X direction.
     */
    public static double ratioX;


    /**
     * The ratio to scale the screen in the Y direction.
     */
    public static double ratioY;

    /**
     * The size of the screen (in pixels) in the X direction.
     */
    public static int screenX;

    /**
     * The size of the screen (in pixels) in the Y direction.
     */
    public static int screenY;


    static  {
       
        // calculate screen size

        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        screenX = dim.width;
        screenY = dim.height;

       // looks good on a SVGA screen so scale so that everything looks 
        // like this.

        ratioX =  (float)screenX / 1024.0;
        ratioY =  (float)screenY / 768.0;

        // However on the notebook I'll experiment :-

        if ((screenX == 640) || (screenY == 480)) {
            ratioX = 0.8;
            ratioY = 0.7;
            }
             
	if (_debug > 5) {
	  System.out.println ( "Screen size is " + screenX + 
			       " by " + screenY );

	  System.out.println ( "RatioX = " + ratioX + " RatioY " + ratioY);   
	}
    } 


    /**
     * This routine calibrates the X size or coordinate sent to it, so that
     * things look the same no matter what computer you are on i.e. it scales
     * the application to the size of this computer's screen.
     */
    public static int x(int size) {
        int newX = (int)( (double)size * ratioX ); 

        // add one if number gets truncated

        if (newX != ( (double)size * ratioX ) ) 
            ++newX;

        return newX;
        }

    /**
     * This routine calibrates the Y size or coordinate sent to it, so that
     * things look the same no matter what computer you are on i.e. it scales
     * the application to the size of this computer's screen.
     */
    public static int y(int size) {
        int newY = (int)( (double)size * ratioY ); 

        // add one if number gets truncated

        if (newY != ( (double)size * ratioY ) ) 
            ++newY;

        return newY;
        }

    /**
     * Makes sure that the specified Frame doesn't dissappear off the 
     * screen by checking the x and y coordinates of its desired 
     * position and clipping them so that it
     * fits onto the particular screen. Unit Programmers should use
     * this function to place their displaying windows on the screen.
     * Subclasses of UnitWindow don't need to call this as it is
     * called automatically.
     * @param fr the frame to be clipped into the screen size
     * @param x the desired x coordinate
     * @param y the desired y coordinate
     */
    public static Point clipFrameToScreen( Window fr, int x, int y) {
        int clipX, clipY;
        int scrY = screenY;

        if (System.getProperty("os.name").equals("Windows 95"))
            scrY = screenY - 20; // for Window 95 tool bar

        if ( (x + fr.getSize().width + 5) > screenX )
            clipX = screenX - fr.getSize().width - 5;
        else
            clipX = x;

        if ( (y + fr.getSize().height + 3) > scrY )
            clipY = scrY - fr.getSize().height - 3;
        else
            clipY = y;

        return new Point(clipX, clipY);
        }

    /**
     * Makes sure that the specified Frame doesn't dissappear off the 
     * screen by checking the x and y coordinates of its desired 
     * position and clipping them so that it
     * fits onto the particular screen. Unit Programmers should use
     * this function to place their displaying windows on the screen.
     * Subclasses of UnitWindow don't need to call this as it is
     * called automatically.
     * @param fr the frame to be clipped into the screen size
     * @param x the desired x coordinate
     * @param y the desired y coordinate
     */
    public static Point clipFrameToScreen( Window fr, Point p) {
        return clipFrameToScreen(fr, p.x, p.y);
        }

    /**
     * Puts the window in the middle of the screen.
     *
     * @param fr the frame to be placed in the middle of the screen
     */
    public static void centralise(Window fr) {
        fr.setLocation( (screenX/2) - (fr.getSize().width/2), 
                          (screenY/2) - (fr.getSize().height/2));
        }

    }
