#define _POSIX_C_SOURCE 200809L
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#define ITERATIONS 100

typedef struct {
    struct timespec ts;
    char padding[1024];  // Increase packet size to ensure pipe write blocks
} packet_t;


int main() {
    int fd[2];
    packet_t packet;

    if (pipe(fd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    int cpid;
    int rc = fork();
    if (rc < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    int wstatus;
    cpid = rc;
    if (rc > 0) {
        close(fd[0]);
        // parent
        for (int i = 0;i < ITERATIONS; i++) {
            clock_gettime(CLOCK_MONOTONIC, &packet.ts);
            if (write(fd[1], &packet, sizeof(packet)) != sizeof(packet)) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        close(fd[1]); // signal eof to the child
        if (waitpid(cpid, &wstatus, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    } else {
        close(fd[1]); // child doesn't write
        long long sum = 0;
        for (int i = 0; i < ITERATIONS; i++) {
            if (read(fd[0], &packet, sizeof(packet)) != sizeof(packet)) {
                perror("read fail");
                exit(EXIT_FAILURE);
            }
            struct timespec end;
            clock_gettime(CLOCK_MONOTONIC, &end);

            sum += (end.tv_sec - packet.ts.tv_sec) * 1000000000LL + (end.tv_nsec - packet.ts.tv_nsec);
        }
        close(fd[0]);
        printf("avg context switch: %lld ns\n", sum / ITERATIONS);
    }
    return 0;
}