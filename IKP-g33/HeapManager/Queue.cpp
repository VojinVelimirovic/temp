#include "queue.h"
#include <stdlib.h>

// Initialize the queue
void initializeQueue(Queue* queue) {
    queue->front = queue->rear = NULL;
    InitializeCriticalSection(&queue->lock);       // zakljuca queue tokom pristupa
    InitializeConditionVariable(&queue->notEmpty); // signalizira threadu kada queue nije vise prazan
}

// Dodavanje novog zahteva (Request) na kraj reda.
void enqueue(Queue* queue, Request request) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        // Handle memory allocation failure
        return;
    }
    newNode->request = request;
    newNode->next = NULL;

    EnterCriticalSection(&queue->lock);

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    }
    else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }

    LeaveCriticalSection(&queue->lock);
    WakeConditionVariable(&queue->notEmpty);
}

// Skida prvi zahtev (Request) iz reda i upisuje ga u prosledjeni pokazivac.
// Vraca `true` ako je zahtev pokupljen, ili `false` ako je red prazan.
bool dequeue(Queue* queue, Request* request) {
    EnterCriticalSection(&queue->lock);

    // Wait until the queue is not empty
    while (queue->front == NULL) {
        SleepConditionVariableCS(&queue->notEmpty, &queue->lock, INFINITE);
    }

    if (queue->front == NULL) {
        LeaveCriticalSection(&queue->lock);
        return false; // Queue is empty
    }

    Node* temp = queue->front;
    *request = temp->request;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp);
    LeaveCriticalSection(&queue->lock);
    return true;
}

// Oslobadja red i zauzete resurse, cleanup
void freeQueue(Queue* queue) {
    EnterCriticalSection(&queue->lock);
    Node* current = queue->front;
    Node* nextNode;

    // Traverse the queue and free each node
    while (current != NULL) {
        nextNode = current->next;
        free(current);
        current = nextNode;
    }

    // Reset the queue
    queue->front = NULL;
    queue->rear = NULL;

    LeaveCriticalSection(&queue->lock);
    DeleteCriticalSection(&queue->lock); // Clean up the critical section
}