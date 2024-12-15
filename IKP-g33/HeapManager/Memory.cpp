#include <stdlib.h>
#include <stdbool.h>
#include "HashMap.h"
#include "Memory.h"
#include "LinkedList.h"

TMemorySegment* segments = NULL;
int totalSegments = 0;          // trenutan broj segmenata. ovaj broj se menja i nalazi se u ovom fajlu zato sto nam ne treba van njega
HashMap* blockHashMap = NULL;
HashMap* blockAddressHashMap = NULL;
//LinkedList* list_of_free_segments = NULL;
int block_id = 1;

int free_memory_error = 0;      // error code u free_memory, koristi ga server da preciznije javi gresku

// inicijalizacija struktura podataka
void initializeMemory(int initialSize) {
    segments = (TMemorySegment*)malloc(initialSize * sizeof(TMemorySegment));
    for (int i = 0; i < initialSize; i++) {
        segments[i].block_id = -1;
        segments[i].address = i;
        segments[i].isFree = true; //na pocetku su segmenti slobodni
        segments[i].mutex = CreateMutex(NULL, FALSE, NULL);
    }
    totalSegments = initialSize;

    /*list_of_free_segments = (LinkedList*)malloc(sizeof(LinkedList));
    initList(list_of_free_segments);
    append(list_of_free_segments, 0, initialSize);*/
    blockHashMap = createHashMap(128); //u hashmapi ima ovoliko, ne mora da bude dinamicna mozemo menjati velicinu
    blockAddressHashMap = createHashMap(128);
}

// dodavanje dodatnih segmenata kada alokacija u trenutnom nizu segmenata nije moguca
void addSegments(int additionalSegments) {
    //ponovo allocuje segmente tako da ih ima vise bez unistavanja postojecih segmenata
    printf("%d",additionalSegments);
    segments = (TMemorySegment*)realloc(segments, (totalSegments + additionalSegments) * sizeof(TMemorySegment));
    for (int i = totalSegments; i < totalSegments + additionalSegments; i++) {
        segments[i].block_id = -1;
        segments[i].address = i;
        segments[i].isFree = true; //novo dodate segmente postavljam da su slobodni
        segments[i].mutex = CreateMutex(NULL, FALSE, NULL);
    }
    //append(list_of_free_segments, totalSegments, additionalSegments);
    totalSegments += additionalSegments; //menjamo ukupan broj segmenata
}

// algoritam za nalazenje slobodnih segmenata
FirstFitResult firstFit(int size) {
    FirstFitResult result;
    result.startIndex = -1;
    result.missingSegments = 0;
    int requiredSegments = (size + SEGMENT_SIZE - 1) / SEGMENT_SIZE;
    int count = 0;


    /*ListNode* prev = NULL;
    ListNode* current = list_of_free_segments->head;
    while (current != NULL) {
        if (current->free_segments >= requiredSegments) {
            result.startIndex = current->address;
            return result;
        }
        prev = current;
        current = current->next;
    }*/

    for (int i = 0; i < totalSegments; i++) {
        if (segments[i].isFree) {
            count++;
            if (count == requiredSegments) {
                result.startIndex = i - requiredSegments + 1; //Ako je nadjen prostor za blok postavlja se startIndex
                return result;                                //i returnuje se rezultat
            }
        }
        else {
            count = 0;
        }
    }

    // Ako nije prosao return znaci da je count nakon for loopa jednak broju slobodnih segmenata na kraju
    // i takodje znaci da je startIndex ostao -1.
    // allocate_memory ce proveravati da li je startIndex -1. Ako jeste znaci da treba da prosiri broj segmenata
    result.missingSegments = requiredSegments - count;

    //if (prev == NULL) {
    //    result.missingSegments = requiredSegments;                       // Ako nema slobodnih segmenata u nizu (lista je prazna)
    //}
    //else if (prev->address + prev->free_segments == totalSegments) {
    //    result.missingSegments = requiredSegments - prev->free_segments; // Ako je posljednji segment u nizu (ili vise njih) slobodan
    //}
    //else {
    //    result.missingSegments = requiredSegments;                       // Ako posljednji segment u nizu nije slobodan
    //}
    
    return result;
}

