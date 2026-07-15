/* Recyclable allocator using an intrusive free-list with boundary-tag coalescing.
 * Designed for a single memory pool. Keeps low fragmentation via splitting and
 * coalescing adjacent free blocks. Metadata stored in headers and footers.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lava_mem.h"

#ifdef LAVA_ESP32

#include "esp_heap_caps.h"

int g_mem_offset = 0;

void *PAL_MEM_ALLOC(size_t size)
{
    void *result = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (result == NULL)
        result = heap_caps_malloc(size, MALLOC_CAP_8BIT);
    if (result != NULL)
        g_mem_offset += (int)size;
    return result;
}

void *PAL_MEM_CALLOC(size_t count, size_t elem_size)
{
    size_t size = count * elem_size;
    void *result = PAL_MEM_ALLOC(size);
    if (result != NULL)
        memset(result, 0, size);
    return result;
}

void *PAL_MEM_REALLOC(void *old_ptr, size_t new_size)
{
    void *result = heap_caps_realloc(old_ptr, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (result == NULL && new_size != 0)
        result = heap_caps_realloc(old_ptr, new_size, MALLOC_CAP_8BIT);
    return result;
}

char *PAL_MEM_STRDUP(char *str)
{
    size_t size;
    char *copy;
    if (str == NULL) return NULL;
    size = strlen(str) + 1;
    copy = PAL_MEM_ALLOC(size);
    if (copy != NULL) memcpy(copy, str, size);
    return copy;
}

void PAL_MEM_FREE(void *ptr)
{
    heap_caps_free(ptr);
}

void PAL_MEM_RESET(void)
{
    g_mem_offset = 0;
}

int PAL_MEM_USED(void)
{
    return g_mem_offset;
}

#else

unsigned char g_mem_pool[MEM_POOL_SIZE];
int g_mem_offset = 0; /* used for statistics only; allocator manages free list */

/* Block layout:
 * [block_header][user data ...][block_footer]
 * header: size (including header+footer), flags (LSB = allocated)
 * footer: duplicate of header (size & flags), to allow coalescing
 */

#define hdr_t uint32_t

#define ALIGN 4
#define HDR_SIZE (int)sizeof(hdr_t)
#define FREE_LINK_SIZE (int)sizeof(void *)
#define MIN_PAYLOAD_SIZE ((FREE_LINK_SIZE > ALIGN) ? FREE_LINK_SIZE : ALIGN)
#define MIN_BLOCK_SIZE (HDR_SIZE * 2 + MIN_PAYLOAD_SIZE) /* header + footer + payload for free-list link */

/* header flags */
#define FLAG_ALLOC 1u

/* helper to align up */
static inline int align_up(int sz) { return (sz + (ALIGN - 1)) & ~(ALIGN - 1); }

/* read/write header at pointer (points to header location) */
static inline hdr_t read_hdr(void *p) { hdr_t v; memcpy(&v, p, sizeof(v)); return v; }
static inline void write_hdr(void *p, hdr_t v) { memcpy(p, &v, sizeof(v)); }
static inline void *read_free_next(void *p) { void *v = NULL; memcpy(&v, p, sizeof(v)); return v; }
static inline void write_free_next(void *p, void *next) { memcpy(p, &next, sizeof(next)); }

/* get size (including hdr+footer) and alloc status */
static inline int hdr_size(hdr_t h) { return (int)(h & ~((hdr_t)FLAG_ALLOC)); }
static inline int hdr_alloc(hdr_t h) { return (int)(h & FLAG_ALLOC); }

/* pointer arithmetic helpers */
static inline void *offset_ptr(void *p, int off) { return (void *)((unsigned char *)p + off); }

/* free list: singly-linked through payload of free blocks */
static void *free_list = NULL; /* points to payload area of a free block */

