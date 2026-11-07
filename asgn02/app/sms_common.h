#ifndef SMS_COMMON_H
#define SMS_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define ALARM_MQ_NAME "/sms_alarm_queue"
#define LOG_MQ_NAME   "/sms_log_queue"

typedef struct {
    int64_t timestamp_ms;
    float temperature;
    float humidity;
} SensorData;

typedef enum {
    ALERT_TEMP_HIGH,
    ALERT_HUMID_HIGH,
    ALERT_TEMP_SPIKE
} AlertType;

typedef struct {
    int64_t timestamp_ms;
    AlertType type;
    float value;
    float threshold;
} AlertMessage;

typedef struct {
    char text[256];
} LogMessage;

typedef struct {
    int sampling_interval_ms;
    float temp_threshold;
    float humid_threshold;
    int run_duration_sec;
    bool verbose;
} AppConfig;

extern volatile bool g_running;
extern AppConfig g_cfg;

#endif