// funkcija za alociranje bloka memorije odredjene velicine
void* allocate_memory(int size) {
    FirstFitResult fit = firstFit(size);
    int requiredSegments = (size + SEGMENT_SIZE - 1) / SEGMENT_SIZE;

    if (fit.startIndex == -1) {
        // Ako nije bilo mesta za blok, segmenti se prosiruju za broj segmenata koji fali
        addSegments(fit.missingSegments);
        fit.startIndex = totalSegments - requiredSegments; //sada kada je prosiren broj segmenata blok moze da se ugura
    }

    // Jedan po jedan od start indexa pa nadalje se zakljucavaju segmenti, menja im se isFree pa se otkljucavaju
    for (int i = fit.startIndex; i < fit.startIndex + requiredSegments; i++) {
        WaitForSingleObject(segments[i].mutex, INFINITE);
        segments[i].block_id = block_id;
        segments[i].isFree = false;
        ReleaseMutex(segments[i].mutex);
    }

    // Sada stvaramo blok
    TBlock* block = (TBlock*)malloc(sizeof(TBlock));
    block->id = block_id;
    block_id++;
    block->start_address = fit.startIndex;
    block->size = size;
    block->segments_taken = requiredSegments;

    //addBlockToList(list_of_free_segments, fit.startIndex, requiredSegments);

    //printList(list_of_free_segments);

    // Taj blok guramo u blockHashMap. kljuc je start adresa a vrednost je sam blok.
    put(blockHashMap, block->start_address, block);
    // Taj blok takodje guramo u blockAddressHashMap. kljuc i vrednost su start adresa bloka. 
    // Vrednost se kasnije moze promijeniti tokom defragmentacije.
    put(blockAddressHashMap, block->id, (void*)(intptr_t)block->start_address);

    // block->start_address je index
    return (void*)(intptr_t)block->id; //return ove funkcije je int konvertovan u (void*) start_addresse
    //ovu return adresu cemo ispisati klijentu na konzolu i on ce nju kasnije da koristi da bi oslobidio blok
}

