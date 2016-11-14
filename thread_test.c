#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void thread_id(int *);

int main(void)
{
    pthread_t *threads;
    int i, n = 16;

    threads = (pthread_t *) malloc(n * sizeof(pthread_t));
    if (threads == NULL) {
        printf("error: could not create thread pointers, aborting ...\n");
        return 1;
    }
    for (i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, (void * (*) (void *)) thread_id, (void *) &i);
        pthread_join(threads[i], NULL);
    }
    return 0;
}

void thread_id(int *n)
{
    printf("Thread %d\n", *n);
}
