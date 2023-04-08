#ifndef FUZZ_H
#define FUZZ_H
#include <pthread.h>
#define CHECK(x, message, ...) if(!(x)) {fprintf(stderr, "%s error %s at %s:%d\n---------ERROR: ", __TIMESTAMP__, strerror(errno), __FILE__, __LINE__); fprintf(stderr, (char*)message, ##__VA_ARGS__); exit(errno); }
#define INFO(message, ...) fprintf(stderr, "%s at %s:%d\n---------INFO: ", __TIMESTAMP__, __FILE__, __LINE__); fprintf(stderr, message, ##__VA_ARGS__)
void fuzz_notify(void);

pthread_t fuzz_setup(void);

#endif