/* Initialize the pool: create a single large free block covering the whole pool */
static int pool_inited = 0;
static void pool_init_once(void)
{
    if (pool_inited) return;
    /* initialize pool to single free block */
    int total = MEM_POOL_SIZE;
    hdr_t initial_hdr = (hdr_t)(total & ~((hdr_t)FLAG_ALLOC));
    /* set header at start */
    write_hdr(g_mem_pool, initial_hdr);
    /* footer at end */
    write_hdr(offset_ptr(g_mem_pool, total - HDR_SIZE), initial_hdr);
    /* free list payload pointer (just after header) */
    free_list = offset_ptr(g_mem_pool, HDR_SIZE);
    write_free_next(free_list, NULL);
    pool_inited = 1;
}

/* Operation log to help locate the last allocations/frees when corruption is detected */
#define OPS_LOG_SZ 256
struct op_entry { char *op; void *ptr; int size; void *ret; };
static struct op_entry ops_log[OPS_LOG_SZ];
static int ops_log_idx = 0;

static void log_op(char *op, void *ptr, int size, void *ret)
{
    ops_log[ops_log_idx].op = op;
    ops_log[ops_log_idx].ptr = ptr;
    ops_log[ops_log_idx].size = size;
    ops_log[ops_log_idx].ret = ret;
    ops_log_idx = (ops_log_idx + 1) & (OPS_LOG_SZ - 1);
}

static void dump_ops_log(void)
{
    fprintf(stderr, "--- allocator op log (last %d) ---\n", OPS_LOG_SZ);
    int i = ops_log_idx;
    int cnt;
    for (cnt = 0; cnt < OPS_LOG_SZ; ++cnt)
    {
        i = (i - 1) & (OPS_LOG_SZ - 1);
        if (!ops_log[i].op) continue;
        fprintf(stderr, "%3d: %s ptr=%p size=%d ret=%p\n", cnt, ops_log[i].op, ops_log[i].ptr, ops_log[i].size, ops_log[i].ret);
    }
}

static void hexdump_region(void *addr)
{
    unsigned char *p = (unsigned char *)addr;
    unsigned char *start;
    unsigned char *end;
    unsigned char *q;
    if (p < g_mem_pool || p >= g_mem_pool + MEM_POOL_SIZE)
    {
        fprintf(stderr, "--- hexdump skipped for non-pool address %p ---\n", addr);
        return;
    }
    start = p >= (g_mem_pool + 32) ? p - 32 : g_mem_pool;
    end = p + 32;
    if (start < g_mem_pool) start = g_mem_pool;
    if (end > g_mem_pool + MEM_POOL_SIZE) end = g_mem_pool + MEM_POOL_SIZE;
    fprintf(stderr, "--- hexdump around %p (pool base %p) ---\n", addr, (void*)g_mem_pool);
    for (q = start; q < end; q += 16)
    {
        unsigned char *r;
        fprintf(stderr, "%08x: ", (unsigned int)(q - g_mem_pool));
        for (r = q; r < q + 16 && r < end; ++r) fprintf(stderr, "%02x ", *r);
        fprintf(stderr, "\n");
    }
}

/* Validate that a payload pointer looks like it points inside the pool */
static int is_valid_payload(void *payload)
{
    unsigned char *p = (unsigned char *)payload;
    if (p < g_mem_pool + HDR_SIZE) return 0;
    if (p > g_mem_pool + MEM_POOL_SIZE - HDR_SIZE) return 0;
    return 1;
}

/* insert a free block into free_list (LIFO) */
static void insert_free_block(void *payload)
{
    /* store next pointer at payload start */
    if (!is_valid_payload(payload)) return;
    write_free_next(payload, free_list);
    free_list = payload;
}

