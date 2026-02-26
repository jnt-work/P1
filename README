CS 214 Project 1: My little malloc()

Authors
-------
James Ntiamoah  (jyn13)
Jai Patel       (jmp704)


Files
-----
mymalloc.h    - Header file with malloc/free macros and function prototypes.
mymalloc.c    - Implementation of mymalloc(), myfree(), heap initialization,
                and leak detection.
memgrind.c    - Stress testing program with 5 workloads, each run 50 times.
tests.c       - Correctness and error detection test suite (8 tests total).
Makefile      - Build rules for memgrind (default), tests, and clean.
AUTHOR        - Partner netIDs.
README        - This file.


Building
--------
  make            Build memgrind (default target).
  make tests      Build the test suite.
  make clean      Remove all compiled binaries.


Test Plan
---------
Our testing strategy follows the structure recommended by the assignment:
for each requirement, we identify a detection method and implement a test
that checks for violations. All tests are in a single program (tests.c)
and take no command-line arguments.

To run:    make tests && ./tests

The test program prints PASS or FAIL for each test and a summary line at
the end. It exits with EXIT_SUCCESS if all tests pass, EXIT_FAILURE
otherwise. The leak detector will also print to stderr at program exit
(test 5 intentionally leaks memory to verify this).


Correctness Tests (tests 1-5)
-----------------------------

Test 1: malloc() reserves non-overlapping memory

  Requirement:  On success, malloc() returns a pointer to a payload that
                does not overlap any other allocated payload.
  Detection:    Allocate 64 objects that fill the entire heap. Write a
                distinct byte pattern to each object (object i is filled
                with byte value i). Then verify that every byte in every
                object still matches its expected pattern. If any byte was
                overwritten, two objects must overlap.
  Test:         test_no_overlap() allocates 64 objects of 56 bytes each
                (56 + 8 byte header = 64 bytes per chunk, 64 * 64 = 4096).
                It fills each with memset, then scans all bytes for
                corruption. All objects are freed afterward.

Test 2: free() deallocates memory

  Requirement:  free() marks a chunk as available so that subsequent calls
                to malloc() can reuse the space.
  Detection:    Fill the heap completely, free everything, then attempt to
                fill it again. If the second round of allocations fails,
                free() did not properly release memory.
  Test:         test_free_works() allocates 64 objects, frees all 64, then
                allocates 64 again. Passes if the second round succeeds
                without any NULL returns.

Test 3: Adjacent free chunks are coalesced

  Requirement:  After freeing, adjacent free chunks must be merged so that
                larger allocations can be satisfied from previously
                fragmented space.
  Detection:    Fragment the heap into many small chunks, free all of them,
                then request a single large allocation that spans multiple
                former chunks. If coalescing is broken, the large allocation
                will fail.
  Test:         test_coalesce() allocates 8 objects of 504 bytes each (fills
                the heap). It frees all 8, then requests a single allocation
                of 2040 bytes (half the heap minus header). This can only
                succeed if free() merged the 8 small free chunks into one
                large free chunk.

Test 4: malloc() returns NULL when the heap is exhausted

  Requirement:  When no free chunk is large enough to satisfy a request,
                malloc() must return NULL and print an error to stderr
                rather than crashing or corrupting data.
  Detection:    Fill the heap, then make additional allocation requests.
                Verify they all return NULL. Also verify that the existing
                objects were not corrupted by the failed allocation attempts.
  Test:         test_null_on_full() fills the heap with 64 objects, writes
                byte patterns to each, then calls malloc() 3 more times.
                It checks that all 3 extra calls return NULL and that the
                original objects are still intact.

Test 5: Leak detection fires at exit

  Requirement:  The leak detector, registered with atexit(), must report
                allocated objects that were never freed.
  Detection:    Intentionally leak objects and verify that the leak detector
                prints the correct count and byte total to stderr when the
                process exits.
  Test:         test_leak() allocates 4 objects of 40 bytes each and does
                not free them. At program exit, the leak detector should
                print: "mymalloc: 160 bytes leaked in 4 objects." to stderr.
                The tester must visually confirm this message appears.


Error Detection Tests (tests 6-8)
---------------------------------
Because myfree() calls exit(2) on error (which would kill the test
program), each error detection test uses fork() to run the offending
operation in a child process. The parent waits for the child and checks
that it exited with status 2. The child closes stdout and stderr to
suppress error messages from cluttering the test output.

Test 6: free() detects a non-heap pointer

  Requirement:  Calling free() with a pointer not obtained from malloc()
                (e.g., a stack address) must be detected and reported.
  Detection:    Pass the address of a local stack variable to free().
                The library should print an error and exit(2).
  Test:         test_free_nonheap() forks a child that calls free(&x) on
                a local int. Passes if the child exits with status 2.

Test 7: free() detects an interior pointer

  Requirement:  Calling free() with a pointer that points inside a chunk
                payload (but not at its start) must be detected.
  Detection:    Allocate an object, then call free() with the pointer
                offset by 10 bytes. The library should detect that no
                chunk begins at that address.
  Test:         test_free_interior() forks a child that calls
                free(p + 10) where p was returned by malloc(100). Passes
                if the child exits with status 2.

