#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<pthread.h>
#include<unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<string.h>
#include "cq.h"

#define PORT 8088
#define BACKLOG 5
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
#define THREAD_NUM 4

int taskCnt;

CircularQueue taskQueue;

void executeTask(int client_fd)
{
    //client_fd --> here is client specific socket
    ssize_t valread;
    char buffer[1024] = {0};
    valread = read(client_fd, buffer, 1023);
    //this is to simulate network related waits
    usleep(500000);
    printf("%s\n", buffer);
    char *ack = (char *)malloc(sizeof(char)*100);
    strcpy(ack, "Hello from server");
    send(client_fd, ack, strlen(ack), 0);
    close(client_fd);
    free(ack);
}

void* startThread(void *arg)
{
    while(1)
    {
        pthread_mutex_lock(&mutexQueue);
        while(taskQueue.count == 0)
        {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }
        int client_fd;
        if(cq_dequeue(&taskQueue, &client_fd)==-1)
        {
            fprintf(stderr, "The task queue is empty");
            pthread_mutex_unlock(&mutexQueue);
            return NULL;
        }
        pthread_mutex_unlock(&mutexQueue);
        executeTask(client_fd);
    }
}

void submitTask(int client_fd, struct sockaddr_in client_addr)
{
    printf("Connection accepted from %s:%d\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
    pthread_mutex_lock(&mutexQueue);
    if(cq_enqueue(&taskQueue, client_fd)==-1)
    {
        fprintf(stderr, "Task queue full. Rejecting connection.\n");
        close(client_fd);
        pthread_mutex_unlock(&mutexQueue);
        return;
    }
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}

int main()
{
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);
    pthread_t threads[THREAD_NUM];
    cq_init(&taskQueue);
    int i,j;
    //start the worker threads
    for(i=0; i<THREAD_NUM; i++)
    {
        pthread_create(threads+i, NULL, startThread, NULL);
    }
    //from the main thread we will do the socket thing(create, bind, listen, accept, offload)
    //client_fd -> socket for per client
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // 2. Bind socket to a port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(PORT);       // Convert to network byte order
    if (bind(server_fd, (struct sockaddr *)&server_addr,sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Listen for incoming connections
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d...\n", PORT);
    while(1)
    {
        // 4. keep on accepting client connection
        //the accept call returns a new fd(socket only) specific to the client
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            perror("accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        submitTask(client_fd, client_addr);
    }
    close(server_fd);
    for(i=0; i<THREAD_NUM; i++)
    {
        pthread_join(threads[i], NULL);
    }
}