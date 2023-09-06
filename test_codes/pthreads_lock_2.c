#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 2
#define NUM_INCREMENTS 2

int sharedVariable = 0;
pthread_mutex_t lock;

void *incrementVariable(void *threadId) {
    long tid = (long)threadId;
    for (int i = 0; i < NUM_INCREMENTS; i++) {
        pthread_mutex_lock(&lock);
        sharedVariable++;
        pthread_mutex_unlock(&lock);
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;

    // Initialize the mutex lock
    pthread_mutex_init(&lock, NULL);

    for (t = 0; t < NUM_THREADS; t++) {
        printf("[PT] Creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, incrementVariable, (void *)t);
        if (rc) {
            printf("[PT] Error: Unable to create thread %ld\n", t);
            exit(-1);
        }
    }

    // Wait for all threads to complete
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    printf("[PT] Final sharedVariable value: %d\n", sharedVariable);

    // Destroy the mutex lock
    pthread_mutex_destroy(&lock);

    pthread_exit(NULL);
}

