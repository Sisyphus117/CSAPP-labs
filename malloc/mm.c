/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define WORD_SIZE (sizeof(size_t)) // 4B, header/footer
#define MIN_BLOCK_SIZE 16
#define HDRP(bp) ((size_t *)(bp))
#define PREVP(bp) ((size_t *)(bp) + 1)
#define NEXTP(bp) ((size_t *)(bp) + 2)
#define FTRP(bp, size) ((size_t *)((char *)(bp) + (size) - WORD_SIZE))

void *head = NULL;

#define BLOCK_SIZE 0xfffffff8 // header+payload
#define ALLOCATED 0x1         // flag bit for allocation
#define IFALLOCATED(bp) ((*(size_t *)(bp)) & ALLOCATED)
#define GETSIZE(bp) ((*(size_t *)(bp)) & BLOCK_SIZE)

static void insert_free(void *bp)
{
    *PREVP(bp) = 0;
    *NEXTP(bp) = (size_t)head;
    if (head != NULL)
        *PREVP(head) = (size_t)bp;
    head = bp;
}

static void remove_free(void *bp)
{
    size_t prev = *PREVP(bp);
    size_t next = *NEXTP(bp);

    if (prev != 0)
        *NEXTP((void *)prev) = next;
    else
        head = (void *)next;
    if (next != 0)
        *PREVP((void *)next) = prev;
}

static void *find_fit(size_t asize)
{
    void *bp;

    for (bp = head; bp != NULL; bp = (void *)*NEXTP(bp))
    {
        if (GETSIZE(bp) >= asize)
            return bp;
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GETSIZE(bp);

    remove_free(bp);
    if (csize - asize >= MIN_BLOCK_SIZE)
    {
        size_t rsize = csize - asize;
        void *rbp = (char *)bp + asize;
        *HDRP(rbp) = rsize;
        *FTRP(rbp, rsize) = rsize;
        insert_free(rbp);
        *HDRP(bp) = asize | ALLOCATED;
        *FTRP(bp, asize) = asize;
    }
    else
    {
        *HDRP(bp) = csize | ALLOCATED;
        *FTRP(bp, csize) = csize;
    }
}

static void *extend_heap(size_t asize)
{
    char *heap_end = (char *)mem_heap_hi() + 1;
    size_t last_size = GETSIZE(heap_end - WORD_SIZE);
    void *last_bp = heap_end - last_size;

    if (!IFALLOCATED(last_bp))
    {
        size_t deficit = asize - last_size;
        if (mem_sbrk(deficit) == (void *)-1)
            return NULL;
        last_size += deficit;
        *HDRP(last_bp) = last_size;
        *FTRP(last_bp, last_size) = last_size;
        return last_bp;
    }

    void *bp = mem_sbrk(asize);
    if (bp == (void *)-1)
        return NULL;
    *HDRP(bp) = asize;
    *FTRP(bp, asize) = asize;
    insert_free(bp);
    return bp;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *p = mem_sbrk(3 * WORD_SIZE);
    if (p == (void *)-1)
        return 1;

    *HDRP((char *)p + WORD_SIZE) = 8 | ALLOCATED;
    *FTRP((char *)p + WORD_SIZE, 8) = 8;
    head = NULL;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize = ALIGN(size) + 2 * WORD_SIZE;
    if (asize < MIN_BLOCK_SIZE)
        asize = MIN_BLOCK_SIZE;

    void *bp = find_fit(asize);
    if (bp == NULL)
    {
        bp = extend_heap(asize);
        if (bp == NULL)
            return NULL;
    }
    place(bp, asize);
    return (char *)bp + WORD_SIZE;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    void *bp = (char *)ptr - WORD_SIZE;
    size_t size = GETSIZE(bp);

    size_t prev_size = GETSIZE((char *)bp - WORD_SIZE);
    void *prev_bp = (char *)bp - prev_size;
    if (!IFALLOCATED(prev_bp))
    {
        remove_free(prev_bp);
        size += prev_size;
        bp = prev_bp;
    }

    char *heap_end = (char *)mem_heap_hi() + 1;
    void *next_bp = (char *)bp + size;
    if ((char *)next_bp < heap_end && !IFALLOCATED(next_bp))
    {
        remove_free(next_bp);
        size += GETSIZE(next_bp);
    }

    *HDRP(bp) = size;
    *FTRP(bp, size) = size;
    insert_free(bp);
}

/*
 * mm_realloc - Resize a block in place when possible.
 *   - Shrinking: keep the block in place and push the remainder onto the
 *     free list.
 *   - Growing: if the block is the last one, extend the heap by the deficit;
 *     otherwise, if the following block is free and large enough, absorb it.
 *   Falls back to mm_malloc/memcpy/mm_free when no in-place option exists.
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    void *bp = (char *)ptr - WORD_SIZE;
    size_t csize = GETSIZE(bp);
    size_t asize = ALIGN(size) + 2 * WORD_SIZE;
    if (asize < MIN_BLOCK_SIZE)
        asize = MIN_BLOCK_SIZE;

    /* Shrink in place and free the tail. */
    if (asize <= csize)
    {
        if (csize - asize >= MIN_BLOCK_SIZE)
        {
            size_t rsize = csize - asize;
            void *rbp = (char *)bp + asize;
            *HDRP(rbp) = rsize;
            *FTRP(rbp, rsize) = rsize;
            insert_free(rbp);
            *HDRP(bp) = asize | ALLOCATED;
            *FTRP(bp, asize) = asize;
        }
        return ptr;
    }

    /* Grow in place. */
    size_t deficit = asize - csize;
    char *heap_end = (char *)mem_heap_hi() + 1;
    void *next_bp = (char *)bp + csize;

    if (next_bp == (void *)heap_end)
    {
        /* Last block: extend the heap like extend_heap does. */
        if (mem_sbrk(deficit) != (void *)-1)
        {
            size_t nsize = csize + deficit;
            *HDRP(bp) = nsize | ALLOCATED;
            *FTRP(bp, nsize) = nsize;
            return ptr;
        }
    }
    else if (!IFALLOCATED(next_bp) && GETSIZE(next_bp) >= deficit)
    {
        /* Absorb the following free block. */
        size_t nsize = GETSIZE(next_bp);
        remove_free(next_bp);
        size_t total = csize + nsize;
        if (total - asize >= MIN_BLOCK_SIZE)
        {
            size_t rsize = total - asize;
            void *rbp = (char *)bp + asize;
            *HDRP(rbp) = rsize;
            *FTRP(rbp, rsize) = rsize;
            insert_free(rbp);
            *HDRP(bp) = asize | ALLOCATED;
            *FTRP(bp, asize) = asize;
        }
        else
        {
            *HDRP(bp) = total | ALLOCATED;
            *FTRP(bp, total) = total;
        }
        return ptr;
    }

    /* Fallback: allocate, copy, free. */
    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    size_t copySize = csize - 2 * WORD_SIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}
