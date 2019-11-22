#ifndef IRIGB_HH
#define IRIGB_HH

#include "DatEnv.hh"

//#define DEBUG_IRIGB

class ostream;

//
//    IRIG-B class.
//
class IrigB : public DatEnv
{
public:
    int MaxFrame; //  Number of frames to process

    IrigB( int argc, const char* argv[] ); // Constructor

    ~IrigB( ); // Destructor

    void ProcessData( void );

private:
};

#endif //  IRIGB_HH
