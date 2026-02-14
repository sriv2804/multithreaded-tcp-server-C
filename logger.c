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
    FILE *fp = fopen(LOG_FILE_PATH, "a");  // Open in append mode
    if (fp == NULL)
    {
        perror("Failed to open log file");
        return;
    }
    fprintf(fp, "%s\n", mesg_ptr);  // Write message with newline
    fclose(fp);  // Close the file
}

void* log_routine(void* arg)
{
    LoggerQueue* logq = (LoggerQueue*)arg;
    char mesg_ptr[mesg_size];
    while(1)
    {
        pthread_mutex_lock(&logq_mutex);
        while(logq->count == 0)
        {
            pthread_cond_wait(&logq_cond,&logq_mutex);
        }
        char *temp_ptr;
        if(log_dequeue(logq, &temp_ptr)==-1)
        {
            fprintf(stderr, "Hit an error in log dequeue");
            pthread_mutex_unlock(&logq_mutex);
            return NULL;
        }
        strncpy(mesg_ptr, temp_ptr, mesg_size -1);
        mesg_ptr[mesg_size-1] = '\0';
        pthread_mutex_unlock(&logq_mutex);
        out_log_mesg(mesg_ptr);
        memset(mesg_ptr, 0, mesg_size);
    }
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
    //start the log thread in detached mode
    pthread_attr_t detachedThread;
    pthread_attr_init(&detachedThread);
    pthread_attr_setdetachstate(&detachedThread, PTHREAD_CREATE_DETACHED);
    pthread_t logThread;
    pthread_create(&logThread, &detachedThread, log_routine, logq);
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

int main()
{
    LoggerQueue *logq = log_init();
    return 0;
}