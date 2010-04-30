#include <time.h>
#include "FrameF.hh"
using namespace std;

FrameF::FrameF(istream& In) 
  : mIn(In), mHeaderOK(false), mOffset(0)
{
    if (!isOK()) return;
}

FrameF::~FrameF(void) {
    // mIn.close();
}

void
FrameF::ReadHeader(void) throw(BadFile) {
    mIn.read(mData, sizeof(mData));
    if (strcmp(mData, "IGWD")) throw BadFile("File is not IGWD");
    int i(1);
    bool Bigend = !*(char*)&i;
    mSwap = (( Bigend && (mData[12] == 0x34)) || 
	     (!Bigend && (mData[12] == 0x12)));
    mHeaderOK = true;
}

bool
FrameF::NxStruct(void) throw(BadFile) {

    //----------------------------------  Read the header if not done yet.
    if (!mHeaderOK) ReadHeader();

    //----------------------------------  Skip remainder of current structure
    if (mOffset) {
        int left = mSHdr.length - mOffset;
	if (left) mIn.ignore(left);
    }

    //----------------------------------  Read the data, swap if necessary
    char* cptr = (char*) &mSHdr;
    mIn.read((char*)&mSHdr, sizeof(mSHdr));
    if (mIn.eof()) throw BadFile("Unexpected EOF");
    
    if (mSwap) {
        short* sptr = (short*) cptr;
	for (unsigned int i=0 ; i<sizeof(mSHdr)/2 ; i++) {
	    char t  = cptr[0];
	    cptr[0] = cptr[1];
	    cptr[1] = t;
	    cptr += 2;
	}
	short ts = sptr[0];
	sptr[0]  = sptr[1];
	sptr[1]  = ts;
    }
    mOffset = sizeof(mSHdr);
    return true;
}

int
FrameF::getInt(void) throw (BadFile) {
    int a;
    mIn.read((char*)&a, sizeof(a));
    if (mIn.eof()) throw BadFile("Unexpected EOF");
    mOffset += sizeof(a);
    if (mSwap) {
        int b(a);
	char* from = (char*) (&b + 1);
	char* to   = (char*) &a;
	for (unsigned int i=0 ; i<sizeof(a) ; i++) *to++ = *--from;
    }
    return a;
}

float
FrameF::getFloat(void) throw (BadFile) {
    float a;
    mIn.read((char*)&a, sizeof(a));
    if (mIn.eof()) throw BadFile("Unexpected EOF");
    mOffset += sizeof(a);
    if (mSwap) {
        float b(a);
	char* from = (char*) (&b + 1);
	char* to   = (char*) &a;
	for (unsigned int i=0 ; i<sizeof(a) ; i++) *to++ = *--from;
    }
    return a;
}

short
FrameF::getShort(void) throw (BadFile) {
    short a;
    mIn.read((char*)&a, sizeof(a));
    if (mIn.eof()) throw BadFile("Unexpected EOF");
    mOffset += sizeof(a);
    if (mSwap) {
        short b(a);
	char* from = (char*) (&b + 1);
	char* to   = (char*) &a;
	for (unsigned int i=0 ; i<sizeof(a) ; i++) *to++ = *--from;
    }
    return a;
}

string
FrameF::getString(void) throw (BadFile) {
    short len = getShort();
    char* str = new char[len];
    mIn.read(str, len);
    if (mIn.eof()) throw BadFile("Unexpected EOF");
    mOffset += len;
    string a(str);
    delete str;
    return a;
}

double 
FrameF::getDouble(void) throw (BadFile) {
    double a;
    mIn.read((char*)&a, sizeof(a));
    if (mIn.eof()) throw BadFile("Unexpected EOF");
    mOffset += sizeof(a);
    if (mSwap) {
        double b(a);
	char* from = (char*) (&b + 1);
	char* to   = (char*) &a;
	for (unsigned int i=0 ; i<sizeof(a) ; i++) *to++ = *--from;
    }
    return a;
}

void 
FrameF::Seek(int off, ios::seekdir mode) {
    mIn.seekg(off, mode);
    mOffset = 0;
}

void 
FrameF::Skip(int off) {
    mIn.ignore(off);
    mOffset += off;
}

