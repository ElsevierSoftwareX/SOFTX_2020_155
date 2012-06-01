package org.csstudio.utility.pv.nds;

/*
 * Copyright (c) 1995, 1996,1997 University of Wales College of Cardiff
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

import java.util.*;
import java.util.zip.*;
import java.net.URL;
import java.io.*;
import java.awt.Color;

/** 
 * Str is a class which contains static functions which perform a 
 * number of useful repetitive function in java. e.g. when converting from
 * a String to a float, you just simply type in :- </p><p>
 * <center>
 * myfloat = Str.strToFloat(str); <br>
 * instead of<br>
 * myfloat = Float.valueOf(str).floatValue();
 * </center></p>
 * <p> The latter, I find really annoying! The routines here therefore colate
 * a number of frequently performed procedures and provide shortcuts
 * to their operation. </p>
 *
 * Also, there are routines for counting the line numbers and
 * finding and replacing etc.
 *
 * @version 1.0 5 Nov 1997
 *
 * @author Ian Taylor
 * @see triana.ocl.Unit
 * @see triana.ocl.Env
 */
public final class Str {

    private static int count=0;
    private static int pos=0;

    /**
     * Attempts to convert a String to a float
     */
    public static float strToFloat(String str) throws 
                              NumberFormatException, NullPointerException {
        return Float.valueOf(str).floatValue();
        }

    /**
     * Attempts to convert a String to a double
     */
    public static double strToDouble(String str) throws 
                              NumberFormatException, NullPointerException {
        return Double.valueOf(str).doubleValue();
        }

    /**
     * Attempts to convert a String to an int
     */
    public static int strToInt(String str) throws 
                              NumberFormatException, NullPointerException {
        return Integer.valueOf(str).intValue();
        }

    /**
     * Attempts to convert a String to a short
     */
    public static short strToShort(String str) throws 
                              NumberFormatException, NullPointerException {
        return Short.valueOf(str).shortValue();
        }

    /**
     * Attempts to convert a String to a long
     */
    public static long strToLong(String str) throws 
                              NumberFormatException, NullPointerException {
        return Long.valueOf(str).longValue();
        }

    /**
     * Attempts to convert a String to a byte
     */
    public static byte strToByte(String str) throws 
                              NumberFormatException, NullPointerException {
        return Byte.valueOf(str).byteValue();
        }

    /**
     * Attempts to convert a String to a boolean
     */
    public static boolean strToBoolean(String str) throws 
                              NumberFormatException, NullPointerException {
        if (str.trim().equals("true"))
            return true; 
        else
            return false;
        }

    /**
     * @return a "19 Mar 1997" type format of the date. Gets rid of
     * the time etc.
     */
    public final static String niceDate(Date d) {
        StringVector items = new StringVector(10);  // 10 should be OK
 
        String line = d.toString();
 
        StringTokenizer st = new StringTokenizer(line);

        while (st.hasMoreTokens()) {
            items.addElement(st.nextToken());  
            }

        return items.at(2) + " " + items.at(1) + " " + 
                         items.at(items.size()-1);
        }

    /**
     * @return a "19 Mar 1997" type format of the date. Gets rid of
     * the time etc. Give new Date() as argument if the current time
     * and date is needed.
     */
    public final static String niceDateAndTime(Date d) {
        StringVector items = new StringVector(10);  // 10 should be OK
 
        String line = d.toString();
 
        StringTokenizer st = new StringTokenizer(line);

        while (st.hasMoreTokens()) {
            items.addElement(st.nextToken());  
            }

        return items.at(0) + " " + items.at(2) + " " + items.at(1) 
                             + " " + items.at(5) + " " + 
                         items.at(3) + " (" + items.at(4) + ")" ;
        }

