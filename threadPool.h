// Noa Gol
// 208469486


#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <sys/types.h>
#include "osqueue.h"
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

typedef struct thread_pool {
    pthread_t *threads;
    int numOfThreads;
    OSQueue *taskQueue;
    int canInsertToQueue;
    int queueStatus;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ThreadPool;



typedef struct task {
    void (*computeFunc)(void *param);

    void *param;
} Task;

ThreadPool *tpCreate(int numOfThreads);

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param);

#endif
