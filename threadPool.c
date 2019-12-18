#include "threadPool.h"

#define ERROR "Error in system call"
#define FAIL -1
#define SUCCESS 0

/**
 * Prints error when system call happen.
 */
void sysCallError() {
    write(2, ERROR, 21);
}


/**
 * The function pop task from queue and execute it.
 * @param threadPool the treadpool struct.
 */
void executeTask(ThreadPool *threadPool) {

    while (threadPool->canInsertToQueue == 1
           || (threadPool->canInsertToQueue == 0
               && !osIsQueueEmpty(threadPool->taskQueue)
               && threadPool->queueStatus == 1)) {

        // Take the lock
        pthread_mutex_lock(&threadPool->mutex);

        // Wait while queue is empty and pool is running
        while (osIsQueueEmpty(threadPool->taskQueue) &&
               threadPool->canInsertToQueue == 1) {
            // Wait on condition
            pthread_cond_wait(&threadPool->cond, &threadPool->mutex);
        }

        // Check if can dequeue
        if (threadPool->queueStatus == 0) {
            pthread_mutex_unlock(&threadPool->mutex);
            return;
        }

        // pop task from the queue
        Task *task = (Task *) osDequeue(threadPool->taskQueue);

        pthread_mutex_unlock(&threadPool->mutex);
        // check that the task exists
        if (task != NULL) {
            // execute task
            task->computeFunc(task->param);
            free(task);
        }
    }
}

/**
 * Free treadpool allocated memory
 * @param threadPool the threadpool struct.
 */
void freeMemory(ThreadPool *threadPool) {
    pthread_mutex_destroy(&threadPool->mutex);
    pthread_cond_destroy(&threadPool->cond);
    osDestroyQueue(threadPool->taskQueue);
    free(threadPool->threads);
    free(threadPool);
}

/**
 * Free all allocated memory of the tasks in
 * the queue.
 * @param taskQueue the queue.
 */
void freeTaskQueue(OSQueue *taskQueue) {
    // goes all the tasks in the queue
    while (!osIsQueueEmpty(taskQueue)) {
        // pull task
        Task *task = osDequeue(taskQueue);
        // free memory
        free(task);
    }
}

/**
 * The function create a thread pool
 * @param numOfThreads number of threads
 * @return new pointer to the new threadpool struct if succeeded,
 * else return null
 */
ThreadPool *tpCreate(int numOfThreads) {

    // create new thread pool
    ThreadPool *threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));

    //check if allocation failed
    if (NULL == threadPool) {
        printf("Allocation failed");
        sysCallError();
        return NULL;
    }

    // set field of struct
    threadPool->numOfThreads = 0;
    threadPool->canInsertToQueue = 1;
    threadPool->queueStatus = 1;

    threadPool->taskQueue = osCreateQueue();
    //check if allocation failed
    if (threadPool->taskQueue == NULL) {
        sysCallError();
        // free memory
        free(threadPool);
        return NULL;
    }


    if (pthread_cond_init(&threadPool->cond, NULL) != 0) {
        sysCallError();
        // free memory
        osDestroyQueue(threadPool->taskQueue);
        free(threadPool);
        return NULL;
    }

    if (pthread_mutex_init(&(threadPool->mutex), NULL) != 0) {
        sysCallError();
        // free memory
        osDestroyQueue(threadPool->taskQueue);
        pthread_cond_destroy(&threadPool->cond);
        free(threadPool);
        return NULL;
    }

    // allocate memory for threads
    threadPool->threads = (pthread_t *) malloc(numOfThreads * sizeof(pthread_t));

    if (threadPool->threads == NULL) {
        sysCallError();
        // free memory
        pthread_cond_destroy(&threadPool->cond);
        pthread_mutex_destroy(&threadPool->mutex);
        free(threadPool);
        return NULL;
    }

    int i;
    // create all threads
    for (i = 0; i < numOfThreads; i++) {
        threadPool->numOfThreads++;
        // create thread
        if (pthread_create(&threadPool->threads[i], NULL, (void *) executeTask, threadPool) != 0) {
            // if fail cancel all threads
            for (i = 0; i < threadPool->numOfThreads - 1; i++) {
                pthread_cancel(threadPool->threads[i]);
            }
            sysCallError();
            // free memory
            freeMemory(threadPool);
            return NULL;
        }
    }

    return threadPool;
}

/**
 * Destroy thread pool.
 * @param threadPool the thread pool
 * @param shouldWaitForTasks should clear queue before ending
 */
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {

    if (threadPool == NULL || threadPool->canInsertToQueue == 0) {
        return;
    }

    // Initialize status
    threadPool->canInsertToQueue = 0;
    if (shouldWaitForTasks == 0) {
        threadPool->queueStatus = 0;
    }

    // Broadcast the condition
    pthread_mutex_lock(&(threadPool->mutex));
    if (pthread_cond_broadcast(&(threadPool->cond)) != 0) {
        freeTaskQueue(threadPool->taskQueue);
        freeMemory(threadPool);
        return;
    }
    pthread_mutex_unlock(&(threadPool->mutex));

    // Join all running threads
    int i;
    for (i = 0; i < threadPool->numOfThreads; i++) {
        pthread_join(threadPool->threads[i], NULL);
    }

    // Free allocated memory
    freeTaskQueue(threadPool->taskQueue);
    freeMemory(threadPool);
}


/**
 * Insert a task to the queue
 * @param threadPool the thread pool
 * @param computeFunc the function to execute
 * @param param parameters for the function
 * @return 0 if added successfully -1 if error
 */
int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {

    if (threadPool == NULL || computeFunc == NULL) {
        return FAIL;
    }

    if (threadPool->canInsertToQueue == 0) {
        return FAIL;
    }

    // Allocate the task
    Task *task = (Task *) malloc(sizeof(Task));
    if (task == NULL) {
        sysCallError();
        return FAIL;
    }

    // Set task values
    task->computeFunc = computeFunc;
    task->param = param;

    // Add to queue
    pthread_mutex_lock(&threadPool->mutex);
    osEnqueue(threadPool->taskQueue, (void *) task);
    // Signal condition
    if (pthread_cond_signal(&threadPool->cond) != 0) {
        return FAIL;
    }
    pthread_mutex_unlock(&threadPool->mutex);


    return SUCCESS;
}
