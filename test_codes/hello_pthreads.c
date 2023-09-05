#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Function to be executed by the thread
void *printHello(void *threadId) {
    long tid = (long)threadId;
    printf("Hello, World! from thread #%ld\n", tid);
    pthread_exit(NULL);
}

int main() {
    int numThreads = 5; // Number of threads to create
    pthread_t threads[numThreads];
    int rc;
    long t;

    for (t = 0; t < numThreads; t++) {
        printf("Creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, printHello, (void *)t);
        if (rc) {
            printf("Error: Unable to create thread %ld\n", t);
            exit(-1);
        }
    }

    // Wait for all threads to complete
    for (t = 0; t < numThreads; t++) {
        pthread_join(threads[t], NULL);
    }

    printf("All threads have completed.\n");

    pthread_exit(NULL);
}

