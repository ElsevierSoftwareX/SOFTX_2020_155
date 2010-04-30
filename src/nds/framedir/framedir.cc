#include "framedir.hh"
#include "FrameF.hh"
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <fnmatch.h>

   using namespace std;

   FrameDir::~FrameDir(void) {
      clear();
   }

   FrameDir::FrameDir(void) 
   : mDebug(0), mDirty (false)
   {
   }

   FrameDir::FrameDir(const char* dir, bool delayed) 
   : mDebug(0), mDirty (false)
   {
      add(dir, delayed);
   }

   const ffData& 
   FrameDir::find(const Time& t) const throw(NoData) {
      if (mDirty) checkData();
      file_iterator iter = mList.upper_bound(t.getS());
      if (iter == mList.begin()) throw NoData("Specified data not avaiable");
      return (*(--iter)).second;
   }

   FrameDir::gps_t 
   FrameDir::getStart(gps_t time) const {
      if (mDirty) checkData();
      file_iterator iter = mList.lower_bound(time);
      if (iter == mList.end()) 
         return 0;
      return iter->second.getStartTime().getS();
   }

   FrameDir::gps_t 
   FrameDir::getLast(gps_t time) const {
      if (mDirty) checkData();
      file_iterator iter = mList.upper_bound(time);
      if (iter == mList.begin()) 
         return 0;
      gps_t t = (--iter)->second.getEndGPS();
      while ((iter != mList.end()) && (t == iter->second.getEndGPS())) {
         t = iter->second.getEndGPS();
         iter++;
      }
      return t;
   }

   void
   FrameDir::add(const char* dir, bool delayed) {
      if (!dir || !*dir) 
         return;
      string entry(dir);
   
    //----------------------------------  Add File if no wildcards.
    // cerr << "Directory: " << entry << endl;
      unsigned int wcpos = entry.find_first_of("*[?");
      if (wcpos == string::npos) {
         addFile(entry.c_str());
         if (mDirty && !delayed) checkData();
      } 
      else {
      
         //------------------------------  Find the first name level with a *
         unsigned int last=0, next=0;
         while (next <= wcpos) {
            last = next;
            next = entry.substr(last).find("/");
            if (next == string::npos) next = entry.length()-last;
            next += last + 1;
         }
      
         string direc(entry.substr(0,last));
         if (direc.empty()) direc = ".";
      
      //----------------------------------  Open the directory.
         DIR* dd = opendir(direc.c_str());
         if (!dd) {
            cerr << "Directory " << direc << " is unknown" << endl;
            return;
         }
      
      //------------------------------  Scan for files matching the pattern
         string pattern=entry.substr(last, next-1-last);
      // cerr << "  Test against: " << pattern << endl;
         bool atomic;
         if (next >= entry.length()) {
            atomic = true;
            wcpos  = next;
         } 
         else {
            wcpos  = entry.substr(next).find_first_of("*[?");
            if (wcpos != string::npos) wcpos += next;
            atomic = (wcpos == string::npos);
         }
         for (dirent* dirt=readdir(dd) ; dirt ; dirt = readdir(dd)) {
            if (!fnmatch(pattern.c_str(), dirt->d_name, 0)) {
               string path(entry);
               path.replace(last, next-last-1, dirt->d_name);
            // cerr << "  testing: " << path << endl;
               if (atomic)  addFile(path.c_str());
               else         add(path.c_str(), true);
            }
         }
         closedir(dd);
         if (mDirty && !delayed) checkData();
      }
   }

