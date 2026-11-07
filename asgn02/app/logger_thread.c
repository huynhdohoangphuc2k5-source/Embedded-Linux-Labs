#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <time.h>
#include "sms_common.h"

#define LOG_FILE "/tmp/sms.log"
#define LOG_FILE_OLD "/tmp/sms.log.1"
#define MAX_LOG_SIZE (1024 * 1024)

static void check_rotation(FILE **fp)
{
    struct stat st;
    if (stat(LOG_FILE, &st) == 0 && st.st_size >= MAX_LOG_SIZE) {
        fclose(*fp);
        rename(LOG_FILE, LOG_FILE_OLD);
        *fp = fopen(LOG_FILE, "a");
    }
}

void* logger_thread_fn(void *arg)
{
    (void)arg;
    mqd_t log_mq = mq_open(LOG_MQ_NAME, O_RDONLY);
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) {
        perror("Cannot open /tmp/sms.log");
        return NULL;
    }

    LogMessage msg;
    unsigned int prio;
    time_t last_flush = time(NULL);

    while (g_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;

        ssize_t bytes = mq_timedreceive(log_mq, (char*)&msg, sizeof(msg), &prio, &ts);
        if (bytes > 0) {
            fprintf(fp, "%s\n", msg.text);
            check_rotation(&fp);
        }

        if (time(NULL) - last_flush >= 5) {
            fflush(fp);
            last_flush = time(NULL);
        }
    }

    fflush(fp);
    fclose(fp);
    if (log_mq != (mqd_t)-1) mq_close(log_mq);
    return NULL;
}
