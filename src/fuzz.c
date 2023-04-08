

#ifdef ENABLE_AFL
#include "fuzz.h"
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE 65536
#define bool char
#define true 1
#define false 0

static pthread_cond_t cond;
static pthread_mutex_t mutex;
static bool ready;

static void *work_thread()
{
    // freopen ("logmasq", "w", stderr);
    INFO("fuzz thread startup successfully\n");
    char *host;
    char *port;
    struct sockaddr_in servaddr;
    int sockfd;
    void *buf;
    int inputfd;
    int loop = 0;

    host = getenv("FUZZ_SERVER_CONFIG");
    CHECK(host != NULL, "FUZZ_SERVER_CONFIG is not defined\n");
    host = strdup(host);
    CHECK(host != NULL, "host is null\n");
    port = strchr(host, ':');
    CHECK(port != NULL, "port is null\n");
    *port = 0;
    port++;
    INFO("port = %s, addr = %s\n", port, host);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    CHECK(inet_pton(AF_INET, host, &servaddr.sin_addr) == 1, "invalid port\n");
    servaddr.sin_port = htons(atoi(port));

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK(sockfd != -1, "socket creation failed\n");

    buf = malloc(BUF_SIZE);
    CHECK(buf != NULL, "memory allocation failed\n");
    char *inputfile = getenv("FUZZ_INPUT_FILE");
    if (inputfile)
    {
        errno = 0;
        inputfd = open(inputfile, O_RDONLY);
        CHECK(inputfd != -1, "fail to open input file\n");
    }
    else
    {
        inputfd = open("/dev/fd/0", O_RDONLY);
    }
    free(host);
#ifdef __AFL_LOOP
    while (__AFL_LOOP(1000))
    {
        INFO("__AFL_LOOP(%d)\n", loop);
        loop++;
#else
    {
        INFO("NO AFL_LOOP\n");
#endif /* ifdef __AFL_LOOP */
        size_t length;
        size_t sendsize;

        errno = 0;
        length = read(inputfd, buf, BUF_SIZE);
        CHECK(length != -1, "fail to read input file\n");
        if (length == 0)
        {
#ifdef __AFL_LOOP
            continue;
#else
            goto next;
#endif /* ifdef __AFL_LOOP */
        }
        INFO("read from input = %s\n", buf);
        CHECK(pthread_mutex_lock(&mutex) == 0, "fail to get mutex\n");

        ready = false;

        length = sendto(sockfd, buf, length, 0,
                        (struct sockaddr *)&servaddr, sizeof(servaddr));
        CHECK(length == length, "fail to send\n");
        memset(buf, 0, BUF_SIZE);
        (void)recvfrom(sockfd, buf, 65536, MSG_DONTWAIT,
                       (struct sockaddr *)&servaddr, sizeof(servaddr));
        INFO("before wait, recv = %s\n", buf);
        while (!ready)
        {
            pthread_cond_wait(&cond, &mutex);
        }
        INFO("after wait\n");
        CHECK(pthread_mutex_unlock(&mutex) == 0, "fail to unlock mutex\n");
    next:;
    }
    INFO("finish afl, exit\n");
    free(buf);
    close(sockfd);
    fclose(stderr);
    exit(0);
    return (NULL);
}

void fuzz_notify()
{
    INFO("fuzz notify\n");
    CHECK(pthread_mutex_lock(&mutex) == 0, "notify fail to get mutex\n");

    ready = true;

    CHECK(pthread_cond_signal(&cond) == 0, "notify fail to signal work thread\n");
    CHECK(pthread_mutex_unlock(&mutex) == 0, "notify fail to unlock mutex\n");
    INFO("fuzz notify end, ready = %d\n", ready);
}

pthread_t fuzz_setup()
{
    pthread_t thread;
    if (getenv("AFL_PERSISTENT"))
    {
        CHECK(pthread_mutex_init(&mutex, NULL) == 0, "fail to init mutex\n");
        CHECK(pthread_cond_init(&cond, NULL) == 0, "fail to init cond\n");
        CHECK(pthread_create(&thread, NULL, work_thread, NULL) == 0, "fail to create work thread\n");
    }
    return thread;
}

#endif