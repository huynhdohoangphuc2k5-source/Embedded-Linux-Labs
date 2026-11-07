CC ?= gcc
CFLAGS ?= -Wall -Wextra -pthread -O2
LDFLAGS ?= -pthread -lrt -lm

SRCS = sms_app.c circular_buffer.c sensor_thread.c processor_thread.c alert_thread.c logger_thread.c

all: sms_app sms_app_x86

sms_app: $(SRCS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sms_app_x86: $(SRCS)
	gcc -g -Wall -Wextra -pthread -o $@ $^ $(LDFLAGS)

clean:
	rm -f sms_app sms_app_x86
