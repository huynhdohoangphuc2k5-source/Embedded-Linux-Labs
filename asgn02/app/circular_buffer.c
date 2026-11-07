#include <stdlib.h>
#include "circular_buffer.h"

CircularBuffer* cb_create(int capacity)
{
    CircularBuffer *cb = (CircularBuffer*)malloc(sizeof(CircularBuffer));
    if (!cb) return NULL;
    cb->buffer = (SensorData*)malloc(sizeof(SensorData) * capacity);
    if (!cb->buffer) { free(cb); return NULL; }

    cb->capacity = capacity;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    cb->total_produced = 0;
    cb->total_consumed = 0;
    cb->overflow_count = 0;

    pthread_mutex_init(&cb->lock, NULL);
    pthread_cond_init(&cb->not_empty, NULL);
    pthread_cond_init(&cb->not_full, NULL);
    return cb;
}

void cb_destroy(CircularBuffer *cb)
{
    if (!cb) return;
    pthread_mutex_destroy(&cb->lock);
    pthread_cond_destroy(&cb->not_empty);
    pthread_cond_destroy(&cb->not_full);
    free(cb->buffer);
    free(cb);
}

bool cb_push(CircularBuffer *cb, SensorData data)
{
    pthread_mutex_lock(&cb->lock);
    if (cb->count == cb->capacity) {
        cb->overflow_count++;
        pthread_mutex_unlock(&cb->lock);
        return false;
    }
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->capacity;
    cb->count++;
    cb->total_produced++;
    pthread_cond_signal(&cb->not_empty);
    pthread_mutex_unlock(&cb->lock);
    return true;
}

bool cb_pop(CircularBuffer *cb, SensorData *data)
{
    pthread_mutex_lock(&cb->lock);
    while (cb->count == 0 && g_running) {
        pthread_cond_wait(&cb->not_empty, &cb->lock);
    }
    if (cb->count == 0 && !g_running) {
        pthread_mutex_unlock(&cb->lock);
        return false;
    }
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->count--;
    cb->total_consumed++;
    pthread_cond_signal(&cb->not_full);
    pthread_mutex_unlock(&cb->lock);
    return true;
}

void cb_get_stats(CircularBuffer *cb, unsigned long *prod, unsigned long *cons, unsigned long *over)
{
    pthread_mutex_lock(&cb->lock);
    *prod = cb->total_produced;
    *cons = cb->total_consumed;
    *over = cb->overflow_count;
    pthread_mutex_unlock(&cb->lock);
}
