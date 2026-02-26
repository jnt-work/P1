#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mymalloc.h"

#define RUNS 50
#define OBJECTS 120

static double elapsed_us(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
}

/*
 * Task 1: malloc(1) then immediately free(), 120 times.
 */
static void task1(void) {
    for (int i = 0; i < OBJECTS; i++) {
        char *p = malloc(1);
        free(p);
    }
}

/*
 * Task 2: malloc(1) 120 times into an array, then free all 120.
 */
static void task2(void) {
    char *ptrs[OBJECTS];
    for (int i = 0; i < OBJECTS; i++)
        ptrs[i] = malloc(1);
    for (int i = 0; i < OBJECTS; i++)
        free(ptrs[i]);
}

/*
 * Task 3: Randomly choose between malloc(1) and free() until 120
 * objects have been allocated and freed. Free any remaining at the end.
 */
static void task3(void) {
    char *ptrs[OBJECTS];
    int allocated = 0;  /* next slot to allocate into */
    int freed = 0;      /* next slot to free from */
    int total = 0;      /* total allocations done so far */

    while (total < OBJECTS || freed < total) {
        if (freed == total) {
            /* nothing to free, must allocate */
            ptrs[allocated++] = malloc(1);
            total++;
        } else if (total >= OBJECTS) {
            /* can't allocate more, must free */
            free(ptrs[freed++]);
        } else if (rand() % 2 == 0) {
            ptrs[allocated++] = malloc(1);
            total++;
        } else {
            free(ptrs[freed++]);
        }
    }
}

/*
 * Task 4: Allocate objects of varying sizes (8, 16, 32, 64, 128 bytes),
 * then free in reverse order. Tests fragmentation with diverse sizes.
 */
static void task4(void) {
    int sizes[] = {8, 16, 32, 64, 128};
    int nsizes = 5;
    int count = 20; /* 20 objects per round, 4 per size */
    char *ptrs[20];

    for (int i = 0; i < count; i++)
        ptrs[i] = malloc(sizes[i % nsizes]);

    for (int i = count - 1; i >= 0; i--)
        free(ptrs[i]);
}

/*
 * Task 5: LIFO stack pattern — push 20 objects, pop all, repeat 6 rounds.
 * Tests repeated memory reuse.
 */
static void task5(void) {
    int rounds = 6;
    int depth = 20;
    char *stack[20];

    for (int r = 0; r < rounds; r++) {
        /* push */
        for (int i = 0; i < depth; i++)
            stack[i] = malloc(32);
        /* pop (LIFO) */
        for (int i = depth - 1; i >= 0; i--)
            free(stack[i]);
    }
}

int main(void) {
    struct timeval start, end;
    double total;

    typedef void (*taskfn)(void);
    taskfn tasks[] = {task1, task2, task3, task4, task5};
    const char *names[] = {
        "Task 1: malloc+free 120 times",
        "Task 2: malloc 120 then free 120",
        "Task 3: random malloc/free interleave",
        "Task 4: varying sizes, reverse free",
        "Task 5: LIFO stack pattern (6 rounds)"
    };

    for (int t = 0; t < 5; t++) {
        total = 0;
        for (int r = 0; r < RUNS; r++) {
            gettimeofday(&start, NULL);
            tasks[t]();
            gettimeofday(&end, NULL);
            total += elapsed_us(start, end);
        }
        printf("%s: %.2f us average over %d runs\n", names[t], total / RUNS, RUNS);
    }

    return 0;
}
