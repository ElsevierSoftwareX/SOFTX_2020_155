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
 
/**
 * StringVector a Vector which deals only with strings and has
 * shortcut names for the functions elementAt (=at), firstElement
 * (=first) and lastElement (=last).<p>
 * 
 *
 * @version 1.0 30 Jan 1997
 * @author Ian Taylor
 */
public class StringVector extends Vector {
 
    /**
     * Constructs an empty string vector with the specified storage
     * capacity and the specified capacityIncrement.
     * @param initialCapacity the initial storage capacity of the vector
     * @param capacityIncrement how much to increase the element's 
     * size by.
     */
    public StringVector(int initialCapacity, int capacityIncrement) {
	super(initialCapacity,capacityIncrement);	 
        }

    /**
     * Constructs an empty string vector with the specified storage capacity.
     * @param initialCapacity the initial storage capacity of the vector
     */
    public StringVector(int initialCapacity) {
	this(initialCapacity, 0);
        }

    /**
     * Constructs an empty vector.
     */
    public StringVector() {
	this(10);
        }
  
    /**
     * Returns the string element at the specified index.
     * @param index the index of the desired element
     * @exception ArrayIndexOutOfBoundsException If an invalid 
     * index was given.
     */
    public final String at(int index) {
        Object o = super.elementAt(index);
	return o instanceof String ? (String)o : null;
        }

    /**
     * Returns the first element of the sequence.
     * @exception NoSuchElementException If the sequence is empty.
     */
    public final String first() {
	return (String)super.firstElement();
    }

    /**
     * Returns the last element of the sequence.
     * @exception NoSuchElementException If the sequence is empty.
     */
    public final Object last() {
	return (String)super.lastElement();
        }

    /**
     * @return a string representation of this vector in the form :- </p>
     * el1 el2 el3 el4 ..... el(size-1) </p>
     */
    public final String toAString() {
        String s = "";
        for (int i=0; i<size(); ++i) 
            s = s + (String)elementAt(i) + " ";
        s= s + "\n";
        return s;
        }

   /**
     * @return a string representation of this vector in the form :- </p>
     * el1 <br> el2 <br> el3 <br> el4 <br> ..... el(size-1)  </p>
     */
    public final String toNewLineString() {
        String s = "";
        for (int i=0; i<size(); ++i) 
            s = s + (String)elementAt(i) + "\n";
        return s;
        }

    /**
     * @return a a double array representation of this vector. Only
     * useful when you know that the StringVector contains a
     * a list of Strings which you know are all doubles. </p>
     * el1 el2 el3 el4 ..... el(size-1) \n </p>
     */
    public final double[] toDoubles() {
        double[] d = new double[size()];

        for (int i=0; i<size(); ++i) 
            d[i] = Str.strToDouble( ((String)elementAt(i)).trim() );

        return d;
        }
    }
