/*
* Illustrate Linux IPC using eventfd
* Child process creates an eventfd and writes to it
* Parent reads using poll/select/read on the eventfd
*
* Author: Deepak Unnikrishnan
*/

#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>             /* Definition of uint64_t */
#include <poll.h>
#include <sys/select.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define USE_READ 0
#define USE_POLL 1
#define USE_SELECT 2


int
main(int argc, char *argv[])
{
    int efd, j;
    uint64_t u;
    ssize_t s;
    struct pollfd pfd;
    struct timeval tv;
    fd_set rfds;
    int rd_mode;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s (0:read, 1:poll, 2:select) <num>...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    efd = eventfd(0, 0);
    if (efd == -1)
        handle_error("eventfd");

    pfd.fd = efd;
    pfd.events = POLLIN;
    rd_mode = strtoull(argv[1], NULL, 0);
    printf("Read mode = %d\n",rd_mode);

    switch (fork()) {
    case 0:
        for (j = 2; j < argc; j++) {
            printf("Child writing %s to efd\n", argv[j]);
            u = strtoull(argv[j], NULL, 0);
                    /* strtoull() allows various bases */
            s = write(efd, &u, sizeof(uint64_t));
            if (s != sizeof(uint64_t))
                handle_error("write");
        }
        printf("Child completed write loop\n");

        exit(EXIT_SUCCESS);

    default:
        sleep(2);

        printf("Parent about to read\n");
        if(rd_mode == USE_READ) {
          printf("Using read\n");
          s = read(efd, &u, sizeof(uint64_t)); 
        }
        else if(rd_mode == USE_POLL) {
          printf("Using poll\n");
          s = poll(&pfd, 1, -1);
        }
        else if(rd_mode == USE_SELECT) {
          printf("Using select\n");
          tv.tv_sec = 500;
          tv.tv_usec = 0;
          FD_ZERO(&rfds);
          FD_SET(efd, &rfds);
          s = select(efd+1, &rfds, NULL, NULL, &tv);
        } else {
           printf("Invalid read mode operation\n");
           return 1;
        }
        printf("s = %zu\n",s);
           
        if (s != sizeof(uint64_t))
            handle_error("read");
        printf("Parent read %llu (0x%llx) from efd\n",
                (unsigned long long) u, (unsigned long long) u);
        exit(EXIT_SUCCESS);

   case -1:
       handle_error("fork");
   }
}
