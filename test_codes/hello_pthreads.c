#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Function to be executed by the thread
void *printHello(void *threadId) {
    long tid = (long)threadId;
    fprintf(stderr,"Hello, World! from thread #%ld\n", tid);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    fprintf(stderr, "Hello Pthreads Beginning\n");
    int numThreads;
    if(argc < 2){
        numThreads = 1; // Default to one threads
    }else{
        numThreads = atoi(argv[1]);
    }
    pthread_t threads[numThreads];
    int rc;
    long t;

    for (t = 0; t < numThreads; t++) {
        fprintf(stderr,"Creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, printHello, (void *)t);
        if (rc) {
            fprintf(stderr,"Error: Unable to create thread %ld\n", t);
            exit(-1);
        }
    }

    // Wait for all threads to complete
    for (t = 0; t < numThreads; t++) {
        pthread_join(threads[t], NULL);
    }

    fprintf(stderr, "All threads have completed.\n");

    pthread_exit(NULL);
}