// funckija za oslobadjanje bloka memorije na odredjenoj adresi
void free_memory(void* address) {
    free_memory_error = 0;
    // iz blockAddressHashMap izvlacimo trenutnu adresu i konvertujemo je u int
    int current_address = (int)(intptr_t)get(blockAddressHashMap, (intptr_t)address);

    // ERROR: Adresa ne postoji u hashmapi adresa
    if (current_address == -1) {
        free_memory_error = 1;
        printf("ERROR: Invalid address. There is no record for address: %d.\n", current_address);
        return;
    }

    // na osnovu te adrese izblacimo blok iz blockHashMap
    TBlock* block = (TBlock*)get(blockHashMap, current_address);
    // ERROR: Adresa ne postoji u hashmapi blokova
    if (block == (TBlock*)-1) {
        free_memory_error = 2;
        printf("ERROR: Invalid address. No block found at address: %d.\n", current_address);
        return;
    }

    // oslobadjamo svaki segment koji je pripadao tom bloku
    for (int i = block->start_address; i < block->start_address + block->segments_taken; i++) {
        // Lock the segment
        WaitForSingleObject(segments[i].mutex, INFINITE);

        // Mark the segment as free
        segments[i].isFree = true;
        segments[i].block_id = -1;
        // Unlock the segment
        ReleaseMutex(segments[i].mutex);
    }
    //append(list_of_free_segments, block->start_address, block->segments_taken);

    // brojimo slobodne segmente
    int freeSegmentsCount = 0;
    for (int i = 0; i < totalSegments; i++) {
        if (segments[i].isFree) {
            freeSegmentsCount++;
        }
    }

    /*ListNode* current = list_of_free_segments->head;
    while (current != NULL) {
        freeSegmentsCount += current->free_segments;
        current = current->next;
    }*/

    // ako ih je preko 5 brisemo one koji visak slobodnih segmenata sa pocetka
    if (freeSegmentsCount > 5) {
        //broj segmenata koje treba da izbrisemo
        int segmentsToRemove = freeSegmentsCount - 5;

        // prolazimo kroz sve segmente. kad god naidjemo na slobodan segment svim segmentima nakon njega se smanjuje adresa za 1
        // ovo se desava dok ne izbacimo segmentsToRemove broj segmenata
        for (int i = 0; i < totalSegments && segmentsToRemove > 0; i++) {
            if (segments[i].isFree) {
                // Za svaki ZAUZET segment nakon slobodnog segmenta kojeg izbacujemo
                // proveravamo da li postoji blok cija se start adresa podudara sa njegovom adresom
                // ukoliko postoji on ce se izvuci u affectedBlock i tu cemo mu smanjiti start addresu za 1
                for (int j = 0; j < totalSegments; j++) {
                    if (segments[j].isFree == false && segments[j].address > i) {
                        //izvukli smo blok cija se start adresa treba smanjiti za 1
                        TBlock* affectedBlock = (TBlock*)get(blockHashMap, segments[j].address);
                        if (affectedBlock != (TBlock*)-1) {
                            // nasli smo njegovu originalnu adresu
                            // zato sto je njegova trenutna startna adresa vrednost blockAddressHashMape
                            // a kljuc odatle je njegova originalna adresa
                            int id = findKeyByValue(blockAddressHashMap, (intptr_t)affectedBlock->start_address);
                            //printf("\n\nOriginal address: %d\n", original_address);
                            if (id != -1) {
                                // Smanjujemo mu trenutnu startnu adresu u blockHashMap
                                remove(blockHashMap, affectedBlock->start_address);
                                affectedBlock->start_address--;
                                //printf("New address: %d\n\n", affectedBlock->start_address);
                                put(blockHashMap, affectedBlock->start_address, affectedBlock);
                                // Azuriramo mu trenutnu startnu adresu u blockAddressHashMap na kljucu njegove originalne startne adrese
                                remove(blockAddressHashMap, id);
                                put(blockAddressHashMap, id, (void*)(intptr_t)affectedBlock->start_address);
                            }
                        }
                    }
                }

                // unutar segments niza svaki segment se pomera za 1 u nazad i njihova adresa se smanjuje za 1
                for (int j = i; j < totalSegments - 1; j++) {
                    segments[j] = segments[j + 1];
                    segments[j].address--;
                }

                // Ukupan broj segmenata se menja
                totalSegments--;
                segmentsToRemove--;
                i--; //posto su se u segments svi pomerili za 1 u nazad moramo i da pomerimo u nazad
            }
        }
    }
    // Sada bukvalno izbacujemo blok iz obe hash mape

    // za blockHashMapu koristimo njegovu trenutnu adresu koju smo izracunali na pocetku metode
    remove(blockHashMap, current_address);

    // za blockAddressHashMap koristimo njegovu originalnu adresu koja je zadata kao argument metode
    remove(blockAddressHashMap, (intptr_t)address);

    //formListFromSegments(list_of_free_segments, segments, totalSegments);
    //printList(list_of_free_segments);
}

// funckija koja deinicijalizuje sve strukture i promjenljive
void cleanup_segments() {
    // Step 1: Clean up each segment
    if (segments != NULL) {
        for (int i = 0; i < totalSegments; i++) {
            // Close the mutex for each segment
            if (segments[i].mutex != NULL) {
                CloseHandle(segments[i].mutex);
            }
        }
        // Free the segments array
        free(segments);
        segments = NULL; // Set to NULL to prevent dangling pointers
    }

    /*if (list_of_free_segments != NULL) {
        freeList(list_of_free_segments);
        free(list_of_free_segments);
        list_of_free_segments = NULL;
    }*/

    // Step 2: Clean up blockHashMap
    if (blockHashMap != NULL) {
        for (int i = 0; i < blockHashMap->size; i++) {
            HashMapEntry* entry = blockHashMap->table[i];
            while (entry != NULL) {
                HashMapEntry* nextEntry = entry->next;

                // Free the block stored in the value (if applicable)
                TBlock* block = (TBlock*)entry->value;
                if (block != NULL) {
                    free(block);
                }

                // Free the HashMapEntry itself
                free(entry);

                entry = nextEntry;
            }
        }
        // Free the hash map table and the hash map structure
        free(blockHashMap->table);
        free(blockHashMap);
        blockHashMap = NULL; // Set to NULL to prevent dangling pointers
    }

    // Step 3: Clean up blockAddressHashMap
    if (blockAddressHashMap != NULL) {
        for (int i = 0; i < blockAddressHashMap->size; i++) {
            HashMapEntry* entry = blockAddressHashMap->table[i];
            while (entry != NULL) {
                HashMapEntry* nextEntry = entry->next;

                // Free the HashMapEntry itself
                free(entry);

                entry = nextEntry;
            }
        }
        // Free the hash map table and the hash map structure
        free(blockAddressHashMap->table);
        free(blockAddressHashMap);
        blockAddressHashMap = NULL; // Set to NULL to prevent dangling pointers
    }

    // Step 4: Reset the totalSegments counter
    totalSegments = 0;
}

