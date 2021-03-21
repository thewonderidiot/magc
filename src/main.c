//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "agc.h"
#include "dsky.h"

//---------------------------------------------------------------------------//
//                                  Defines                                  //
//---------------------------------------------------------------------------//
#define AGC_CYCLES_PER_STEP  1536
#define AGC_WAKEUP_PERIOD_NS 18000000
#define DSKY_PORT 19681

//---------------------------------------------------------------------------//
//                         Local Function Prototypes                         //
//---------------------------------------------------------------------------//
static void agc_execute(uint16_t cycles);
static void dsky_service(void);
static int initialize_timer(timer_t *timer);
static int initialize_socket(int *sockfd);
static void void_handler(int sig);

//---------------------------------------------------------------------------//
//                                Local Data                                 //
//---------------------------------------------------------------------------//
static agc_state_t agc_state;
static int dsky_sock;
static int dsky_fd = -1;
static dsky_t dsky_state;

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
int main(int argc, char **argv) {
    memset(&agc_state, 0, sizeof(agc_state));

    if (argc != 2) {
        fprintf(stderr, "Usage: magc <rope>\n");
        return 1;
    }

    agc_init(&agc_state);
    int status = agc_load_rope(&agc_state, argv[1]);
    if (status != 0) {
        fprintf(stderr, "Failed to load rope %s: %d\n", argv[1], status);
        return status;
    }

    status = initialize_socket(&dsky_sock);
    if (status != 0) {
        fprintf(stderr, "Failed to initialize socket: %d\n", status);
        return status;
    }

    timer_t timer;
    status = initialize_timer(&timer);
    if (status != 0) {
        fprintf(stderr, "Failed to initialize timer: %d\n", status);
        return status;
    }

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGUSR1);
    signal(SIGINT, void_handler);
    signal(SIGUSR1, void_handler);
    signal(SIGPIPE, SIG_IGN);

    int sig;
    for (;;) {
        sigwait(&sigset, &sig);
        if (sig == SIGUSR1) {
            agc_execute(AGC_CYCLES_PER_STEP);
        } else if (sig == SIGINT) {
            break;
        }
    }

    timer_delete(timer);
    if (dsky_fd > 0) {
        close(dsky_fd);
    }
    close(dsky_sock);


    return 0;
}

//---------------------------------------------------------------------------//
//                        Local Function Definitions                         //
//---------------------------------------------------------------------------//
static void agc_execute(uint16_t cycles) {
    for (int i = 0; i < cycles; i++) {
        agc_service(&agc_state);
        if (i & 040) {
            dsky_service();
        }
    }
}

static void dsky_service(void) {
    struct sockaddr_in cli;
    socklen_t addrlen;
    int status;
    uint8_t key;

    if (dsky_fd < 0) {
        addrlen = sizeof(cli);
        dsky_fd = accept(dsky_sock, (struct sockaddr*)&cli, &addrlen);
        if (dsky_fd < 0) {
            return;
        }

        int flags = fcntl(dsky_fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(dsky_fd, F_SETFL, flags);
        return;
    }

    if (memcmp(&dsky_state, &agc_state.dsky, sizeof(dsky_state)) != 0) {
        memcpy(&dsky_state, &agc_state.dsky, sizeof(dsky_state));
        status = write(dsky_fd, &dsky_state, sizeof(dsky_state));
        if (status != sizeof(agc_state.dsky)) {
            close(dsky_fd);
            dsky_fd = -1;
            return;
        }
    }

    status = read(dsky_fd, &key, sizeof(key));
    if (status > 0) {
        dsky_set_chan15(&agc_state, key);
        if (key == 022) {
            agc_state.restart = 0;
            if (!agc_state.altest) {
                agc_state.dsky.restart = 0;
            }
        }       
    }       
}
static int initialize_timer(timer_t *timer) {
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGUSR1;
    sigev.sigev_notify_attributes = NULL;

    int status = timer_create(CLOCK_REALTIME, &sigev, timer);
    if (status != 0) {
        return errno;
    }

    struct itimerspec in, out;
    in.it_value.tv_sec = 0;
    in.it_value.tv_nsec = AGC_WAKEUP_PERIOD_NS;
    in.it_interval.tv_sec = 0;
    in.it_interval.tv_nsec = AGC_WAKEUP_PERIOD_NS;
    status = timer_settime(*timer, 0, &in, &out);
    if (status != 0) {
        timer_delete(*timer);
        return errno;
    }

    return 0;
}

static int initialize_socket(int *sockfd) {
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1) {
        return errno;
    }

    int sock_on = 1;
    setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&sock_on, sizeof(sock_on));

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(DSKY_PORT); 

    int status = bind(*sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (status != 0) {
        close(*sockfd);
        return errno;
    }

    status = listen(*sockfd, 1);
    if (status != 0) {
        return errno;
    }

    int flags = fcntl(*sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(*sockfd, F_SETFL, flags);

    return 0;
}

static void void_handler(int sig) {
    (void)sig;
}