/* remove a specific free block from the free list (linear search). Returns 1 if removed. */
static int remove_free_block(void *payload)
{
    void *prev_payload = NULL;
    void *cur_payload = free_list;
    while (cur_payload)
    {
        if (!is_valid_payload(cur_payload))
        {
            /* corruption detected: reset free list to a single block */
            free_list = NULL;
            pool_inited = 0;
            pool_init_once();
            return 0;
        }
        if (cur_payload == payload)
        {
            void *next = read_free_next(payload);
            if (next && !is_valid_payload(next)) next = NULL;
            if (prev_payload)
                write_free_next(prev_payload, next);
            else
                free_list = next;
            return 1;
        }
        prev_payload = cur_payload;
        cur_payload = read_free_next(cur_payload);
    }
    return 0;
}

/* Try to find a free block with at least req_size (including header/footer). First-fit. */
static void *find_fit(size_t req_size)
{
    void *prev_payload = NULL;
    void *payload = free_list;
    while (payload)
    {
        if (!is_valid_payload(payload))
        {
            /* corruption detected -> reinit pool */
            free_list = NULL;
            pool_inited = 0;
            pool_init_once();
            return NULL;
        }
        hdr_t h = read_hdr(offset_ptr(payload, -HDR_SIZE));
        int blk_size = hdr_size(h);
        if ((size_t)blk_size >= req_size)
        {
            /* remove from list */
            void *next = read_free_next(payload);
            if (next && !is_valid_payload(next)) next = NULL;
            if (prev_payload)
                write_free_next(prev_payload, next);
            else
                free_list = next;
            return payload;
        }
        prev_payload = payload;
        payload = read_free_next(payload);
    }
    return NULL;
}

/* split a free block if it's larger than needed; leaves block (payload) returned as allocated payload pointer */
static void split_and_mark_alloc(void *payload, size_t req_size)
{
    void *hdr_ptr = offset_ptr(payload, -HDR_SIZE);
    hdr_t h = read_hdr(hdr_ptr);
    int blk_size = hdr_size(h);
    if ((size_t)blk_size < req_size)
    {
        /* Should not happen: mark whole block allocated to avoid corruption */
        hdr_t alloc_hdr = (hdr_t)(blk_size | FLAG_ALLOC);
        write_hdr(hdr_ptr, alloc_hdr);
        write_hdr(offset_ptr(hdr_ptr, blk_size - HDR_SIZE), alloc_hdr);
        return;
    }

    size_t remaining = (size_t)blk_size - req_size;
    if (remaining >= (size_t)MIN_BLOCK_SIZE)
    {
        /* shrink current into allocated block of req_size, create new free block after it */
        hdr_t alloc_hdr = (hdr_t)((hdr_t)req_size | FLAG_ALLOC);
        write_hdr(hdr_ptr, alloc_hdr);
        write_hdr(offset_ptr(hdr_ptr, (int)req_size - HDR_SIZE), alloc_hdr); /* footer */

        /* new free block starts immediately after the allocated block */
        void *new_block_hdr = offset_ptr(hdr_ptr, (int)req_size);
        hdr_t free_hdr = (hdr_t)(remaining & ~((hdr_t)FLAG_ALLOC));
        write_hdr(new_block_hdr, free_hdr);
        write_hdr(offset_ptr(new_block_hdr, (int)remaining - HDR_SIZE), free_hdr);
        /* insert new payload into free list */
        insert_free_block(offset_ptr(new_block_hdr, HDR_SIZE));
    }
    else
    {
        /* can't split, mark whole block allocated */
        hdr_t alloc_hdr = (hdr_t)(blk_size | FLAG_ALLOC);
        write_hdr(hdr_ptr, alloc_hdr);
        write_hdr(offset_ptr(hdr_ptr, blk_size - HDR_SIZE), alloc_hdr);
    }
}

