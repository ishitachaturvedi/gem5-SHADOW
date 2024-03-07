#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

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
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    fprintf(stderr, "Pthreads Lock Beginning\n");
    int numThreads;
    if(argc < 2){
        numThreads = 1; // Default to one threads
    }else{
        numThreads = atoi(argv[1]);
    }
    pthread_t threads[numThreads];
    int rc;
    long t;

    // Initialize the mutex lock
    pthread_mutex_init(&lock, NULL);

    for (t = 0; t < numThreads; t++) {
        fprintf(stderr, "Creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, incrementVariable, (void *)t);
        fprintf(stderr, "%s", "Thread is created!\n");
        if (rc) {
            fprintf(stderr, "Error: Unable to create thread %ld\n", t);
            exit(-1);
        }
        fprintf(stderr, "Increment done %ld\n", t);
    }

    // Wait for all threads to complete
    for (t = 0; t < numThreads; t++) {
        fprintf(stderr, "joing_thread %ld\n", t);
        //pthread_join(threads[t], NULL);
	    fprintf(stderr, "Thread_joined %ld\n",t);
    }

    fprintf(stderr, "All threads have completed.\n");
    fprintf(stderr, "Final sharedVariable value: %d\n", sharedVariable);

    // Destroy the mutex lock
    pthread_mutex_destroy(&lock);

    fprintf(stderr, "MUTEX_LOCK_DESTROYED.\n");

    //pthread_exit(NULL);
    exit(0);
}

