#pragma once
#include "HashMap.h"
extern int free_memory_error;

//struktura za povratnu vrednost FirstFit-a
typedef struct FirstFitResult {
    int startIndex;       // Indeks prvog slobodnog segmenta. Ako nema mesta za blok nakon firstFita on ce biti -1
    int missingSegments;  // U koliko nema mesta za blok nakon firstFita ovo nam govori koliko segmenata treba dodati na kraju.
} FirstFitResult;

// inicijalizacija struktura podataka
void initializeMemory(int initialSize);

// dodavanje dodatnih segmenata kada alokacija u trenutnom nizu segmenata nije moguca
void addSegments(int additionalSegments);

// algoritam za nalazenje slobodnih segmenata
FirstFitResult firstFit(int size);

// funkcija za alociranje bloka memorije odredjene velicine
void* allocate_memory(int size);

// funckija za oslobadjanje bloka memorije na odredjenoj adresi
void free_memory(void* address);

// funckija koja deinicijalizuje sve strukture i promjenljive
void cleanup_segments();

// funckija koja graficki prikazuje trenutni izgled segments niza
void drawMemorySegments();

void drawMemorySegments2();
