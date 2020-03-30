//
// Created by jonathan.hanks on 3/26/20.
//

#ifndef DAQD_TRUNK_THREAD_LAUNCHER_HH
#define DAQD_TRUNK_THREAD_LAUNCHER_HH

#include <functional>

#include <pthread.h>

using thread_handler_t = std::function< void( void ) >;

/*!
 * @brief A more generic interface around pthread_create
 * @details pthread_create has a limited interface, pass a void *,
 * std::thread doesn't allow specifying stack size, scheduling, ...
 * so provide a wrapper on pthread create that takes a more generic
 * argument
 * @param tid The pthread_t thread id structure
 * @param attr Structure describing pthread attributes to set
 * @param handler the code to run on the new thread
 */
extern int launch_pthread( pthread_t&            tid,
                           const pthread_attr_t& attr,
                           thread_handler_t      handler );

#endif // DAQD_TRUNK_THREAD_LAUNCHER_HH
