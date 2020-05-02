//
// Created by erik.vonreis on 5/1/20.
//

#ifndef _UTIL_MODELRATE_H
#define _UTIL_MODELRATE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * Get control model loop rates and dcuid from GDS param files
 * Needed to properly size TP data into the data stream
 *
 * If non-null, the function looks in the path gds_tp_dir to find the param files.
 * Otherwise, it will use the environment variable GDS_TP_DIR as the path.
 * If the gds_tp_dir is null and GDS_TP_DIR is not defined, then the function looks
 * in /opt/rtcds/$SITE/$IFO/target/gds/param
 *
 * rate and dcuid input arguments may be unchanged when there is an error.
 *
 * @param rate (out) on success, return the loop rate of the named model in Hz.
 * @param dcuid (out) on success, returns the dcuid for the model
 * @param modelname  (in) the full name of the model
 * @param gds_tp_dir  (in, can be null), if non null, a path to the directory
 * containing the GDS param files.
 * @return always returns 0.
 */
int
getmodelrate( int*        rate,
              int*        dcuid,
              const char* modelname,
              char*       gds_tp_dir );

#ifdef __cplusplus
};
#endif //__cplusplus


#endif // UTIL_MODELRATE_H
