#ifndef CQ_H
#define CQ_H

#define MAX_SIZE 256

typedef struct
{
    int data[MAX_SIZE];
    int front, rear, count;
}CircularQueue;

void cq_init(CircularQueue *q);
int cq_enqueue(CircularQueue *q, int value);
int cq_dequeue(CircularQueue *q, int *value);
int cq_is_empty(CircularQueue *q);
int cq_is_full(CircularQueue *q);
#endif