//=======================================  Add another frame file
   void
   FrameDir::addFile(const char* File) throw(BadFile) {
      if (!File || !*File) 
         return;
      if (getDebug()) cerr << "Adding file: " << File << endl;
   
    //----------------------------------  Parse the file name (time and length)
      const char* ptr = File;
      for (const char* p=ptr ; *p ; p++) if (*p == '/') ptr = p+1;
      while (*ptr && *ptr != '-') ptr++;
      if (!*ptr++) {
	  if (getDebug()) cerr << "File name not standard: " << File << endl;
	  return;
      }
      // check for second non-numeric field
      const char* ptr2 = ptr;
      if (!isdigit (*ptr2)) {
         while (*ptr2 && *ptr2 != '-') ptr2++;
         if (!*ptr2++) {
 	    if (getDebug()) cerr << "File name not standard: " << File << endl;
	    return;
         }
	 ptr = ptr2;
      }
      gps_t time = strtol(ptr, (char**)&ptr, 10);
      gps_t tlen = 0;
      if (*ptr == '-') {
	  ptr++;
	  tlen = strtol(ptr, (char**)&ptr, 10);
      }
      if (!time || (*ptr && *ptr != '.')) {
	  if (getDebug()) cerr << "File name not standard: " << File << endl;
	  return;
      }
      mList[time] = ffData(File, Time(time, 0), 1, Interval(tlen), tlen != 0);
      if (getDebug()) cerr << "Found time: " << time 
			   << " nSec: " << tlen << endl;
      mDirty = true;
   }

   void
   FrameDir::remove(const char* dir) {
      for (file_iterator iter=begin() ; iter != end() ; ) {
         const ffData& d = iter->second;
         file_iterator next = iter;
         next++;
         if (!fnmatch(dir, d.getFile(), 0)) {
            erase(iter->first);
         }
         iter = next;
      }
   }

   void
   FrameDir::clear(void) {
      mList.clear();
      mDirty = false;
   }

   void
   FrameDir::erase(gps_t time) {
      mList.erase(time);
   }

   void
   FrameDir::checkData(CheckLevel lvl) {
      mDirty = false;
      if (getDebug()) cerr << "check Data" << endl;

    //----------------------------------  Make sure there's data to process.
      if (begin() == end()) 
         return;
   
    //----------------------------------  Swich on mode.
      ffData::count_t lastNFrame(1);
      Interval        lastDt(1.0);
      switch(lvl) {
         case none:
            break;
      
      //----------------------------------  Try to fill time gaps;
         case gapsOnly:
            for (file_iterator iter=begin() ; iter != end() ; ) {
            
            //--------------------------  Access current and next entries
               const ffData& d = iter->second;
               file_iterator next = iter;
               next++;
               if (next == end()) 
                  break;
            //--------------------------  Fill in info if there's a gap
               if (!Almost(d.getEndTime(), next->second.getStartTime())) {
               
               //----------------------  Is gap same as last?
                  Interval Delta=next->second.getStartTime() - d.getStartTime();
                  if (Delta == lastDt*double(lastNFrame)) {
                     if (getDebug()) {
                        cerr << "Inferring file: " << d.getFile()
                           << " parameters from previous file." << endl;
                     }
                     mList[iter->first]=ffData(d.getFile(), d.getStartTime(),
                                          lastNFrame, lastDt, false);
                  
                  //----------------------  Read nFrames, length from file.
                  } 
                  else {
                     if (getDebug()) {
                        cerr << "Getting parameters from file: " 
                           << d.getFile() << "." << endl;
                     }
                     try {
                        ffData newFdata = getFileData(d.getFile());
                        mList[iter->first] = newFdata;
                        lastNFrame = newFdata.getNFrames();
                        lastDt     = newFdata.getDt();
                        if (getDebug()) {
                           cerr << "lastNFrame = " << lastNFrame << " lastDt = " <<
                              lastDt << endl;
                        }
                     } 
                        catch (BadFile& b) {
                           if (getDebug()) {
                              cerr << "Exception: " << b.what() 
                                 << " caught while processing file " 
                                 << d.getFile() << endl;
                           }
                           mList.erase(iter->first);
                        }
                  }
               }
               iter = next;
            }
            break;
      
         case allData:
            for (file_iterator iter=begin() ; iter != end() ; ) {
               const ffData& d = iter->second;
               file_iterator next = iter;
               next++;
               if (!d.isValid()) {
                  try {
                     mList[iter->first] = getFileData(d.getFile());
                  } 
                     catch (BadFile& b) {
                        if (getDebug()) {
                           cerr << "Exception: " << b.what() 
                              << " caught while processing file " 
                              << d.getFile() << endl;
                        }
                        mList.erase(iter->first);
                     }
               }
               iter = next;
            }
            break;
      }
   }

   void
   FrameDir::checkData(CheckLevel lvl) const {
      const_cast<FrameDir*>(this)->checkData();
   }

   ffData
   FrameDir::getFileData(const char* File) throw (BadFile) {
      ffData::count_t NFrame(1);
      Time Start(0);
      Interval Dt(0.0);
   
    //----------------------------------  Read in the file header
      ifstream s(File, ios::in);
      FrameF f(s);
      if (!f.isOK()) throw BadFile("Unable to open File");
   
    //----------------------------------  Find the FrHeader structure type
      short IDFrHeader(0);
      while (!IDFrHeader && f.NxStruct()) {
         if (f.getID() == 1) {
            string StructName = f.getString();
            if (StructName == "FrameH") IDFrHeader = f.getShort();
         }
      }
      if (!IDFrHeader) throw BadFile("No FrameH definition");
   
    //----------------------------------  Look for the first header
      while (f.NxStruct() && f.getID() != IDFrHeader);
      if (f.getID() != IDFrHeader) throw BadFile("Can't find a frame header");
   
    //----------------------------------  Get the Start time and length
      f.getString();
      if (f.getVersion() >= 4) f.Skip(3*sizeof(int));
      else                     f.Skip(2*sizeof(int));
      Start.setS(f.getInt());
      Start.setN(f.getInt());
      f.Skip(sizeof(short)+  sizeof(int));
      Dt = Interval(f.getDouble());
   
    //----------------------------------  Get the number of frames from FrEOF
      struct FrEOF {
         int nFrames;
         int nBytes;
         int chkFlag;
         int chkSum;
      } eofstruc;
   
      int eofsize = sizeof(FrEOF) + sizeof(FrameF::StrHdr);
      if (f.getVersion() >= 4) eofsize += sizeof(int);      // TOC offset
      f.Seek(-eofsize, ios::end);
      if (f.NxStruct() && f.getLength() == eofsize) {
         eofstruc.nFrames = f.getInt();
         eofstruc.nBytes  = f.getInt();
         eofstruc.chkFlag = f.getInt();
         eofstruc.chkSum  = f.getInt();
         NFrame = eofstruc.nFrames;
         if (!NFrame) NFrame = 1;
      } 
      else {
         cerr << "End of File structure not found!" << endl;
      }
   
      s.close();
      return ffData(File, Start, NFrame, Dt, true);
   }

