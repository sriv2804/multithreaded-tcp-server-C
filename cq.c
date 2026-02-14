#include "cq.h"

void cq_init(CircularQueue *q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

int cq_enqueue(CircularQueue *q, int value) {
    if (q->count == MAX_SIZE) return -1;
    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % MAX_SIZE;
    q->count++;
    return 0;
}

int cq_dequeue(CircularQueue *q, int *value) {
    if (q->count == 0) return -1;
    *value = q->data[q->front];
    q->front = (q->front + 1) % MAX_SIZE;
    q->count--;
    return 0;
}

int cq_is_empty(CircularQueue *q) {
    return q->count == 0;
}

int cq_is_full(CircularQueue *q) {
    return q->count == MAX_SIZE;
}