Test 8: free() detects a double free

  Requirement:  Calling free() on a pointer that has already been freed
                must be detected.
  Detection:    Allocate an object, free it, then free it again. The
                library should detect that the chunk is already marked free.
  Test:         test_free_doublefree() forks a child that calls free(p)
                twice on the same pointer. Passes if the child exits with
                status 2.


Test Program Descriptions
-------------------------

tests (built from tests.c)
  Usage:      ./tests
  Arguments:  None.
  Output:     Prints PASS/FAIL for each of the 8 tests to stdout, followed
              by a summary line with total passed and failed counts. The
              leak detector prints to stderr at exit (expected due to
              test 5). Exit code is 0 if all tests pass, 1 otherwise.

memgrind (built from memgrind.c)
  Usage:      ./memgrind
  Arguments:  None.
  Output:     Prints the average execution time in microseconds for each of
              the 5 workloads (50 runs each). See below for workload
              descriptions.


Memgrind Workloads
------------------

Task 1: malloc() and immediately free() a 1-byte object, 120 times.
  Purpose:    Tests basic allocation and deallocation in rapid succession.
              Verifies that the allocator can handle a tight alloc/free
              cycle without accumulating fragmentation.

Task 2: Allocate 120 1-byte objects into an array, then free all 120.
  Purpose:    Tests the allocator under sustained load. All 120 objects
              coexist in the heap simultaneously (120 chunks of 16 bytes =
              1920 bytes), then are freed in order. Exercises the
              coalescing logic when many adjacent chunks are freed
              sequentially.

Task 3: Randomly interleave malloc() and free() until 120 total
        allocations have been made, then free any remaining objects.
  Purpose:    Simulates a realistic workload where allocations and
              deallocations are intermixed unpredictably. Tests that the
              allocator handles fragmentation correctly when free chunks
              appear at random positions in the heap.

Task 4: Allocate 20 objects of varying sizes (8, 16, 32, 64, 128 bytes
        cycling), then free all in reverse order.
  Purpose:    Tests fragmentation handling with diverse object sizes.
              Reverse-order freeing means chunks are freed from the end
              of the heap backward, exercising coalescing in the opposite
              direction from sequential freeing.

Task 5: LIFO stack pattern -- push 20 objects of 32 bytes, pop all,
        repeat for 6 rounds (120 total allocations).
  Purpose:    Simulates a call-stack-like memory pattern where objects are
              allocated in bursts and freed in LIFO order. Tests whether
              the allocator efficiently reuses the same region of memory
              across repeated rounds without growing fragmentation.


Design Notes
------------

1. Header structure (8 bytes)

   Each chunk has an 8-byte header containing two int fields: size (total
   chunk size including header) and is_allocated (1 or 0). Using int
   instead of size_t saves space -- the heap is only 4096 bytes, well
   within int range. This yields the minimum possible chunk size of 16
   bytes (8-byte header + 8-byte minimum payload), maximizing the number
   of objects that can be allocated simultaneously.

2. Implicit free list

   Chunks are stored contiguously in the heap array with no explicit
   next/prev pointers. Given a chunk's address and its size field, the
   next chunk is found by simple pointer arithmetic (address + size).
   Traversal starts at heap.bytes and continues until the end of the
   array.

3. Alignment

   All payload sizes are rounded up to the nearest multiple of 8 using
   the expression (size + 7) & ~7. Since the header is exactly 8 bytes,
   every chunk boundary falls on an 8-byte-aligned offset from the start
   of the heap, satisfying the alignment requirement for all standard
   data types up to 8 bytes (including double and pointers).

4. Splitting policy

   When a free chunk is larger than needed, it is split into two chunks
   only if the leftover space can hold at least one minimum-size chunk
   (header + 8 bytes of payload = 16 bytes). This prevents creating
   unusably small fragments.

5. Coalescing

   Coalescing is performed eagerly in myfree() immediately after marking
   a chunk as free. A single forward pass through the heap merges all
   pairs of adjacent free chunks. The pass uses a continue statement to
   handle chains of multiple adjacent free chunks in a single traversal.

6. Lazy initialization

   The heap is initialized on the first call to mymalloc() or myfree()
   via a static flag (is_initialized). The initialization function sets
   up the entire heap as one free chunk of 4096 bytes and registers the
   leak detector with atexit().

7. Error detection in free()

   myfree() performs three checks before freeing:
   (a) Bounds check: the pointer must fall within heap.bytes.
   (b) Validity check: the heap is traversed from the start to verify
       that the pointer matches the payload address of some chunk. This
       catches both non-heap pointers that happen to fall within the
       array and interior pointers that point into the middle of a chunk.
   (c) Double-free check: the matched chunk must be currently allocated.
   All three errors print a diagnostic to stderr (including the source
   file and line number via the __FILE__/__LINE__ macros) and terminate
   with exit(2).

8. Leak detector

   Registered via atexit() during initialization. Traverses the heap and
   counts all chunks still marked as allocated, summing their payload
   sizes (chunk size minus header size). If any leaks are found, prints
   the total bytes and object count to stderr. The leak detector does not
   call exit() itself, as required by the spec.

9. Fork-based error testing

   Since myfree() calls exit(2) on errors, error detection tests cannot
   run in the main process without killing the test suite. Each error
   test uses fork() to isolate the bad operation in a child process. The
   parent calls waitpid() and checks that the child exited with status 2.
   The child closes stdout and stderr before executing the error case to
   prevent the child's output from interleaving with the parent's test
   results.
