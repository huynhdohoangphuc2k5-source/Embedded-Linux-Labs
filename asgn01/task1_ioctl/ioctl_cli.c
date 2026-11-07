#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "asgn1_ioctl.h"

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <dev_path> <cmd> [arg]\n", argv[0]);
        return 1;
    }
    int fd = open(argv[1], O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    if (strcmp(argv[2], "reset") == 0) {
        if (ioctl(fd, ASGN1_RESET_BUFFER) < 0) perror("ioctl reset");
        else printf("[OK] Buffer reset successfully\n");
    } else if (strcmp(argv[2], "stats") == 0) {
        struct asgn1_stats st;
        if (ioctl(fd, ASGN1_GET_STATS, &st) < 0) perror("ioctl stats");
        else printf("STATS: open=%d write=%d read=%d len=%d last_size=%d\n",
                    st.open_count, st.write_count, st.read_count, st.buffer_len, st.last_write_size);
    } else if (strcmp(argv[2], "mode") == 0) {
        if (argc < 4) { fprintf(stderr, "Mode arg missing\n"); close(fd); return 1; }
        int m = atoi(argv[3]);
        if (ioctl(fd, ASGN1_SET_MODE, &m) < 0) perror("ioctl mode");
        else printf("[OK] Mode changed to %d\n", m);
    } else if (strcmp(argv[2], "version") == 0) {
        char ver[64];
        if (ioctl(fd, ASGN1_GET_VERSION, ver) < 0) perror("ioctl version");
        else printf("VERSION: %s\n", ver);
    }
    close(fd);
    return 0;
}
