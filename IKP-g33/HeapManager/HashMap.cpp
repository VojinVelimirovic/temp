#include "hashmap.h"

// Simple hash function
static int hash(int key, int size) {
    return key % size;
}

// Kreira novu hash mapu sa zadatom velicinom.
HashMap* createHashMap(int size) {
    HashMap* map = (HashMap*)malloc(sizeof(HashMap));
    map->size = size;
    map->table = (HashMapEntry**)calloc(size, sizeof(HashMapEntry*));
    return map;
}

// Dodavanje para (kljuc, vrednost)
void put(HashMap* map, int key, void* value) {
    int index = hash(key, map->size);
    HashMapEntry* entry = map->table[index];
    HashMapEntry* newEntry = (HashMapEntry*)malloc(sizeof(HashMapEntry));
    newEntry->key = key;
    newEntry->value = value;
    newEntry->next = entry; // Collision handling by chaining
    map->table[index] = newEntry;
}

// Preuzimanje na osnovu kljuca
void* get(HashMap* map, int key) {
    int index = hash(key, map->size);
    HashMapEntry* entry = map->table[index];
    while (entry) {
        if (entry->key == key) {
            return entry->value;
        }
        entry = entry->next;
    }
    return (void*)-1; // Key not found
}

// Oslobadja memoriju zauzetu hash mapom
void deleteHashMap(HashMap* map) {
    for (int i = 0; i < map->size; i++) {
        HashMapEntry* entry = map->table[i];
        while (entry) {
            HashMapEntry* temp = entry;
            entry = entry->next;
            free(temp);
        }
    }
    free(map->table);
    free(map);
}

// Preuzimanje na osnovu vrednosti
int findKeyByValue(HashMap* map, intptr_t value) {
    for (int i = 0; i < map->size; i++) {
        HashMapEntry* entry = map->table[i];
        while (entry) {
            if ((intptr_t)entry->value == value) {
                return entry->key; // Return the key if the value matches
            }
            entry = entry->next;
        }
    }
    return -1; // Return -1 if the value is not found
}

// Uklanja element sa zadatim kljucem iz hash mape
void remove(HashMap* map, int key) {
    int index = hash(key, map->size);
    HashMapEntry* entry = map->table[index];
    HashMapEntry* prev = NULL;

    // Traverse the chain in the correct bucket
    while (entry) {
        if (entry->key == key) {
            // If entry is the first in the chain, just update the bucket
            if (prev == NULL) {
                map->table[index] = entry->next;
            }
            else {
                // Otherwise, bypass the current entry
                prev->next = entry->next;
            }

            // Free the entry memory
            free(entry);

            return;
        }
        prev = entry;
        entry = entry->next;
    }
}
