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
    "ccb",
    /* First member's full name */
    "LessTanker",
    /* First member's email address */
    "sunqx2006@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
// #define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
// #define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define SEGREGATED_FREE_LIST
#define DEBUG

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) ((*(unsigned int*)(p)) = (val)) 

#define PACK(size, alloc) ((unsigned int)((size) | (alloc)))

#define GET_SIZE(p) ((GET(p)) & (~0x7))
#define GET_ALLOC(p) ((GET(p)) & (0x1))

#define HDPR(bp) ((char*)(bp) - WSIZE)
#define FTPR(bp) ((char*)(bp) + GET_SIZE(HDPR(bp)) - DSIZE)

#define NEXT_BLKPR(bp) ((char*)(bp) + GET_SIZE(HDPR(bp)))
#define PREV_BLKPR(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

typedef struct freeblk{
    struct freeblk *prev;
    struct freeblk *next;
}freeblk_t;

#define LIST_NUM 28

static freeblk_t *free_lists[LIST_NUM];

static char *head_listp = 0;

static void* extend_heap(size_t words);
static void* coalesce(void *bp);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);
static int get_freelist_index(size_t size);
static void ins_freeblk(void* bp);
static void rm_freeblk(freeblk_t* fb);
static void print_freelst();

#ifdef DEBUG
    size_t get_size(void* bp){ return GET_SIZE(HDPR(bp));}
    size_t get_alloc(void* bp){ return GET_ALLOC(HDPR(bp));}
#endif
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* mm_init 开头：初始化 free_lists */
    for (int i = 0; i < LIST_NUM; i++) 
        free_lists[i] = NULL;
    //调用mem_sbrk函数为序言块和结尾块分配足够的空间
    if((head_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;
    //用于对齐
    PUT(head_listp, 0);
    //序言块大小为DSIZE，标记为已分配
    PUT(head_listp + WSIZE, PACK(DSIZE, 1));
    PUT(head_listp + 2*WSIZE, PACK(DSIZE, 1));
    //结尾块只包含头指针，且将大小标记为0用于指示结尾
    PUT(head_listp + 3*WSIZE, PACK(0, 1));
    head_listp += 2*WSIZE;
    //初始分配CHUNKSIZE大小的空间，并用head_listp标记
    if(extend_heap(CHUNKSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; //对齐后实际需要的空间
    size_t extend_size; //如果没有空闲块，需要额外申请的空间
    char* bp; //返回被分配块的首地址的指针

    //如果未初始化，先初始化
    if(head_listp == 0)
        mm_init();

    //申请空间为0，直接返回NULL
    if(size == 0)
        return NULL;

    //调整实际需要的空间
    //申请空间最小为2*DSIZE，否则分配DSIZE倍数的空间
    if(size < 2 * DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE -1)) / DSIZE);

#ifdef DEBUG
    fprintf(stderr, "[mm_malloc] [enter] size = %u, asize=%u\n", size, asize);
//    print_freelst();
#endif

    //寻找适配的空闲块并分配空间
    if((bp = find_fit(asize)) != NULL)
    {

#ifdef DEBUG
    fprintf(stderr, "[mm_malloc] [successfully find bp] bp=%p, asize=%u\n", bp, asize);
#endif

        place(bp, asize);
        return bp;
    }

#ifdef DEBUG
    fprintf(stderr, "[mm_malloc] [Not find fit, need extend] size=%u, asize=%u\n", size, asize);
#endif

    //没有合适大小的空闲块，向堆申请额外的内存空间并重新分配
    extend_size = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extend_size)) == NULL)
        return NULL;

#ifdef DEBUG
    fprintf(stderr, "[mm_malloc] [after-extend] bp=%p, asize=%u\n", bp, asize);
#endif

    place(bp, asize);

#ifdef DEBUG
    fprintf(stderr, "[mm_malloc] [after-place] bp = %p\n", bp);
    print_freelst();