   /**
    * Finds the next occurance of the given string <i>text</i> from the   
    * string <i>theString</i> and replaces it with the <i>newText</i>.
    *
    * @param theString the string to scan
    * @param text the text to find within theString
    * @param newText the string to replace <i>text</i> with
    * @return the string after replacing the string
    */
    public static String replaceNext(String theString, String text, 
                                       String newText) {
        pos = theString.indexOf(text, pos);
        if ((pos == -1) || (pos > theString.length()))
            return null;

        theString = theString.substring(0,pos) + 
                       theString.substring(pos + text.length());

        StringBuffer sb = new StringBuffer(theString);
        sb.insert(pos, newText);
        return sb.toString();
        }

    /**
     * Replace all occurances of <i>text</i>  with <i>newText</i> from   
     * <i>theString</i>.
     * @param theString the string to scan
     * @param text the text to find within theString
     * @param newText the string to replace <i>text</i> with
     * @return the new String after all the relevant text has
     * been replaced
     */
    public static String replaceAll(String theString, String text, 
                                                String newText) {
        String str;
        count = 0;
        pos=0;

        while ((str = replaceNext(theString, text, newText)) !=null) {
            ++count;
            theString = str;
	      }

        return theString;
        }

    /**
     * From the specified position in the String this function
     * returns the number of characters <b>excluding line feeds
     * </b> for this position.  TextAreas don't seem to use line
     * feeds for example.
     */
    public static int noLineChars(String str, int pos) {
       int curr=0;
       int old=0;
       int count=0;
       String lineSep = "\n";
       String carrRet = "\r";
       int sepLen = lineSep.length();
       int retLen = carrRet.length();

       curr = str.indexOf(lineSep, old);

       while (( curr < pos) && (curr != -1)) {
           old=curr;
           curr = str.indexOf(lineSep, old);
           if (!str.substring(old, curr).trim().equals(""))
               count += sepLen;
           if (!str.substring(curr-retLen, 
                              curr+sepLen).equals(carrRet+lineSep))
               count += retLen;
           curr += sepLen;
           }

       System.out.println("Offset by " + count);
       return pos - count;
       }

     /**
      * gets number of replacings made by the replaceAll function.
      */
     public static int howMany() {
         return count;
         }

     /**
      * Splits the text by items separated by white space
      */
     public static StringVector splitTextBySpace(String text) {
         return new StringSplitter(text, "whitespace", false);
         }
    /**
     * This function splits the text into a vector of lines
     * contained within that text i.e. splits the text on the 
     * newline character.
     */
    public static StringVector splitText(String text) {
         return new StringSplitter(text, "newline", false);
        }

    /**
     * Prints a vector one element after another with a whitespace
     * between each element.
     */
    public static void printVector(Vector v, PrintWriter pw) {
        for (int i=0; i<v.size(); ++i) {
            if (v.elementAt(i) instanceof URL) {
                URL url = (URL)v.elementAt(i);
                pw.print( url.getProtocol() + "://" + url.getHost() +
                                           url.getFile() + " ");
                }
            else if (v.elementAt(i) instanceof ZipFile) 
                pw.print( ((ZipFile)v.elementAt(i)).getName() + " ");
            else
                pw.print(v.elementAt(i).toString() + " ");
            }
        pw.print("\n");
        }

   /**
    * For printing out the types and associated colours to the user
    * configuration file. Can be also used to print out any hashtable
    * objects in a :- </p>
    * <center> colorname actualcolor<line feed>
    * </center>
    * <p> format
    */
    public static void printHashtable(Hashtable v, PrintWriter pw) {
        int max = v.size() - 1;
        String s1, s2;
        Object el;

        Enumeration k = v.keys();
        Enumeration e = v.elements();

        for (int i = 0; i <= max; i++) {
            s1 = k.nextElement().toString();
            el = e.nextElement(); 
            if (el instanceof Color) {
                Color c = (Color)el;
                s2 = String.valueOf(c.getRed()) + " " +
                             String.valueOf(c.getGreen()) + " " +
                                      String.valueOf(c.getBlue());
                }
            else
                s2 = el.toString();
            pw.print( s1 + " " + s2 + "\n");
	    }
        }
    }
