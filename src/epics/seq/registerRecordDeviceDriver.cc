/* THIS IS A GENERATED FILE. DO NOT EDIT */
/* Generated from ../O.Common/lsc.dbd */

#include "registryCommon.h"

extern "C" {
epicsShareExtern rset *pvar_rset_aoRSET;
epicsShareExtern int (*pvar_func_aoRecordSizeOffset)(dbRecordType *pdbRecordType);
epicsShareExtern rset *pvar_rset_aiRSET;
epicsShareExtern int (*pvar_func_aiRecordSizeOffset)(dbRecordType *pdbRecordType);
epicsShareExtern rset *pvar_rset_boRSET;
epicsShareExtern int (*pvar_func_boRecordSizeOffset)(dbRecordType *pdbRecordType);
epicsShareExtern rset *pvar_rset_biRSET;
epicsShareExtern int (*pvar_func_biRecordSizeOffset)(dbRecordType *pdbRecordType);
epicsShareExtern rset *pvar_rset_mbbiRSET;
epicsShareExtern int (*pvar_func_mbbiRecordSizeOffset)(dbRecordType *pdbRecordType);
epicsShareExtern rset *pvar_rset_stringinRSET;
epicsShareExtern int (*pvar_func_stringinRecordSizeOffset)(dbRecordType *pdbRecordType);
epicsShareExtern rset *pvar_rset_calcRSET;
epicsShareExtern int (*pvar_func_calcRecordSizeOffset)(dbRecordType *pdbRecordType);

static const char * const recordTypeNames[7] = {
    "ao",
    "ai",
    "bo",
    "bi",
    "mbbi",
    "stringin",
    "calc"
};

static const recordTypeLocation rtl[7] = {
    {pvar_rset_aoRSET, pvar_func_aoRecordSizeOffset},
    {pvar_rset_aiRSET, pvar_func_aiRecordSizeOffset},
    {pvar_rset_boRSET, pvar_func_boRecordSizeOffset},
    {pvar_rset_biRSET, pvar_func_biRecordSizeOffset},
    {pvar_rset_mbbiRSET, pvar_func_mbbiRecordSizeOffset},
    {pvar_rset_stringinRSET, pvar_func_stringinRecordSizeOffset},
    {pvar_rset_calcRSET, pvar_func_calcRecordSizeOffset}
};

epicsShareExtern dset *pvar_dset_devAoSoft;
epicsShareExtern dset *pvar_dset_devAiSoft;
epicsShareExtern dset *pvar_dset_devBoSoft;
epicsShareExtern dset *pvar_dset_devBiSoft;
epicsShareExtern dset *pvar_dset_devMbbiSoft;
epicsShareExtern dset *pvar_dset_devSiSoft;
epicsShareExtern dset *pvar_dset_devSiSeq;

static const char * const deviceSupportNames[7] = {
    "devAoSoft",
    "devAiSoft",
    "devBoSoft",
    "devBiSoft",
    "devMbbiSoft",
    "devSiSoft",
    "devSiSeq"
};

static const dset * const devsl[7] = {
    pvar_dset_devAoSoft,
    pvar_dset_devAiSoft,
    pvar_dset_devBoSoft,
    pvar_dset_devBiSoft,
    pvar_dset_devMbbiSoft,
    pvar_dset_devSiSoft,
    pvar_dset_devSiSeq
};

epicsShareExtern void (*pvar_func_susRegistrar)(void);
// epicsShareExtern void (*pvar_func_daqConfigRegistrar)(void);

int sus_registerRecordDeviceDriver(DBBASE *pbase)
{
    registerRecordTypes(pbase, 7, recordTypeNames, rtl);
    registerDevices(pbase, 7, deviceSupportNames, devsl);
    (*pvar_func_susRegistrar)();
//    (*pvar_func_daqConfigRegistrar)();
    return 0;
}

/* registerRecordDeviceDriver */
static const iocshArg registerRecordDeviceDriverArg0 =
                                            {"pdbbase",iocshArgPdbbase};
static const iocshArg *registerRecordDeviceDriverArgs[1] =
                                            {&registerRecordDeviceDriverArg0};
static const iocshFuncDef registerRecordDeviceDriverFuncDef =
                {"sus_registerRecordDeviceDriver",1,registerRecordDeviceDriverArgs};
static void registerRecordDeviceDriverCallFunc(const iocshArgBuf *)
{
    sus_registerRecordDeviceDriver(pdbbase);
}

} // extern "C"
/*
 * Register commands on application startup
 */
#include "iocshRegisterCommon.h"
class IoccrfReg {
  public:
    IoccrfReg() {
        iocshRegisterCommon();
        iocshRegister(&registerRecordDeviceDriverFuncDef,registerRecordDeviceDriverCallFunc);
    }
};
#if !defined(__GNUC__) || !(__GNUC__<2 || (__GNUC__==2 && __GNUC_MINOR__<=95))
namespace { IoccrfReg iocshReg; }
#else
IoccrfReg iocshReg;
#endif
