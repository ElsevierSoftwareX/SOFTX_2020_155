#include <string.h>

#include <zmq.h>
#include "zmq_transport.h"

/*
 * This needs to be kept in sync with daq_core.h
 *
 * It is the daq_msg_header_t w/o the tpNum table
 */
typedef struct daq_msg_header_abrev_t
{
    unsigned int dcuId; // Unique DAQ unit id
    unsigned int fileCrc; // Configuration file checksum
    unsigned int status; // FE controller status
    unsigned int cycle; // DAQ cycle count (0-15)
    unsigned int timeSec; // GPS seconds
    unsigned int timeNSec; // GPS nanoseconds
    unsigned int dataCrc; // Data CRC checksum
    unsigned int dataBlockSize; // Size of data block for this message
    unsigned int tpBlockSize; // Size of the tp block for this message
    unsigned int tpCount; // Number of TP chans in this data set
} daq_msg_header_abrev_t;

/**
 * Send a daq_multi_dcu_data_t over a zmq socket in a 'compressed' form
 * @param src Pointer to a daq_multi_dcu_data_t structure
 * @param socket Pointer to a zmq socket
 * @return 0 on error
 *
 * @note The 'compressed' stream just skips unneeded TP table entries and DCU
 * entries.
 */
int
zmq_send_daq_multi_dcu_t( daq_multi_dcu_data_t* src,
                          void*                 socket,
                          int*                  xmit_size )
{
    int                     cur_model = 0;
    int                     total_models = 0;
    char*                   dest_data = 0;
    size_t                  header_size = 0;
    size_t                  transfer_size = 0;
    daq_msg_header_abrev_t* dest_header = 0;
    daq_multi_dcu_data_t    _dest_buffer;

    if ( !src || !socket )
        return 0;
    total_models = _dest_buffer.header.dcuTotalModels =
        src->header.dcuTotalModels;
    _dest_buffer.header.fullDataBlockSize = src->header.fullDataBlockSize;

    /* only send the number of dcus that are actually used */
    dest_data = (char*)&( _dest_buffer.header.dcuheader[ 0 ] );
    for ( cur_model = 0; cur_model < total_models; ++cur_model )
    {
        dest_header = (daq_msg_header_abrev_t*)dest_data;
        /* copy over the header, including only the portion of the TP table that
         * is used */
        header_size = sizeof( daq_msg_header_abrev_t ) +
            sizeof( unsigned int ) *
                ( src->header.dcuheader[ cur_model ].tpCount );
        memcpy( (void*)dest_header,
                (void*)( &src->header.dcuheader[ cur_model ] ),
                header_size );
        dest_data += header_size;
    }
    /* copy the raw data */
    memcpy( (void*)dest_data,
            (void*)( &src->dataBlock[ 0 ] ),
            src->header.fullDataBlockSize );
    transfer_size = (char*)dest_data - (char*)( &_dest_buffer ) +
        src->header.fullDataBlockSize;
    zmq_send( socket, (void*)&_dest_buffer, transfer_size, 0 );
    if ( xmit_size )
    {
        *xmit_size = (int)transfer_size;
    }
    return 1;
}

/**
 * Receive a daq_multi_dcu_data_t structure over a zmq socket in a 'compressed'
 * form
 * @param dest Pointer to a daq_multi_dcu_data_t structure to fill
 * @param socket Socket to receive data on
 * @return 0 on error
 *
 * @note The 'compressed' stream just skips unneeded TP table entries and DCU
 * entries.
 */
