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
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20190554",
    /* Your full name*/
    "JaeHyeon Song",
    /* Your email address */
    "938sjh@naver.com",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define CHUNKSIZE 8192

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define GET(ptr) (*(unsigned int *)(ptr))
#define PUT(ptr, val) (*(unsigned int *)(ptr) = (val))

#define BLK_SIZE(ptr) (GET(ptr)  & ~(0x7))
#define IS_ALLOC(ptr) (GET(ptr) & 0x1)

#define HEADER(ptr) ((void *)(ptr) - 4) //word size = 4byte
#define FOOTER(ptr) ((void *)(ptr) + BLK_SIZE(HEADER(ptr)) - ALIGNMENT)

#define NEXT_BLK(ptr) ((void *)(ptr) + BLK_SIZE(HEADER(ptr)))
#define PREV_BLK(ptr) ((void *)(ptr) - BLK_SIZE((void *)(ptr) - ALIGNMENT))

#define NEXT_PTR(ptr) (*(char **)(ptr))
#define PREV_PTR(ptr) (*(char **)(ptr+4))

//LIFO policy
static char *root_free_list = 0;
static char *heap_ptr = 0;

//NEXT, PREV 순으로 위치함

//LIFO

static void insert_node(void *ptr);
static void delete_node(void *ptr);
static void *coalesce(void *ptr);
static void *incr_heap(size_t size);
static void alloc_blk(void *ptr, size_t size);
static void *find_fit(size_t size);

static void insert_node(void *ptr)
{
    NEXT_PTR(ptr) = root_free_list;
    PREV_PTR(ptr) = NULL;
    PREV_PTR(root_free_list) = ptr;
    root_free_list = ptr;

}
static void delete_node(void *ptr)
{
    if(PREV_PTR(ptr))
        NEXT_PTR(PREV_PTR(ptr)) = NEXT_PTR(ptr);
    else
        root_free_list = NEXT_PTR(ptr);
    PREV_PTR(NEXT_PTR(ptr)) = PREV_PTR(ptr);
}
static void *coalesce(void *ptr)
{
    /*
       case 1: prev_block allocated, next_block allocated
       case 2: prev_block allocated, next_block free
       case 3: prev_block free, next_block allocated
       case 4: prev_block free, next_block free
    */
    
    //case 1 only insert in free_list 
    
    size_t next_flag = IS_ALLOC(HEADER(NEXT_BLK(ptr)));
    size_t prev_flag = IS_ALLOC(HEADER(PREV_BLK(ptr)));
    size_t new_size;

    if(PREV_BLK(ptr) == ptr)
    {
        prev_flag = 1;
    }

    //case 2 
    if(prev_flag && !next_flag)
    {
        new_size = BLK_SIZE(HEADER(ptr)) + BLK_SIZE(HEADER(NEXT_BLK(ptr)));
        delete_node(NEXT_BLK(ptr));
        PUT(HEADER(ptr), new_size);
        PUT(FOOTER(ptr), new_size);
    }
    //case 3
    else if(!prev_flag && next_flag)
    {
        new_size = BLK_SIZE(HEADER(ptr)) + BLK_SIZE(HEADER(PREV_BLK(ptr)));
        ptr = PREV_BLK(ptr);
        delete_node(ptr);
        PUT(HEADER(ptr), new_size);
        PUT(FOOTER(ptr), new_size);
    }
    //case 4
    else if(!prev_flag && !next_flag)
    {
        new_size = BLK_SIZE(HEADER(ptr)) + BLK_SIZE(HEADER(PREV_BLK(ptr))) + BLK_SIZE(HEADER(NEXT_BLK(ptr)));
        delete_node(PREV_BLK(ptr));
        delete_node(NEXT_BLK(ptr));
        ptr = PREV_BLK(ptr);
        PUT(HEADER(ptr), new_size);
        PUT(FOOTER(ptr), new_size);
    }
    insert_node(ptr);

    return ptr;
}

static void *incr_heap(size_t size)
{
    char *ptr;
    size_t new_size;

    new_size = ALIGN(size);
    
    if(new_size < 16)
        new_size = 16;
    
    ptr = mem_sbrk(new_size);
    if((int)ptr == -1)
        return NULL;
    
    PUT(HEADER(ptr), new_size);
    PUT(FOOTER(ptr), new_size);
    PUT(HEADER(NEXT_BLK(ptr)), 1); //size = 0, alloc = 1

    return coalesce(ptr);
}

