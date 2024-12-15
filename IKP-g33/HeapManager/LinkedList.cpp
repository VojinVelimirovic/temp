#include <stdio.h>
#include <stdlib.h>
#include "LinkedList.h"

void initList(LinkedList* list) {
    list->head = NULL;
}

// Append a new node to the linked list
void append(LinkedList* list, int address, int free_segments) {
    ListNode* newNode = (ListNode*)malloc(sizeof(ListNode));
    if (!newNode) {
        perror("Failed to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    newNode->address = address;
    newNode->free_segments = free_segments;
    newNode->next = NULL;

    if (!list->head) {
        list->head = newNode;
    }
    else {
        ListNode* current = list->head;
        while (current->next) {
            current = current->next;
        }
        current->next = newNode;
    }
}

// Print the linked list
void printList(const LinkedList* list) {
    ListNode* current = list->head;
    while (current) {
        printf("address: %d, free_segments: %d -> ", current->address, current->free_segments);
        current = current->next;
    }
    printf("NULL\n");
}

// Free the memory allocated for the linked list
void freeList(LinkedList* list) {
    ListNode* current = list->head;
    ListNode* nextNode;
    while (current) {
        nextNode = current->next;
        free(current);
        current = nextNode;
    }
    free(list);
}

void addBlockToList(LinkedList* list, int start_address, int requiredSegments) {
    if (!list || !list->head) {
        printf("Error: The list is empty or uninitialized.\n");
        return;
    }

    ListNode* current = list->head;
    ListNode* prev = NULL;
    int end_address = start_address + requiredSegments;

    while (current) {
        // If the node is fully consumed by the allocated block
        if (current->address >= start_address && current->address < end_address) {
            int overlap_start = current->address;
            int overlap_end = overlap_start + current->free_segments;

            if (overlap_end > end_address) {
                // Partial overlap: Update node to start after the allocated block
                current->address = end_address;
                current->free_segments = overlap_end - end_address;
                break;
            }
            else {
                // Full overlap: Remove the current node
                ListNode* temp = current;
                if (prev) {
                    prev->next = current->next;
                }
                else {
                    list->head = current->next;
                }
                current = current->next;
                free(temp);
                continue;
            }
        }

        // If the allocation starts within the current node
        if (current->address < start_address && current->address + current->free_segments > start_address) {
            int overlap_end = current->address + current->free_segments;
            current->free_segments = start_address - current->address;

            if (overlap_end > end_address) {
                // Split the current node into two nodes
                ListNode* newNode = (ListNode*)malloc(sizeof(ListNode));
                if (!newNode) {
                    perror("Failed to allocate memory for new node");
                    exit(EXIT_FAILURE);
                }
                newNode->address = end_address;
                newNode->free_segments = overlap_end - end_address;
                newNode->next = current->next;
                current->next = newNode;
            }
            break;
        }

        prev = current;
        current = current->next;
    }
}

void formListFromSegments(LinkedList* list, TMemorySegment* segments, int totalSegments) {
    if (!list || !segments) {
        printf("Error: Invalid list or segments array.\n");
        return;
    }

    initList(list);

    int i = 0;
    while (i < totalSegments) {
        if (segments[i].isFree) {
            int startAddress = segments[i].address;
            int freeSegmentsCount = 0;

            // Count consecutive free segments
            while (i < totalSegments && segments[i].isFree) {
                freeSegmentsCount++;
                i++;
            }

            append(list, startAddress, freeSegmentsCount);
        }
        else {
            i++;
        }
    }
}