#pragma once
#include "MemorySegment.h"
typedef struct ListNode {
    int address;
    int free_segments;
    struct ListNode* next;
} ListNode;

typedef struct LinkedList {
    ListNode* head;
} LinkedList;


// inicijalizacija liste
void initList(LinkedList* list);

// dodavanje novog elementa
void append(LinkedList* list, int address, int free_segments);

// azuriranje stanja liste kada se ubaci novi blok
void addBlockToList(LinkedList* list, int start_address, int requiredSegments);

// pravljenje liste iznova, od nule
void formListFromSegments(LinkedList* list, TMemorySegment* segments, int totalSegments);

// oslobadjanje zauzetih resursa, cleanup
void freeList(LinkedList* list);

// prikaz trenutnog stanja liste
void printList(const LinkedList* list);