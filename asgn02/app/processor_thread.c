#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mqueue.h>
#include "sms_common.h"
#include "circular_buffer.h"

extern CircularBuffer *g_shared_buf;

void* processor_thread_fn(void *arg)
{
    (void)arg;
    mqd_t alarm_mq = mq_open(ALARM_MQ_NAME, O_WRONLY);
    mqd_t log_mq   = mq_open(LOG_MQ_NAME, O_WRONLY);

    float temp_window[10] = {0};
    int window_count = 0;
    int window_idx = 0;
    float prev_temp = -1.0f;

    SensorData d;
    while (cb_pop(g_shared_buf, &d)) {
        temp_window[window_idx] = d.temperature;
        window_idx = (window_idx + 1) % 10;
        if (window_count < 10) window_count++;

        float sum = 0;
        for (int i = 0; i < window_count; i++) sum += temp_window[i];
        float avg_temp = sum / window_count;

        if (d.temperature > g_cfg.temp_threshold) {
            AlertMessage alert = {d.timestamp_ms, ALERT_TEMP_HIGH, d.temperature, g_cfg.temp_threshold};
            mq_send(alarm_mq, (char*)&alert, sizeof(alert), 0);
        }
        if (d.humidity > g_cfg.humid_threshold) {
            AlertMessage alert = {d.timestamp_ms, ALERT_HUMID_HIGH, d.humidity, g_cfg.humid_threshold};
            mq_send(alarm_mq, (char*)&alert, sizeof(alert), 0);
        }
        if (prev_temp >= 0 && fabsf(d.temperature - prev_temp) > 5.0f) {
            AlertMessage alert = {d.timestamp_ms, ALERT_TEMP_SPIKE, d.temperature, 5.0f};
            mq_send(alarm_mq, (char*)&alert, sizeof(alert), 0);
        }
        prev_temp = d.temperature;

        LogMessage log_msg;
        snprintf(log_msg.text, sizeof(log_msg.text),
                 "[SAMPLE] Time=%lldms Temp=%.1fC Humid=%.1f%% AvgTemp=%.1fC",
                 (long long)d.timestamp_ms, d.temperature, d.humidity, avg_temp);
        mq_send(log_mq, (char*)&log_msg, sizeof(log_msg), 0);
    }

    if (alarm_mq != (mqd_t)-1) mq_close(alarm_mq);
    if (log_mq != (mqd_t)-1) mq_close(log_mq);
    return NULL;
}