// funckija koja graficki prikazuje trenutni izgled segments niza
void drawMemorySegments() {
    if (segments == NULL || totalSegments <= 0) {
        printf("No memory segments to display.\n");
        return;
    }

    int maxColumns = 10; // max broj kolona
    printf("------------------------- SEGMENTS -------------------------\n\n");
    for (int rowStart = 0; rowStart < totalSegments; rowStart += maxColumns) {
        int rowEnd = (rowStart + maxColumns < totalSegments) ? rowStart + maxColumns : totalSegments;

        // Print addresses above the squares
        for (int i = rowStart; i < rowEnd; i++) {
            printf("%3d   ", segments[i].address);
        }
        printf("\n");

        // Print top line of squares
        for (int i = rowStart; i < rowEnd; i++) {
            printf("+---+ ");
        }
        printf("\n");

        // Print middle line with 'x' or space based on isFree
        for (int i = rowStart; i < rowEnd; i++) {
            if (segments[i].isFree) {
                printf("|   | "); // Print an empty box for free segments
            }
            else {
                printf("|%3d| ", segments[i].block_id); // Print the block_id for occupied segments
            }
        }
        printf("\n");

        // Print bottom line of squares
        for (int i = rowStart; i < rowEnd; i++) {
            printf("+---+ ");
        }
        printf("\n");

        // Add a blank line between rows for clarity
        printf("\n");
    }
}

void drawMemorySegments2() {
    if (segments == NULL || totalSegments <= 0) {
        printf("No memory segments to display.\n");
        return;
    }

    int maxColumns = 10; // max broj kolona
    printf("------------------------- SEGMENTS -------------------------\n\n");
    for (int rowStart = 0; rowStart < totalSegments; rowStart += maxColumns) {
        int rowEnd = (rowStart + maxColumns < totalSegments) ? rowStart + maxColumns : totalSegments;

        // Print addresses above the squares
        for (int i = rowStart; i < rowEnd; i++) {
            printf("%3d   ", segments[i].address);
        }
        printf("\n");

        // Print top line of squares
        for (int i = rowStart; i < rowEnd; i++) {
            printf("+---+ ");
        }
        printf("\n");

        // Print middle line with original start_address or space
        for (int i = rowStart; i < rowEnd; i++) {
            if (!segments[i].isFree) {
                // Find the block corresponding to the segment's current address
                int currentAddress = segments[i].address;
                TBlock* block = (TBlock*)get(blockHashMap, currentAddress);

                if (block != (void*)-1) {
                    int originalStartAddress = (int)(intptr_t)findKeyByValue(blockAddressHashMap, block->start_address);
                    if (originalStartAddress != -1) {
                        printf("|%3d| ", originalStartAddress);
                    }
                    else {
                        printf("| ? | "); // If the original start address is not found, show '?'
                    }
                }
                else {
                    // Find the previous filled segment and its block
                    int foundAddress = -1;
                    for (int j = i - 1; j >= 0; j--) {
                        if (!segments[j].isFree) {
                            int prevAddress = segments[j].address;
                            TBlock* prevBlock = (TBlock*)get(blockHashMap, prevAddress);
                            if (prevBlock != (void*)-1 && prevBlock->start_address == prevAddress) {
                                foundAddress = (int)(intptr_t)findKeyByValue(blockAddressHashMap, prevBlock->start_address);
                                break;
                            }
                        }
                    }
                    if (foundAddress != -1) {
                        printf("|%3d| ", foundAddress);
                    }
                    else {
                        printf("| ? | "); // If no valid block is found
                    }
                }
            }
            else {
                printf("|   | "); // For free segments
            }
        }
        printf("\n");

        // Print bottom line of squares
        for (int i = rowStart; i < rowEnd; i++) {
            printf("+---+ ");
        }
        printf("\n\n"); // Add a blank line between rows for clarity
    }
}
