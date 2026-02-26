#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

/* ------------------------------------------------------------------ */
/* Helper: run a function in a child process, check it exits with 2   */
/* ------------------------------------------------------------------ */
static void expect_exit2(const char *name, void (*fn)(void)) {
    pid_t pid = fork();
    if (pid < 0) {
        fail(name, "fork() failed");
        return;
    }
    if (pid == 0) {
        /* child: close stdout and stderr so output doesn't clutter parent */
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        fn();
        /* if fn() returns, it didn't call exit — that's a failure */
        _exit(0);
    }
    /* parent */
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 2)
        pass(name);
    else
        fail(name, "expected exit(2) but got different exit status");
}

/* ------------------------------------------------------------------ */
/* Test 6: free() detects non-heap pointer                             */
/* ------------------------------------------------------------------ */
static void do_free_nonheap(void) {
    int x = 42;
    free(&x);
}

static void test_free_nonheap(void) {
    expect_exit2("free() detects non-heap pointer", do_free_nonheap);
}

/* ------------------------------------------------------------------ */
/* Test 7: free() detects interior pointer                             */
/* ------------------------------------------------------------------ */
static void do_free_interior(void) {
    char *p = malloc(100);
    free(p + 10);
}

static void test_free_interior(void) {
    expect_exit2("free() detects interior pointer", do_free_interior);
}

/* ------------------------------------------------------------------ */
/* Test 8: free() detects double free                                  */
/* ------------------------------------------------------------------ */
static void do_free_doublefree(void) {
    char *p = malloc(100);
    free(p);
    free(p);
}

static void test_free_doublefree(void) {
    expect_exit2("free() detects double free", do_free_doublefree);
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

    printf("<---------------- Error Detection Tests ---------------->\n");

    test_free_nonheap();
    test_free_interior();
    test_free_doublefree();

    printf("<---------------- Results: %d passed, %d failed ---------------->\n", total_pass, total_fail);

    return total_fail ? EXIT_FAILURE : EXIT_SUCCESS;
}