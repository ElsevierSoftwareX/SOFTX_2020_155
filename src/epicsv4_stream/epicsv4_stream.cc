/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * Heavily based on exampleCPP work by Mary Kraimer
 */


#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>
#include <sstream>
#include <vector>

#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "gps.hh"

#include <pv/channelProviderLocal.h>

#include <pv/serverContext.h>

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;

volatile bool done = false;

struct MainLoopArgs
{
    PVRecord* record;
    size_t data_rate;
};

// Helper function to make sure PVRecord::endGroupPut is called
class EndGroupPut
{
private:
    PVRecord& rec_;
public:
    EndGroupPut(PVRecord& rec): rec_(rec) {}
    ~EndGroupPut()
    {
        rec_.endGroupPut();
    }
};

// Create a PVRecord with a GPS timestamp of the given name and add it to the specified database
PVRecordPtr create_transfer_array(PVDatabasePtr master, const std::string& recordName)
{
    StructureConstPtr top = getFieldCreate()->createFieldBuilder()->
            addNestedStructure("data")->
            add("gps", pvLong)->
            add("nano", pvLong)->
            addArray("blob", pvUByte    )->
            endNested()->
            createStructure();
    PVStructurePtr pvStructure = getPVDataCreate()->createPVStructure(top);
    PVRecordPtr dest = PVRecord::create(recordName, pvStructure);
    bool result = master->addRecord(dest);

    if(!result)
        throw std::runtime_error("Unable to add array record to the database");
    return dest;
}

// Main loop, this tests updating the given record at 16Hz.
void* update_loop(void *arg)
{
    const int HERTZ = 16;

    MainLoopArgs* args = (MainLoopArgs*)arg;

    std::vector<epicsUInt8> source(args->data_rate/HERTZ);

    GPS::gps_clock clock;

    PVRecord *rec = args->record;
    PVStructurePtr structPtr = rec->getPVStructure();
    PVLongPtr gpsPtr = structPtr->getSubField<PVLong>("data.gps");
    PVLongPtr gpsNanoPtr = structPtr->getSubField<PVLong>("data.nano");
    PVScalarArrayPtr arrayPtr = structPtr->getSubField<PVScalarArray>("data.blob");

    GPS::gps_time time_step = GPS::gps_time(0, 1000000000/HERTZ);
    GPS::gps_time transmit_time = clock.now();
    ++transmit_time.sec;
    transmit_time.nanosec = 0;


    for (int i = 0; !done; ++i)
    {
        GPS::gps_time now = clock.now();
        while (now < transmit_time)
        {
            usleep(1);
            now = clock.now();
        }

        std::fill(source.begin(), source.end(), i % 256);

        shared_vector<epicsUInt8> sh_vec(source.size());
        std::copy(source.begin(), source.end(), sh_vec.begin());

        {
            rec->beginGroupPut();
            EndGroupPut eput(*rec);
            arrayPtr->putFrom(freeze(sh_vec));
            gpsPtr->put(transmit_time.sec);
            gpsNanoPtr->put(transmit_time.nanosec);
        }

        transmit_time = transmit_time + time_step;

        //std::cout << transmit_time.sec << ":" << transmit_time.nanosec << std::endl;
    }
    return NULL;
}

int main(int argc,char *argv[])
{
    MainLoopArgs args;

    string record_name("transfer_array");
    args.data_rate = 100 * 1024 * 1024;

    if (argc > 1)
        record_name = argv[1];
    if (argc > 2)
    {
        istringstream stream(argv[2]);
        stream >> args.data_rate;
    }

    PVDatabasePtr master = PVDatabase::getMaster();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();
    PVRecordPtr transfer_array = create_transfer_array(master, record_name);
    args.record = transfer_array.get();

    ServerContext::shared_pointer ctx =
            startPVAServer("local",0,true,true);

    pthread_t th;

    if (pthread_create(&th, NULL, update_loop, (void*)&args) != 0)
    {
        throw std::runtime_error("Unable to start update thread");
    }

    master.reset();
    cout << "transfer test server\n";
    string str;
    while(true) {
        cout << "Type exit to stop: \n";
        getline(cin,str);
        if(str.compare("exit")==0) break;

    }
    done = true;
    sleep(2);
    ctx->destroy();
    return 0;
}
