#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mqueue.h>
#include "sms_common.h"

void* alert_thread_fn(void *arg)
{
    (void)arg;
    mqd_t alarm_mq = mq_open(ALARM_MQ_NAME, O_RDONLY);
    mqd_t log_mq   = mq_open(LOG_MQ_NAME, O_WRONLY);

    AlertMessage alert;
    unsigned int prio;

    while (g_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;

        ssize_t bytes = mq_timedreceive(alarm_mq, (char*)&alert, sizeof(alert), &prio, &ts);
        if (bytes > 0) {
            const char *type_str = "UNKNOWN";
            if (alert.type == ALERT_TEMP_HIGH) type_str = "TEMP_HIGH";
            else if (alert.type == ALERT_HUMID_HIGH) type_str = "HUMID_HIGH";
            else if (alert.type == ALERT_TEMP_SPIKE) type_str = "TEMP_SPIKE";

            fprintf(stderr, "[ALERT][%lld] TYPE: %s VALUE: %.1f THRESHOLD: %.1f\n",
                    (long long)alert.timestamp_ms, type_str, alert.value, alert.threshold);

            LogMessage log_msg;
            snprintf(log_msg.text, sizeof(log_msg.text),
                     "[ALERT][%lld] TYPE: %s VALUE: %.1f THRESHOLD: %.1f",
                     (long long)alert.timestamp_ms, type_str, alert.value, alert.threshold);
            mq_send(log_mq, (char*)&log_msg, sizeof(log_msg), 0);
        }
    }

    if (alarm_mq != (mqd_t)-1) mq_close(alarm_mq);
    if (log_mq != (mqd_t)-1) mq_close(log_mq);
    return NULL;
}
