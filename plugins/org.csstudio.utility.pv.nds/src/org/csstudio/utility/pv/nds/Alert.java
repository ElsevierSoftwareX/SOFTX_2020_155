package org.csstudio.utility.pv.nds;

/*
 * Copyright (c) 1995 - 1998 University of Wales College of Cardiff
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
import java.awt.event.ActionEvent;
import java.util.*;
import java.io.*;

/**
 * The window allows an arror message to be displayed in a window. The message
 * is a String which can contain several lines. This functions splits the input
 * up into seperate lines and displays them centrally in a modal window
 * and puts an ok on the bottom of the window which the user clicks when
 * he/she ahs read the message.
 *
 * @version 1.0 24 March 1998 
 * @author Ian Taylor
 *
 */
public class Alert extends ErrorDialog {
    /**
     * Creates an Alert window which prints a formatted message onto
     * the upper part of the window and an OK button below it. 
     *
     * @param parent the frame the modal window belongs to. If this is null
     * a dummy frame is made for your convenience.
     * @param text the text to be displayed. This can consist of several 
     * lines of explanation.
     *
     */
    public Alert(Frame parent, String text) {
        super(parent, "Alert!", text);
        }
    }
