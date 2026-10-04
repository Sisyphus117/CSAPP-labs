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

void *head = NULL;
void *cur = NULL;
void *tail = NULL;

#define BLOCK_SIZE 0xfffffff8 // header+payload
#define ALLOCATED 0x1         // flag bit for allocation

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    head = mem_sbrk(SIZE_T_SIZE);
    if (head == (void *)-1)
    {
        return 1;
    }
    cur = head;
    tail = head;
    *(size_t *)head = SIZE_T_SIZE;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size = ALIGN(size);
    size_t newsize = ALIGN(size + SIZE_T_SIZE);

    size_t before_size = 0;
    while (1)
    {
        size_t cur_val = *(size_t *)cur;
        size_t b_size = cur_val & BLOCK_SIZE;
        if (cur_val & ALLOCATED)
        {
            if (before_size != 0) // update free blocks
            {
                void *last_free = (char *)cur - before_size;
                *(size_t *)last_free = before_size;
                before_size = 0;
            }
            if (cur == tail)
            {
                cur = head;
                break;
            }
            cur = cur + b_size;
            continue;
        }

        void *free_start = (char *)cur - before_size;

        before_size += b_size;
        if (before_size >= newsize)
        {
            if (before_size > newsize)
            {
                size_t rest = before_size - newsize;
                void *rest_start = (char *)free_start + newsize;
                *(size_t *)rest_start = rest; // not allocated
                if (cur == tail)
                {
                    tail = rest_start;
                }
                cur = rest_start; // scan from here in the  next run
            }
            else
            {
                if (cur == tail)
                {
                    tail = free_start;
                    cur = head;
                }
                else
                    cur = (char *)cur + b_size;
            }
            *(size_t *)free_start = newsize + ALLOCATED;
            return (void *)((char *)free_start + SIZE_T_SIZE);
        }
        if (cur == tail)
        {
            size_t extend_size = newsize - before_size;
            void *p = mem_sbrk(extend_size);
            if (p == (void *)-1)
                return NULL;
            *(size_t *)free_start = newsize + ALLOCATED;
            tail = free_start;
            cur = head;
            return (void *)((char *)free_start + SIZE_T_SIZE);
        }
        cur = cur + b_size;
    }

    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = newsize + ALLOCATED;
        tail = p;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
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
    size_t *p = (size_t *)((char *)ptr - SIZE_T_SIZE);
    (*p) = (*p) & BLOCK_SIZE;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE) & BLOCK_SIZE;
    copySize -= SIZE_T_SIZE;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
