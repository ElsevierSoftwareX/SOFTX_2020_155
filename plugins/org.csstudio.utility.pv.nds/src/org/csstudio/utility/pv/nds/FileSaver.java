package org.csstudio.utility.pv.nds;

import java.io.*;
import java.awt.*;
import java.util.*;

public abstract class FileSaver implements Debug {
  public abstract boolean saveFile (DataBlock block);
  public abstract void startSavingSession ();
  public abstract void endSavingSession ();
}
