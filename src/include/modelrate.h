//
// Created by erik.vonreis on 5/1/20.
//

#ifndef _UTIL_MODELRATE_H
#define _UTIL_MODELRATE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*!
 * @brief get a string from the environment and return a copy coverted to
 * lowercase
 * @param env_name the name of the environment variable
 * @param dest destination buffer (non-null)
 * @param dest_size size of dest (must be > 1)
 * @details If the environment variable env_name is not set, sets dest to the
 * empty string, else copies the value into dest and converts it to lowercase.
 * @note This truncates the value if needed to fit in dest.  Dest is always a
 * null terminated string after this call.
 */
extern void
get_env_lower( const char* env_name, char* dest, size_t dest_size );

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
extern int get_model_rate_dcuid( int*        rate,
              int*        dcuid,
              const char* modelname,
              char*       gds_tp_dir );

#ifdef __cplusplus
};
#endif //__cplusplus


#endif // UTIL_MODELRATE_H
