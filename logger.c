#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<pthread.h>
#include<string.h>

#define MAX_LOGGER_SIZE 128
typedef struct
{
    char *ptr[MAX_LOGGER_SIZE];
    int front;
    int rear;
    int count;
}LoggerQueue;

pthread_mutex_t logq_mutex;
pthread_cond_t  logq_cond;
#define LOG_FILE_PATH "/tmp/server.log"
#define mesg_size 500

// Open-once file handle and logger thread state
static FILE *g_log_fp = NULL;
static pthread_t g_log_thread;
static int g_log_shutdown = 0; // set to 1 to request logger thread exit


int log_enqueue(LoggerQueue *q, char *log_ptr)
{
    if (q->count == MAX_LOGGER_SIZE) 
    return -1;
    q->ptr[q->rear] = log_ptr;
    q->rear = (q->rear + 1) % MAX_LOGGER_SIZE;
    q->count++;
    return 0;
}

int log_dequeue(LoggerQueue *q, char **mesg_ptr)
{
    if (q->count == 0) 
    return -1;
    *mesg_ptr = q->ptr[q->front];
    q->front = (q->front + 1) % MAX_LOGGER_SIZE;
    q->count--;
    return 0;
}
void out_log_mesg(char *mesg_ptr)
{
    // Expect g_log_fp to be opened once in log_routine()
    if (!g_log_fp)
    {
        // Best-effort lazy open if not already open
        g_log_fp = fopen(LOG_FILE_PATH, "a");
        if (!g_log_fp)
        {
            perror("Failed to open log file");
            return;
        }
        setvbuf(g_log_fp, NULL, _IOLBF, 0);
    }
    fprintf(g_log_fp, "%s\n", mesg_ptr);
    fflush(g_log_fp);
}

void* log_routine(void* arg)
{
    LoggerQueue* logq = (LoggerQueue*)arg;
    char mesg_ptr[mesg_size];

    // Open the log file once and keep it open until shutdown
    g_log_fp = fopen(LOG_FILE_PATH, "a");
    if (!g_log_fp)
    {
        perror("Failed to open log file");
        return NULL;
    }
    setvbuf(g_log_fp, NULL, _IOLBF, 0);

    while(1)
    {
        // Acquire the mutex protecting the shared queue before checking state
        pthread_mutex_lock(&logq_mutex);
        while(logq->count == 0 && !g_log_shutdown)
        {
            pthread_cond_wait(&logq_cond,&logq_mutex);
        }
        if (g_log_shutdown && logq->count == 0)
        {
            pthread_mutex_unlock(&logq_mutex);
            break; // graceful exit when drained
        }

        // Dequeue under the lock to safely access shared queue indices and pointers
        char *temp_ptr = NULL;
        if(log_dequeue(logq, &temp_ptr)==-1)
        {
            // Spurious wakeup or race; continue loop
            pthread_mutex_unlock(&logq_mutex);
            continue;
        }

        // Copy message into a stack buffer while still holding the lock
        // (safe as producers don't modify enqueued buffers)
        strncpy(mesg_ptr, temp_ptr, mesg_size -1);
        mesg_ptr[mesg_size-1] = '\0';

        // Release the lock before I/O so producers are not blocked by disk ops
        pthread_mutex_unlock(&logq_mutex);

        out_log_mesg(mesg_ptr);
        memset(mesg_ptr, 0, mesg_size);
    }

    // Close the log file on shutdown
    if (g_log_fp)
    {
        fflush(g_log_fp);
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
    return NULL;
}

LoggerQueue* log_init()
{
    int i,j;
    LoggerQueue* logq = (LoggerQueue* )malloc(sizeof(LoggerQueue));
    for(i=0; i<MAX_LOGGER_SIZE; i++)
    {
        logq->ptr[i] = NULL;
    }
    logq->front = logq->rear = logq->count = 0;
    // initialize synchronization primitives
    pthread_mutex_init(&logq_mutex, NULL);
    pthread_cond_init(&logq_cond, NULL);
    g_log_shutdown = 0;

    // start the log thread (joinable)
    pthread_create(&g_log_thread, NULL, log_routine, logq);
    return logq;
}

//this is the client facing function that adds log messages to queue
int print_log(LoggerQueue* logq, char *log_mesg)
{
    pthread_mutex_lock(&logq_mutex);
    if(logq->count == MAX_LOGGER_SIZE)
    {
        fprintf(stderr, "LOG queue full");
        pthread_mutex_unlock(&logq_mutex);
        return 1;
    }
    char *log_ptr = logq->ptr[logq->rear];
    if(log_ptr)
    {
      memset(log_ptr, 0, mesg_size);
    }
    else
    log_ptr = (char*)malloc(mesg_size*sizeof(char));
    strncpy(log_ptr, log_mesg, mesg_size-1);
    log_ptr[mesg_size -1] = '\0';
    if(log_enqueue(logq, log_ptr)==-1)
    {
        fprintf(stderr, "Hit an error in log enqueue");
        pthread_mutex_unlock(&logq_mutex);
        return -1;
    }
    pthread_mutex_unlock(&logq_mutex);
    pthread_cond_signal(&logq_cond);
    return 0;
}

// Graceful shutdown: drain, stop thread, free resources
int logger_shutdown(LoggerQueue* logq)
{
    if (!logq) return 0;

    // Signal shutdown and wake the logger thread
    pthread_mutex_lock(&logq_mutex);
    g_log_shutdown = 1;
    pthread_cond_broadcast(&logq_cond);
    pthread_mutex_unlock(&logq_mutex);

    // Wait for thread to finish
    pthread_join(g_log_thread, NULL);

    // Free pooled buffers
    for (int i = 0; i < MAX_LOGGER_SIZE; ++i)
    {
        free(logq->ptr[i]);
        logq->ptr[i] = NULL;
    }

    // Destroy sync primitives
    pthread_mutex_destroy(&logq_mutex);
    pthread_cond_destroy(&logq_cond);

    free(logq);
    return 0;
}

int main()
{
    LoggerQueue *logq = log_init();
    return 0;
}