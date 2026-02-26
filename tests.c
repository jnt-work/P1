#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef REALMALLOC
#include "mymalloc.h"
#endif

#define MEMSIZE    4096
#define HEADERSIZE 8
#define OBJECTS    64
#define OBJSIZE    (MEMSIZE / OBJECTS - HEADERSIZE)

static int total_pass = 0;
static int total_fail = 0;

static void pass(const char *name) {
    printf("  PASS: %s\n", name);
    total_pass++;
}

static void fail(const char *name, const char *reason) {
    printf("  FAIL: %s — %s\n", name, reason);
    total_fail++;
}

/* ------------------------------------------------------------------ */
/* Test 1: malloc() reserves non-overlapping memory                    */
/* ------------------------------------------------------------------ */
static void test_no_overlap(void) {
    const char *name = "malloc() reserves non-overlapping memory";
    char *obj[OBJECTS];
    int i, j, errors = 0;

    for (i = 0; i < OBJECTS; i++) {
        obj[i] = malloc(OBJSIZE);
        if (obj[i] == NULL) {
            fail(name, "malloc returned NULL before heap was full");
            return;
        }
    }

    for (i = 0; i < OBJECTS; i++)
        memset(obj[i], i, OBJSIZE);

    for (i = 0; i < OBJECTS; i++) {
        for (j = 0; j < OBJSIZE; j++) {
            if ((unsigned char)obj[i][j] != (unsigned char)i)
                errors++;
        }
    }

    for (i = 0; i < OBJECTS; i++)
        free(obj[i]);

    if (errors == 0)
        pass(name);
    else
        fail(name, "some bytes were overwritten by a neighboring object");
}

/* ------------------------------------------------------------------ */
/* Test 2: free() deallocates memory                                   */
/* ------------------------------------------------------------------ */
static void test_free_works(void) {
    const char *name = "free() deallocates memory (re-allocation after full free)";
    char *obj[OBJECTS];
    int i, second_round_failed = 0;

    for (i = 0; i < OBJECTS; i++) {
        obj[i] = malloc(OBJSIZE);
        if (obj[i] == NULL) {
            fail(name, "round 1 allocation failed unexpectedly");
            return;
        }
    }

    for (i = 0; i < OBJECTS; i++)
        free(obj[i]);

    for (i = 0; i < OBJECTS; i++) {
        obj[i] = malloc(OBJSIZE);
        if (obj[i] == NULL)
            second_round_failed++;
    }

    for (i = 0; i < OBJECTS; i++) {
        if (obj[i] != NULL)
            free(obj[i]);
    }

    if (second_round_failed == 0)
        pass(name);
    else
        fail(name, "re-allocation after free failed — free() may not be deallocating");
}

/* ------------------------------------------------------------------ */
/* Test 3: Adjacent free chunks are coalesced                          */
/* ------------------------------------------------------------------ */
static void test_coalesce(void) {
    const char *name = "Adjacent free chunks are coalesced";

    #define COAL_OBJECTS 8
    #define COAL_OBJSIZE (MEMSIZE / COAL_OBJECTS - HEADERSIZE)
    #define COAL_BIGSIZE (MEMSIZE / 2 - HEADERSIZE)

    char *obj[COAL_OBJECTS];
    char *big = NULL;
    int i;

    for (i = 0; i < COAL_OBJECTS; i++) {
        obj[i] = malloc(COAL_OBJSIZE);
        if (obj[i] == NULL) {
            fail(name, "initial allocation failed");
            return;
        }
    }

    for (i = 0; i < COAL_OBJECTS; i++)
        free(obj[i]);

    big = malloc(COAL_BIGSIZE);
    if (big == NULL) {
        fail(name, "large allocation failed after freeing — coalescing may be broken");
        return;
    }

    free(big);
    pass(name);
}

/* ------------------------------------------------------------------ */
/* Test 4: malloc() returns NULL and doesn't crash on a full heap      */
/* ------------------------------------------------------------------ */
static void test_null_on_full(void) {
    const char *name = "malloc() returns NULL when heap is exhausted";
    char *obj[OBJECTS];
    int i, allocated = 0, errors = 0;

    for (i = 0; i < OBJECTS; i++) {
        obj[i] = malloc(OBJSIZE);
        if (obj[i] == NULL) break;
        memset(obj[i], (unsigned char)i, OBJSIZE);
        allocated++;
    }

    for (i = 0; i < 3; i++) {
        char *extra = malloc(OBJSIZE);
        if (extra != NULL) {
            errors++;
            free(extra);
        }
    }

    for (i = 0; i < allocated; i++) {
        int j;
        for (j = 0; j < OBJSIZE; j++) {
            if ((unsigned char)obj[i][j] != (unsigned char)i)
                errors++;
        }
    }

    for (i = 0; i < allocated; i++)
        free(obj[i]);

    if (errors == 0)
        pass(name);
    else
        fail(name, "malloc() returned non-NULL on exhausted heap, or corrupted existing data");
}

/* ------------------------------------------------------------------ */
/* Test 5: Leak detection fires at exit                                */
/*                                                                     */
/* This test intentionally leaks memory. You should see a message on  */
/* stderr at program exit, like:                                       */
/*   mymalloc: 160 bytes leaked in 4 objects.                         */
/* ------------------------------------------------------------------ */
static void test_leak(void) {
    const char *name = "Leak detector (check stderr at exit for leak message)";
    int i;

    for (i = 0; i < 4; i++) {
        void *p = malloc(40);
        if (p == NULL) {
            fail(name, "malloc returned NULL unexpectedly");
            return;
        }
    }

    printf("  INFO: 4 objects of 40 bytes leaked intentionally.\n");
    printf("        Check stderr output at program exit for the leak report.\n");
    pass(name);
}

int

main(void)
{
    printf("<---------------- mymalloc Correctness Tests ---------------->\n");

    test_no_overlap();
    test_free_works();
    test_coalesce();
    test_null_on_full();
    test_leak();  

    printf("<---------------- Results: %d passed, %d failed ---------------->\n", total_pass, total_fail);
    printf("\nReminder: run error detection tests manually:\n");
    printf("  ./test_err_nonheap    (expect: error msg + exit code 2)\n");
    printf("  ./test_err_interior   (expect: error msg + exit code 2)\n");
    printf("  ./test_err_doublefree (expect: error msg + exit code 2)\n");

    return total_fail ? EXIT_FAILURE : EXIT_SUCCESS;
}