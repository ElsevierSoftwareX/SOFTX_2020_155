/* Sequencer lock */
#include <pthread.h>
pthread_mutex_t seq_lock = PTHREAD_MUTEX_INITIALIZER;