#endif

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    //处理传入参数为空指针的情况
    if(ptr == NULL)
        return;
    size_t size = GET_SIZE(HDPR(ptr));
    //检查
    if(head_listp == 0)
        mm_init();
    //修改头尾指针并合并相邻的空闲块
    PUT(HDPR(ptr), PACK(size, 0));
    PUT(FTPR(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    //ptr为NULL时变成malloc
    if(ptr == NULL)
        return mm_malloc(size);
    //size为0时变为free
    if(size == 0)
    {
        mm_free(ptr);
        return 0;
    } 
    //不判断oldsize与size的大小比较，直接分配一段堆空间并拷贝过去
    void* newptr = mm_malloc(size);

    if(!newptr) 
        return 0;

    size_t oldsize = GET_SIZE(HDPR(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    mm_free(ptr);
    return newptr;
}

static void* extend_heap(size_t words)
{
    char* bp; //标记新分配的块
    size_t size;
    //对齐到DSIZE
    // size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    size = (words + (DSIZE - 1)) & ~(DSIZE - 1);
    //转换为long类型确保不丢失信息
    //如果size过大会失败，此时返回NULL
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

#ifdef DEBUG
    fprintf(stderr, "[extend_heap] mem_sbrk returned bp=%p size=%u heap_lo=%p heap_hi=%p\n",
            bp, (unsigned)size, mem_heap_lo(), mem_heap_hi());
#endif

    //设置头尾指针
    PUT(HDPR(bp), PACK(size, 0));
    PUT(FTPR(bp), PACK(size, 0));
    //更新结尾块，原先结尾块被覆盖，不需要单独处理
    PUT(HDPR(NEXT_BLKPR(bp)), PACK(0,1));
    //如果上一个块未被分配，需要进行合并
    return coalesce(bp);
}

static void* coalesce(void *bp)
{
    //计算前后块是否空闲
    size_t prev_alloc = GET_ALLOC(FTPR(PREV_BLKPR(bp)));
    size_t next_alloc = GET_ALLOC(HDPR(NEXT_BLKPR(bp)));

#ifdef DEBUG
    fprintf(stderr, "[coalesce] enter bp=%p hdr=%x size=%u prev_alloc=%u next_alloc=%u\n",
        bp, GET(HDPR(bp)), (unsigned)GET_SIZE(HDPR(bp)), (unsigned)prev_alloc, (unsigned)next_alloc);
#endif

    //计算前后和当前块的大小
    size_t size = GET_SIZE(HDPR(bp));

#ifdef SEGREGATED_FREE_LIST
    if(!prev_alloc) 
        rm_freeblk((freeblk_t*)(PREV_BLKPR(bp)));
    if(!next_alloc)
        rm_freeblk((freeblk_t*)(NEXT_BLKPR(bp)));
#endif

    if(prev_alloc && next_alloc)
    {
        //前后块都已分配，直接返回bp
    }
    else if(prev_alloc && !next_alloc)
    {
        //后面的块空闲，更新当前块的头尾指针
        size += GET_SIZE(HDPR(NEXT_BLKPR(bp)));
        PUT(HDPR(bp), PACK(size, 0));
        PUT(FTPR(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc)
    {
        //前面的块空闲，更新前面的块的头尾指针，同时更新bp
        size += GET_SIZE(FTPR(PREV_BLKPR(bp)));
        PUT(HDPR(PREV_BLKPR(bp)), PACK(size, 0));
        PUT(FTPR(PREV_BLKPR(bp)), PACK(size, 0));
        bp = PREV_BLKPR(bp);
    }
    else
    {
        //前后块都空闲，更新前面块的头尾指针，更新bp
        size += GET_SIZE(FTPR(PREV_BLKPR(bp))) + GET_SIZE(HDPR(NEXT_BLKPR(bp)));
        PUT(HDPR(PREV_BLKPR(bp)), PACK(size, 0));
        PUT(FTPR(PREV_BLKPR(bp)), PACK(size, 0));
        bp = PREV_BLKPR(bp);
    }

#ifdef DEBUG
    fprintf(stderr, "[coalesce] final bp=%p size=%u\n", bp, (unsigned)GET_SIZE(HDPR(bp)));
#endif

#ifdef SEGREGATED_FREE_LIST
    ins_freeblk(bp);
#endif

    return bp;
}

static void* find_fit(size_t asize)
{
#ifdef SEGREGATED_FREE_LIST
    int index = get_freelist_index(asize);

#ifdef DEBUG
    fprintf(stderr, "[find_fit] [enter] index=%d\n", index);
//    print_freelst();
#endif

    for(int i=index;i<LIST_NUM;i++)
    {
        freeblk_t *fb = free_lists[i];

#ifdef DEBUG
    fprintf(stderr, "[find_fit] [before while loop] index=%d, i=%d, fb=%p\n", index, i, fb);
#endif

        while(fb)
        {
            if (!GET_ALLOC(HDPR(fb)) && GET_SIZE(HDPR(fb)) >= asize)
            {

#ifdef DEBUG
    fprintf(stderr, "[find_fit] fb=%p, asize = %u, GET_SIZE(HDPR(fb)) = %u, index = %d, i= %d\n", fb, asize, 
        GET_SIZE(HDPR(fb)), index, i);
#endif

                return (void*)fb;
            }
            fb = fb->next;
        }
    }
    return NULL;
#else
    //首次适配策略，从head_listp开始遍历，寻找第一个足够大的空闲块
    for(void* bp = head_listp; GET_SIZE(HDPR(bp)) > 0; bp = NEXT_BLKPR(bp))
    {
        if(!GET_ALLOC(HDPR(bp)) && GET_SIZE(HDPR(bp)) > asize)
            return bp;
    }
    return NULL;
#endif
}

static void place(void* bp, size_t asize)
{
    //csize标记该块的大小，asize代表需要被分配的大小
    size_t csize = GET_SIZE(HDPR(bp)); 

#ifdef DEBUG
    fprintf(stderr, "[place] [enter] bp=%p asize=%u csize=%u\n", bp, (unsigned)asize, (unsigned)csize);
#endif

#ifdef SEGREGATED_FREE_LIST
#ifdef DEBUG
    fprintf(stderr, "[place] [will rm_freeblk] bp=%p\n", bp);
#endif
    rm_freeblk((freeblk_t*)bp);
#endif

    //剩余空间不足以再分配一个块
    if(csize - asize < 4 * DSIZE)
    {
        PUT(HDPR(bp), PACK(csize, 1));
        PUT(FTPR(bp), PACK(csize, 1));

#ifdef DEBUG
    fprintf(stderr, "[place] [space not enough] csize=%u, asize=%u\n", csize, asize);
#endif

    }
    //更改剩余块的头尾指针
    else
    {
        PUT(HDPR(bp), PACK(asize, 1));
        PUT(FTPR(bp), PACK(asize, 1));
        char *next_bp = NEXT_BLKPR(bp);
        PUT(HDPR(next_bp), PACK(csize - asize, 0));
        PUT(FTPR(next_bp), PACK(csize - asize, 0));

#ifdef DEBUG
    fprintf(stderr, "[place] [split: allocated] bp=%p asize=%u next_bp=%p remain=%u\n",
        bp, (unsigned)asize, (void*)next_bp, (unsigned)(csize - asize));
#endif

#ifdef SEGREGATED_FREE_LIST
        ins_freeblk((void*)next_bp);
#endif

    }
}

static int get_freelist_index(size_t size)
{
    int index = 0;
    size_t s = 1;
    while(s < size && index < LIST_NUM-1)
    {
        s <<= 1;
        index++;
    }
    return index;
}

static void ins_freeblk(void* bp)
{

#ifdef DEBUG
    fprintf(stderr, "[ins_freeblk] [enter] bp=%p hdr=0x%x size=%u idx=%d\n", bp, GET(HDPR(bp)), 
    (unsigned)GET_SIZE(HDPR(bp)), get_freelist_index(GET_SIZE(HDPR(bp))));
#endif

    if(!bp) return;
    if(GET_ALLOC(HDPR(bp))) return;
    size_t fsize = GET_SIZE(HDPR(bp));
    int index = get_freelist_index(GET_SIZE(HDPR(bp)));
    freeblk_t *fb = (freeblk_t *)bp;
    fb->prev = NULL;
    fb->next = free_lists[index];
    if (free_lists[index] != NULL) {
        free_lists[index]->prev = fb;
    }
    free_lists[index] = fb;

#ifdef DEBUG
    fprintf(stderr, "[ins_freeblk] [final] index=%d, fb=%p, hdr=0x%x\n", index, fb, GET(HDPR(fb)));
#endif 

}

static void rm_freeblk(freeblk_t *fb)
{
    if(!fb) return;

#ifdef DEBUG
    fprintf(stderr, "[rm_freeblk] [enter] fb=%p hdr=0x%x size=%u idx=%d prev=%p next=%p, hdr=0x%x\n",
        fb, GET(HDPR(fb)), (unsigned)GET_SIZE(HDPR(fb)), get_freelist_index(GET_SIZE(HDPR(fb))),
        fb->prev, fb->next, GET(HDPR(fb)));
#endif

    size_t size = GET_SIZE(HDPR(fb));
    if(size == 0) return;
    int index = get_freelist_index(size);
    if(fb->prev && fb->next)
    {
        fb->prev->next = fb->next;
        fb->next->prev = fb->prev;
    }
    else if(fb->prev && !fb->next)
    {
        fb->prev->next = NULL;
    }
    else if(!fb->prev && fb->next)
    {
        free_lists[index] = fb->next;
        fb->next->prev = NULL;
    }
    else
    {
        free_lists[index] = NULL;
    }
    fb->prev = NULL;
    fb->next = NULL;

#ifdef DEBUG
    fprintf(stderr, "[rm_freeblk] [after-rm] fb=%p, index = %d, free_lists[index]=%p, hdr=0x%x\n", 
        fb, index, free_lists[index], GET(HDPR(fb)));
#endif

}

static void print_freelst()
{
    for (int i = 0; i < LIST_NUM; i++) {
        freeblk_t *f = free_lists[i];
        if (f == NULL) {
            fprintf(stderr, "[print] index=%d, free_lists[%d] = NULL\n", i, i);
        } else {
            fprintf(stderr, "[print] index=%d, free_lists[%d] = %p, GET_SIZE(HDPR(...)) = %u, hdr=0x%x\n",
                    i, i, f, (unsigned)GET_SIZE(HDPR(f)), GET(HDPR(f)));
        }
    }
}
