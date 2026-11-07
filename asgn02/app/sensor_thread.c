#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include "sms_common.h"
#include "circular_buffer.h"
#include "../driver/sms_sensor.h"

extern CircularBuffer *g_shared_buf;

void* sensor_thread_fn(void *arg)
{
    (void)arg;
    int fd = open("/dev/sms_sensor", O_RDONLY);
    if (fd < 0) {
        perror("[Sensor Thread] Cannot open /dev/sms_sensor");
        return NULL;
    }

    unsigned int interval_val = g_cfg.sampling_interval_ms;
    ioctl(fd, SMS_SET_INTERVAL, &interval_val);

    struct timespec next_period;
    clock_gettime(CLOCK_MONOTONIC, &next_period);

    char line[128];
    while (g_running) {
        next_period.tv_nsec += g_cfg.sampling_interval_ms * 1000000L;
        while (next_period.tv_nsec >= 1000000000L) {
            next_period.tv_sec += 1;
            next_period.tv_nsec -= 1000000000L;
        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_period, NULL);
        if (!g_running) break;

        lseek(fd, 0, SEEK_SET);
        ssize_t n = read(fd, line, sizeof(line) - 1);
        if (n > 0) {
            line[n] = '\0';
            long long ts;
            float t, h;
            if (sscanf(line, "%lld,%f,%f", &ts, &t, &h) == 3) {
                SensorData d;
                d.timestamp_ms = (int64_t)ts;
                d.temperature = t;
                d.humidity = h;

                if (g_cfg.verbose) {
                    printf("[SENSOR] Time: %lld, Temp: %.1f C, Humid: %.1f %%\n",
                           (long long)d.timestamp_ms, d.temperature, d.humidity);
                }
                if (!cb_push(g_shared_buf, d)) {
                    fprintf(stderr, "[WARN] Buffer full! Dropped sample.\n");
                }
            }
        }
    }
    close(fd);
    return NULL;
}
