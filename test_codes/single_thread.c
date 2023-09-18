#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 1
#define NUM_INCREMENTS 2

int sharedVariable = 0;
pthread_mutex_t lock;

void *incrementVariable() {
    for (int i = 0; i < NUM_INCREMENTS; i++) {
        sharedVariable++;
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;

    for (t = 0; t < NUM_THREADS; t++) {
        incrementVariable();
        fprintf(stderr, "%s", "Thread is created!\n");
    }

    printf("All threads have completed.\n");
    printf("Final sharedVariable value: %d\n", sharedVariable);
}