static void alloc_blk(void *ptr, size_t size)
{
    size_t diff;
    
    diff = BLK_SIZE(HEADER(ptr)) - size;

    if(diff >= 2 * ALIGNMENT)
    {
        PUT(HEADER(ptr), size | 1);
        PUT(FOOTER(ptr), size | 1); //bit or 1 -> mark allocated
        delete_node(ptr);
        ptr = NEXT_BLK(ptr);

        PUT(HEADER(ptr), diff);
        PUT(FOOTER(ptr), diff); 
        coalesce(ptr);
    }
    else
    {
        PUT(HEADER(ptr), BLK_SIZE(HEADER(ptr)) | 1);
        PUT(FOOTER(ptr), BLK_SIZE(HEADER(ptr)) | 1); 
        delete_node(ptr);
    }
}

static void *find_fit(size_t size)
{
    void *ptr;
    for(ptr = root_free_list; IS_ALLOC(HEADER(ptr)) == 0; ptr = NEXT_PTR(ptr))
        if(size <= (size_t)BLK_SIZE(HEADER(ptr)))
            return ptr;

    return NULL;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    heap_ptr = mem_sbrk(4*ALIGNMENT);
    if(heap_ptr == NULL)
        return -1;
    
    PUT(heap_ptr, 0);
    PUT(heap_ptr + 4, ALIGNMENT | 1);
    PUT(heap_ptr + 8, ALIGNMENT | 1);
    PUT(heap_ptr + 12, 1);
    heap_ptr += 8;
    root_free_list = heap_ptr;

    if(incr_heap(16) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize;
    void *ptr;
    size_t request_size;

    if(size == 0)
        return NULL;
    
    if(size <= 8)
        newsize = 16;
    else
        newsize = 8 * ((size + 15) / 8);

    if((ptr = find_fit(newsize)) != NULL)
    {
        alloc_blk(ptr, newsize);
        return ptr;
    }

    if(newsize > CHUNKSIZE)
        request_size = newsize;
    else
        request_size = CHUNKSIZE;
    
    ptr = incr_heap(request_size);
    if(ptr == NULL)
        return NULL;
    alloc_blk(ptr, newsize);

    return ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size;

    if(ptr == NULL)
        return;
    size = BLK_SIZE(HEADER(ptr));
    PUT(HEADER(ptr), size);
    PUT(FOOTER(ptr), size);
    coalesce(ptr);    
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize, newsize;
    void *new_ptr;

    if(size < 0)
        return NULL;
    else if(size == 0)
    {
        mm_free(ptr);
        return NULL;
    }
    else
    {
        oldsize = BLK_SIZE(HEADER(ptr));
        newsize = size + 8; //HEADER, FOOTER
        
        if(oldsize >= newsize)
            return ptr;
        else
        {
            size_t comb_size;
            comb_size = oldsize + BLK_SIZE(HEADER(NEXT_BLK(ptr)));
            if(!IS_ALLOC(HEADER(NEXT_BLK(ptr))) && (comb_size >= newsize))
            {
                delete_node(NEXT_BLK(ptr));
                PUT(HEADER(ptr), comb_size | 1);
                PUT(FOOTER(ptr), comb_size | 1);

                return ptr;
            }
            else
            {
                new_ptr = mm_malloc(newsize);
                alloc_blk(new_ptr, newsize);
                memcpy(new_ptr, ptr, newsize);
                mm_free(ptr);
                return new_ptr;
            }
        }
    }
}
/*
int mm_check(void)
{
    void *ptr;

    for(ptr = root_free_list; IS_ALLOC(HEADER(ptr)) == 0 ; ptr = NEXT_PTR(ptr))
    {
        //marked as free?
        if(IS_ALLOC(HEADER(ptr)) != 0)
            return 1;
        //escaped coalescing?
        if(PREV_PTR(HEADER(ptr)) != NULL && HEADER(ptr) - FOOTER(PREV_PTR(HEADER(ptr))) == 4)
            return 1;
        //pointers in free list point to valid free block?
        if(ptr != root_free_list && PREV_PTR(ptr) == NULL)
            return 1;
        //valid free block?
        if(ptr > mem_heap_hi() || ptr < mem_heap_lo())
            return 1;
    }

    return 0;
}*/
