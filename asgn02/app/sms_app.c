#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <mqueue.h>
#include <sys/mman.h>
#include "sms_common.h"
#include "circular_buffer.h"
#include "sensor_thread.h"
#include "processor_thread.h"
#include "alert_thread.h"
#include "logger_thread.h"

volatile bool g_running = true;
CircularBuffer *g_shared_buf = NULL;
AppConfig g_cfg = {500, 40.0f, 80.0f, 60, false};

void sig_handler(int sig)
{
    if (sig == SIGINT) {
        printf("\n[SHUTDOWN] Received SIGINT. Gracefully shutting down...\n");
        g_running = false;
        if (g_shared_buf) {
            pthread_mutex_lock(&g_shared_buf->lock);
            pthread_cond_broadcast(&g_shared_buf->not_empty);
            pthread_cond_broadcast(&g_shared_buf->not_full);
            pthread_mutex_unlock(&g_shared_buf->lock);
        }
    }
}

int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "i:t:h:d:v")) != -1) {
        switch (opt) {
        case 'i': g_cfg.sampling_interval_ms = atoi(optarg); break;
        case 't': g_cfg.temp_threshold = atof(optarg); break;
        case 'h': g_cfg.humid_threshold = atof(optarg); break;
        case 'd': g_cfg.run_duration_sec = atoi(optarg); break;
        case 'v': g_cfg.verbose = true; break;
        default:
            fprintf(stderr, "Usage: %s [-i ms] [-t C] [-h %%] [-d sec] [-v]\n", argv[0]);
            return 1;
        }
    }

    signal(SIGINT, sig_handler);
    mlockall(MCL_CURRENT | MCL_FUTURE);

    /* Cấu hình POSIX Message Queues */
    struct mq_attr attr_alarm = {.mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = sizeof(AlertMessage), .mq_curmsgs = 0};
    struct mq_attr attr_log   = {.mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = sizeof(LogMessage),   .mq_curmsgs = 0};

    mq_unlink(ALARM_MQ_NAME);
    mq_unlink(LOG_MQ_NAME);

    mqd_t q_alarm = mq_open(ALARM_MQ_NAME, O_CREAT | O_RDWR, 0666, &attr_alarm);
    mqd_t q_log   = mq_open(LOG_MQ_NAME,   O_CREAT | O_RDWR, 0666, &attr_log);

    if (q_alarm == (mqd_t)-1 || q_log == (mqd_t)-1) {
        perror("mq_open error");
        return 1;
    }

    g_shared_buf = cb_create(64);

    pthread_t th_sensor, th_proc, th_alert, th_log;

    /* Cấu hình SCHED_FIFO cho Sensor Thread */
    pthread_attr_t attr_rt;
    pthread_attr_init(&attr_rt);
    struct sched_param param = {.sched_priority = 50};
    pthread_attr_setschedpolicy(&attr_rt, SCHED_FIFO);
    pthread_attr_setschedparam(&attr_rt, &param);
    pthread_attr_setinheritsched(&attr_rt, PTHREAD_EXPLICIT_SCHED);

    pthread_create(&th_sensor, &attr_rt, sensor_thread_fn, NULL);
    pthread_attr_destroy(&attr_rt);

    pthread_create(&th_proc, NULL, processor_thread_fn, NULL);
    pthread_create(&th_alert, NULL, alert_thread_fn, NULL);
    pthread_create(&th_log, NULL, logger_thread_fn, NULL);

    printf("[SMS] System started. Running for %d seconds (Press Ctrl+C to abort)...\n", g_cfg.run_duration_sec);

    int elapsed = 0;
    while (g_running && (g_cfg.run_duration_sec == 0 || elapsed < g_cfg.run_duration_sec)) {
        sleep(1);
        elapsed++;
    }

    g_running = false;
    pthread_mutex_lock(&g_shared_buf->lock);
    pthread_cond_broadcast(&g_shared_buf->not_empty);
    pthread_cond_broadcast(&g_shared_buf->not_full);
    pthread_mutex_unlock(&g_shared_buf->lock);

    /* Chờ tất cả luồng kết thúc */
    pthread_join(th_sensor, NULL);
    pthread_join(th_proc, NULL);
    pthread_join(th_alert, NULL);
    pthread_join(th_log, NULL);

    unsigned long prod, cons, over;
    cb_get_stats(g_shared_buf, &prod, &cons, &over);
    printf("\n=== SUMMARY ===\n");
    printf("Samples Produced : %lu\n", prod);
    printf("Samples Consumed : %lu\n", cons);
    printf("Buffer Overflows : %lu\n", over);

    cb_destroy(g_shared_buf);
    mq_close(q_alarm);
    mq_close(q_log);
    mq_unlink(ALARM_MQ_NAME);
    mq_unlink(LOG_MQ_NAME);

    printf("[SMS] Clean exit completed.\n");
    return 0;
}
