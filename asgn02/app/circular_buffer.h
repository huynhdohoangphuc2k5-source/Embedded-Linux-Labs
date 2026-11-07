#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <pthread.h>
#include "sms_common.h"

typedef struct {
    SensorData *buffer;
    int capacity;
    int head;
    int tail;
    int count;
    unsigned long total_produced;
    unsigned long total_consumed;
    unsigned long overflow_count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} CircularBuffer;

CircularBuffer* cb_create(int capacity);
void cb_destroy(CircularBuffer *cb);
bool cb_push(CircularBuffer *cb, SensorData data);
bool cb_pop(CircularBuffer *cb, SensorData *data);
void cb_get_stats(CircularBuffer *cb, unsigned long *prod, unsigned long *cons, unsigned long *over);

#endif
