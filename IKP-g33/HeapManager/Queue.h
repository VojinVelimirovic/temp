#pragma once

#ifndef QUEUE_H
#define QUEUE_H

#include <windows.h>
#include <stdbool.h>

typedef struct {
    bool isAllocate;         // True for allocation, false for deallocation
    int value;               // Size to allocate or address to deallocate
    SOCKET clientSocket;
} Request;

typedef struct Node {
    Request request;
    struct Node* next;
} Node;

// Thread-safe queue structure
typedef struct {
    Node* front;
    Node* rear;
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE notEmpty;
} Queue;

// Inicijalizacija reda i kritickih sekcija 
void initializeQueue(Queue* queue);

// Dodavanje novog zahteva (Request) na kraj reda.
void enqueue(Queue* queue, Request request);

// Skida prvi zahtev (Request) iz reda i upisuje ga u prosledjeni pokazivac.
// Vraca `true` ako je zahtev pokupljen, ili `false` ako je red prazan.
bool dequeue(Queue* queue, Request* request);

// Oslobadja red i zauzete resurse, cleanup
void freeQueue(Queue* queue);

#endif // QUEUE_H