int
zmq_recv_daq_multi_dcu_t( daq_multi_dcu_data_t* dest, void* socket )
{
    zmq_msg_t               message;
    int                     size = 0;
    int                     cur_model = 0;
    int                     total_models = 0;
    char*                   cur_data = 0;
    size_t                  header_size = 0;
    size_t                  transfer_size = sizeof( unsigned int ) * 2;
    daq_msg_header_abrev_t* cur_header = 0;
    daq_multi_dcu_header_t* global_header = 0;

    if ( !dest || !socket )
        return 0;
    zmq_msg_init( &message );
    size = zmq_msg_recv( &message, socket, 0 );

    global_header = (daq_multi_dcu_header_t*)zmq_msg_data( &message );

    total_models = dest->header.dcuTotalModels = global_header->dcuTotalModels;
    dest->header.fullDataBlockSize = global_header->fullDataBlockSize;

    /* Copy over the headers for the DCUs actually used */
    cur_data = (char*)( &global_header->dcuheader[ 0 ] );
    for ( cur_model = 0; cur_model < total_models; ++cur_model )
    {
        cur_header = (daq_msg_header_abrev_t*)cur_data;
        /* copy header, but only copy over the bits of the TP table that
         * were sent
         */
        header_size = sizeof( daq_msg_header_abrev_t ) +
            sizeof( unsigned int ) * ( cur_header->tpCount );
        memcpy( (void*)( &dest->header.dcuheader[ cur_model ] ),
                (void*)cur_data,
                header_size );
        transfer_size += header_size;
        cur_data += header_size;
    }
    /* now copy the remaining data as the data block */
    memcpy( (void*)( &dest->dataBlock[ 0 ] ),
            (void*)cur_data,
            size - transfer_size );
    zmq_msg_close( &message );
    return 1;
}

/**
 * Receive a daq_multi_dcu_data_t structure over a zmq socket in a 'compressed'
 * form
 * @param dest Pointer to a daq_multi_dcu_data_t[buffer_count] structure to fill
 * one entry in
 * @param socket Socket to receive data on
 * @param int buffer_count the length of dest
 * @return The buffer that was used (cycle % buffer_count), -1 on error
 *
 * @note The 'compressed' stream just skips unneeded TP table entries and DCU
 * entries.  The output is written into the destination buffer at offset
 * (current input cycle % buffer_count)
 */
int
zmq_recv_daq_multi_dcu_t_into_buffer( daq_multi_dcu_data_t* dest_buffers,
                                      pthread_spinlock_t*   locks,
                                      void*                 socket,
                                      int                   buffer_count,
                                      unsigned int*         dest_gps_sec,
                                      int*                  dest_cyle )
{
    zmq_msg_t               message;
    int                     size = 0;
    int                     cur_model = 0;
    int                     total_models = 0;
    char*                   cur_data = 0;
    int                     dest_index = 0;
    daq_multi_dcu_data_t*   dest = 0;
    size_t                  header_size = 0;
    size_t                  transfer_size = sizeof( unsigned int ) * 2;
    daq_msg_header_abrev_t* cur_header = 0;
    daq_multi_dcu_header_t* global_header = 0;

    if ( !dest_buffers || !socket )
        return -1;
    zmq_msg_init( &message );
    size = zmq_msg_recv( &message, socket, 0 );

    global_header = (daq_multi_dcu_header_t*)zmq_msg_data( &message );
    dest_index = ( global_header->dcuheader->cycle % buffer_count );
    dest = dest_buffers + dest_index;

    pthread_spin_lock( &locks[ dest_index ] );

    total_models = dest->header.dcuTotalModels = global_header->dcuTotalModels;
    dest->header.fullDataBlockSize = global_header->fullDataBlockSize;

    /* Copy over the headers for the DCUs actually used */
    cur_data = (char*)( &global_header->dcuheader[ 0 ] );
    for ( cur_model = 0; cur_model < total_models; ++cur_model )
    {
        cur_header = (daq_msg_header_abrev_t*)cur_data;
        /* copy header, but only copy over the bits of the TP table that
         * were sent
         */
        header_size = sizeof( daq_msg_header_abrev_t ) +
            sizeof( unsigned int ) * ( cur_header->tpCount );
        memcpy( (void*)( &dest->header.dcuheader[ cur_model ] ),
                (void*)cur_data,
                header_size );
        transfer_size += header_size;
        cur_data += header_size;
    }
    /* now copy the remaining data as the data block */
    memcpy( (void*)( &dest->dataBlock[ 0 ] ),
            (void*)cur_data,
            size - transfer_size );
    *dest_gps_sec = dest->header.dcuheader[ 0 ].timeSec;
    *dest_cyle = global_header->dcuheader->cycle;
    pthread_spin_unlock( &locks[ dest_index ] );
    zmq_msg_close( &message );
    return dest_index;
}