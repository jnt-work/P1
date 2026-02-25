#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mymalloc.h"

// Macro for size of the heap
#define MEMLENGTH 4096

// Global heap array
static union {
    char bytes[MEMLENGTH];
    double not_used;
} heap;

// Check if heap is initialized
static int is_initialized = 0;

// Metadata header for every chunk
struct header {
    size_t size;   // Total size of chunk (header + payload)
    int is_allocated;   // 1 if allocated, 0 if free
    int padding;    // Padding to ensure 16-byte alignment
};

void leak_detector() {
    // Iterate through heap
    // If any chunk is allocated, count it and add its payload size
    // Print the result to stderr

    size_t total_leaked_bytes = 0;
    int leaked_objects_count = 0;

    char *current = heap.bytes;

    while (current < heap.bytes + MEMLENGTH) {
        struct header *curh = (struct header *)current;

        if (curh->is_allocated == 1) {
            leaked_objects_count++;
            total_leaked_bytes += (curh->size = sizeof(struct header)); // Add only payload size
        }
        current += curh->size;
    }

    if (leaked_objects_count > 0) {
        fprintf(stderr, "mymalloc: %zu bytes leaked in %d objects.\n", total_leaked_bytes, leaked_objects_count);
    }

}

void initialize_heap() {
    // Cast the start of the byte array to our struct pointer
    struct header *first_chunk = (struct header *)heap.bytes;

    first_chunk->size = MEMLENGTH;
    first_chunk->is_allocated = 0;

    // Run the leak detector when the program finishes
    atexit(leak_detector);

    is_initialized = 1;
}

void *mymalloc(size_t size, char *file, int line) {
    if (!is_initialized) {
        initialize_heap();
    }

    // Align requested size to the nearest multiple of 8
    size_t aligned_payload_size = (size + 7) & ~7;

    // Calculate total size needed for chunk
    size_t required_chunk_size = sizeof(struct header) + aligned_payload_size;

    // Traverse heap to find free chunk
    char *current = heap.bytes;

    while (current < heap.bytes + MEMLENGTH) {
        struct header *h = (struct header *)current;

        if (h->is_allocated == 0 && h->size >= required_chunk_size) {
            // Found a suitable chunk
            // Calculate how much space would be left if we split the chunk
            size_t leftover_space = h->size - required_chunk_size;

            // Only split if leftover space can hold a new header + payload
            if (leftover_space >= sizeof(struct header) + 8) {
                // Create a pointer for where the new header would be
                char *new_chunk_start = current + required_chunk_size;

                // Cast memory address to struct header
                struct header *new_h = (struct header *)new_chunk_start;

                // Set metadata
                new_h->size = leftover_space;
                new_h->is_allocated = 0;

                // Shrink current chunk to required size
                h->size = required_chunk_size;
            }

            // Mark current chunk as allocated
            h->is_allocated = 1;

            // Return pointer to payload
            return (void *)(current + sizeof(struct header));
        }
        current += h->size;
    }
    // No suitable chunk found, return NULL
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
    if (!is_initialized) initialize_heap();

    if (ptr == NULL) return;

    // Step backward to find start of chunk
    char *char_ptr = (char *)ptr;

    // ERROR CHECKING
    // Verify ptr is within the bounds of heap.bytes
    if (char_ptr < heap.bytes || char_ptr >= heap.bytes + MEMLENGTH) {
        fprintf(stderr, "free: Inappropriate pointer %p (%s:%d)\n", ptr, file, line);
        exit(2);
    }

    // Verify ptr actually points to the payload of a valid chunk by traversing the
    // heap from the start and seeing if any valid payload address matches 'ptr'
    char *current = heap.bytes;
    struct header *found = NULL;
    
    while (current < heap.bytes + MEMLENGTH) {
        struct header *curh = (struct header *)current;

        if (curh->size < (int)sizeof(struct header) ||
            current + curh->size > heap.bytes + MEMLENGTH) {
            fprintf(stderr, "free: Inappropriate pointer (Heap corruption) (%s:%d)\n", file, line);
            exit(2);
        }

        char *payload_start = current + sizeof(struct header);
        if (payload_start == char_ptr) {
            // Found the chunk that contains ptr
            found = curh;
            break;
        }
        current += curh->size;
    }

    if (found == NULL) {
        fprintf(stderr, "free: Inappropriate pointer %p (%s:%d)\n", ptr, file, line);
        exit(2);
    }

    // Double free check
    if(found->is_allocated == 0) {
        fprintf(stderr, "free: Inappropriate pointer (Double free detected) %p (%s:%d)\n", ptr, file, line);
        exit(2);
    }

    found->is_allocated = 0;

    // Coalesce adjacent free chunks
    // Traverse heap
    // If current chunk is free and next chunk is free, merge by adding the next chunk's size to current chunk's size
    current = heap.bytes;

    while (current < heap.bytes + MEMLENGTH) {
        struct header *curh = (struct header *)current;

        if (curh->is_allocated == 0) {
            char *next_chunk = current + curh->size;
            if (next_chunk < heap.bytes + MEMLENGTH) {
                struct header *nexth = (struct header *)(next_chunk);

                if (nexth->is_allocated == 0) {
                    curh->size += nexth->size;
                    continue;
                }
            }
        }
        current += curh->size;
    }

}

