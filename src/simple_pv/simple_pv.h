#ifndef __STREAMING_SIMPLE_PV_H__
#define __STREAMING_SIMPLE_PV_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SIMPLE_PV_INT 0
#define SIMPLE_PV_STRING 1
#define SIMPLE_PV_DOUBLE 2

/*!
 * @brief A Simple structure representing a read-only PV
 */
typedef struct SimplePV
{
    const char* name;
    int         pv_type; /// SIMPLE_PV_INT or SIMPLE_PV_STRING
    void*       data;

    // These values are only used for an int or double pv
    // they will be cased to the appropriate type
    double alarm_high;
    double alarm_low;
    double warn_high;
    double warn_low;
} SimplePV;

typedef void* simple_pv_handle;

/*!
 * @brief Create a pCas server to export the given list of PVS
 * @param prefix Prefix to add to the name of each PV
 * @param pvs A array of SimplePV structures describing the data to export into
 * EPICS
 * @param pv_count The number of entries in pvs
 *
 * @returns NULL on error, else a handle object representing the pCas server
 *
 * @note call simple_pv_server_destroy to destroy the returned server
 */
simple_pv_handle
simple_pv_server_create( const char* prefix, SimplePV* pvs, int pv_count );

/*!
 * @brief Given a handle to a pCas server, update the data being reflected into
 * epics. This causes the data field re-read and reflected into EPICS.
 * @param server The pCas server
 *
 * @note It is safe to call this if server == NULL
 */
void simple_pv_server_update( simple_pv_handle server );

/*!
 * @brief Destroy and close down a pCas server created by
 * simple_pv_server_create.
 * @param server
 *
 * @note It is safe to call this if server == NULL or *server == NULL
 */
void simple_pv_server_destroy( simple_pv_handle* server );

#ifdef __cplusplus
}
#endif

#endif /* __STREAMING_SIMPLE_PV_H__ */