void *PAL_MEM_ALLOC(size_t size)
{
    if ((int)size <= 0) return NULL;
    pool_init_once();
    log_op("alloc_request", NULL, size, __builtin_return_address(0));
    size_t user_sz = (size_t)align_up((int)size);
    size_t req_size = user_sz + HDR_SIZE * 2; /* header + footer + user */
    if (req_size < MIN_BLOCK_SIZE) req_size = MIN_BLOCK_SIZE;

    void *payload = find_fit(req_size);
    if (!payload)
    {
        /* no fit */
        fprintf(stderr, "MEM FAIL: need %zu\n", req_size);
        return NULL;
    }
    split_and_mark_alloc(payload, req_size);
    g_mem_offset += (int)req_size; /* statistic (approx) */
    log_op("alloc", payload, (int)req_size, __builtin_return_address(0));
    return payload;
}

void *PAL_MEM_CALLOC(size_t count, size_t elem_size)
{
    size_t total = (size_t)count * (size_t)elem_size;
    log_op("calloc_request", NULL, (int)total, __builtin_return_address(0));
    void *p = PAL_MEM_ALLOC(total);
    if (p) memset(p, 0, total);
    log_op("calloc", p, (int)total, __builtin_return_address(0));
    return p;
}

void PAL_MEM_FREE(void *p)
{
    if (!p) return;
    log_op("free", p, 0, __builtin_return_address(0));
    /* mark block as free and coalesce with neighbors */
    void *hdr_ptr = offset_ptr(p, -HDR_SIZE);
    /* validate hdr_ptr within pool */
    if (!is_valid_payload(p) || (unsigned char *)hdr_ptr < g_mem_pool || (unsigned char *)hdr_ptr >= g_mem_pool + MEM_POOL_SIZE)
    {
        fprintf(stderr, "PAL_MEM_FREE: invalid pointer %p (outside pool)\n", p);
        dump_ops_log();
        hexdump_region(p);
        /* defensive: reset pool to avoid repeated crashes */
        PAL_MEM_RESET();
        return;
    }
    hdr_t h = read_hdr(hdr_ptr);
    int blk_size = hdr_size(h);
    if (!hdr_alloc(h))
    {
        fprintf(stderr, "PAL_MEM_FREE: double free or corrupted alloc bit at %p: hdr=0x%08x\n", hdr_ptr, (unsigned)h);
        dump_ops_log();
        hexdump_region(hdr_ptr);
        PAL_MEM_RESET();
        return;
    }
    /* validate size */
    if (blk_size < MIN_BLOCK_SIZE || blk_size > MEM_POOL_SIZE || (unsigned char *)hdr_ptr + blk_size > g_mem_pool + MEM_POOL_SIZE)
    {
        fprintf(stderr, "PAL_MEM_FREE: corrupted header at %p: hdr=0x%08x blk_size=%d\n", hdr_ptr, (unsigned)h, blk_size);
        dump_ops_log();
        hexdump_region(hdr_ptr);
        PAL_MEM_RESET();
        return;
    }
    /* mark free */
    hdr_t free_hdr = (hdr_t)(blk_size & ~((hdr_t)FLAG_ALLOC));
    write_hdr(hdr_ptr, free_hdr);
    write_hdr(offset_ptr(hdr_ptr, blk_size - HDR_SIZE), free_hdr);

    /* try coalescing with previous block if exists */
    /* previous footer is at hdr_ptr - HDR_SIZE, get its header */
    if ((unsigned char *)hdr_ptr > g_mem_pool + HDR_SIZE)
    {
        hdr_t prev_footer = read_hdr(offset_ptr(hdr_ptr, -HDR_SIZE));
        int prev_size = hdr_size(prev_footer);
        if (!hdr_alloc(prev_footer))
        {
            /* previous is free; remove it from free list and merge */
            void *prev_hdr = offset_ptr(hdr_ptr, -prev_size);
            void *prev_payload = offset_ptr(prev_hdr, HDR_SIZE);
            remove_free_block(prev_payload);
            hdr_ptr = prev_hdr;
            blk_size += prev_size;
        }
    }

    /* try coalescing with next block if exists */
    void *next_hdr = offset_ptr(hdr_ptr, blk_size);
    if ((unsigned char *)next_hdr < g_mem_pool + MEM_POOL_SIZE - HDR_SIZE)
    {
        hdr_t next_h = read_hdr(next_hdr);
        if (!hdr_alloc(next_h))
        {
            int next_size = hdr_size(next_h);
            void *next_payload = offset_ptr(next_hdr, HDR_SIZE);
            remove_free_block(next_payload);
            blk_size += next_size;
        }
    }

    /* write merged free hdr/footer */
    hdr_t merged = (hdr_t)(blk_size & ~((hdr_t)FLAG_ALLOC));
    write_hdr(hdr_ptr, merged);
    write_hdr(offset_ptr(hdr_ptr, blk_size - HDR_SIZE), merged);
    insert_free_block(offset_ptr(hdr_ptr, HDR_SIZE));
}

