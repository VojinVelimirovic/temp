#pragma once
#pragma once
#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MemorySegment.h"


typedef struct HashMapEntry {
    int key;               // Integer key (address of the block)
    void* value;           // Pointer to the value (Block or int*)
    struct HashMapEntry* next; // For collision resolution (chaining)
} HashMapEntry;

typedef struct HashMap {
    int size;                  // Size of the hash table
    HashMapEntry** table;      // Array of pointers to HashMapEntry
} HashMap;

// Kreira novu hash mapu sa zadatom velicinom.
HashMap* createHashMap(int size);

// Dodavanje para (kljuc, vrednost)
void put(HashMap* map, int key, void* value);

// Preuzimanje na osnovu kljuca
void* get(HashMap* map, int key);

// Oslobadja memoriju zauzetu hash mapom
void deleteHashMap(HashMap* map);

// Preuzimanje na osnovu vrednosti
int findKeyByValue(HashMap* map, intptr_t value);

// Uklanja element sa zadatim kljucem iz hash mape
void remove(HashMap* map, int key);
#endif // HASHMAP_H