void *PAL_MEM_REALLOC(void *old_ptr, size_t new_size)
{
    if (!old_ptr) return PAL_MEM_ALLOC(new_size);
    log_op("realloc_request", old_ptr, new_size, __builtin_return_address(0));
    if ((int)new_size <= 0) { PAL_MEM_FREE(old_ptr); return NULL; }

    void *hdr_ptr = offset_ptr(old_ptr, -HDR_SIZE);
    hdr_t h = read_hdr(hdr_ptr);
    int blk_size = hdr_size(h);
    int user_sz = blk_size - HDR_SIZE * 2;
    if (new_size <= (size_t)user_sz)
    {
        /* shrink in place (no split for simplicity) */
        return old_ptr;
    }

    /* try to expand into next free block */
    void *next_hdr = offset_ptr(hdr_ptr, blk_size);
    if ((unsigned char *)next_hdr < g_mem_pool + MEM_POOL_SIZE - HDR_SIZE)
    {
        hdr_t next_h = read_hdr(next_hdr);
        if (!hdr_alloc(next_h))
        {
            int combined = blk_size + hdr_size(next_h);
            int needed = align_up((int)new_size) + HDR_SIZE * 2;
            if (combined >= needed)
            {
                /* remove next from free list */
                remove_free_block(offset_ptr(next_hdr, HDR_SIZE));
                /* update header/footer to combined (allocated) */
                hdr_t alloc_hdr = (hdr_t)(combined | FLAG_ALLOC);
                write_hdr(hdr_ptr, alloc_hdr);
                write_hdr(offset_ptr(hdr_ptr, combined - HDR_SIZE), alloc_hdr);
                return old_ptr;
            }
        }
    }

    /* fallback: allocate new and copy */
    void *newp = PAL_MEM_ALLOC(new_size);
    if (!newp) return NULL;
    int to_copy = user_sz < (int)new_size ? user_sz : (int)new_size;
    memcpy(newp, old_ptr, to_copy);
    PAL_MEM_FREE(old_ptr);
    log_op("realloc", newp, new_size, __builtin_return_address(0));
    return newp;
}

char *PAL_MEM_STRDUP(char *str)
{
    if (!str) return NULL;
    int len = (int)strlen(str);
    char *p = (char *)PAL_MEM_ALLOC(len + 1);
    if (p) memcpy(p, str, len + 1);
    return p;
}

void PAL_MEM_RESET(void)
{
    /* reinitialize pool to single free block */
    memset(g_mem_pool, 0, MEM_POOL_SIZE);
    free_list = NULL;
    pool_inited = 0;
    pool_init_once();
    g_mem_offset = 0;
}

int PAL_MEM_USED(void)
{
    /* approximate: sum of allocated sizes by scanning pool */
    int used = 0;
    unsigned char *p = g_mem_pool;
    unsigned char *end = g_mem_pool + MEM_POOL_SIZE;
    while (p + HDR_SIZE <= end)
    {
        hdr_t h = read_hdr(p);
        if (h == 0) break;
        int sz = hdr_size(h);
        if (hdr_alloc(h)) used += sz;
        p += sz;
    }
    return used;
}

#endif
