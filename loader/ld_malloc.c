#include "ld_malloc.h"
#include "ld_malloc_internal.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/random.h>

static pthread_mutex_t ld_malloc_lock = PTHREAD_MUTEX_INITIALIZER;
static bool ld_malloc_initialized = false;

static size_t ld_malloc_header_canary;
static size_t ld_malloc_footer_canary;
static ld_malloc_chunk_header_t * ld_malloc_freelist_head = NULL;

static void ld_malloc_abort(char * msg) {
    LD_MALLOC_ERRPRINT("ld_malloc: ");
    LD_MALLOC_ERRPRINT(msg);
    LD_MALLOC_ERRPRINT("\n");
    exit(-1);
}

static void ld_malloc_init() {
    if (getrandom(&ld_malloc_header_canary, sizeof (size_t), 0) != sizeof (size_t))
        ld_malloc_abort("failed to get randomness for header canary");
    if (getrandom(&ld_malloc_footer_canary, sizeof (size_t), 0) != sizeof (size_t))
        ld_malloc_abort("failed to get randomness for footer canary");

    ld_malloc_freelist_head = NULL;
    ld_malloc_initialized = true;
}

static void ld_malloc_freelist_link(ld_malloc_chunk_header_t * chunk) {
    if (chunk->size & LD_MALLOC_CHUNK_USED)
        ld_malloc_abort("adding allocated chunk to freelist");

    if (ld_malloc_freelist_head != NULL) {
        if (!LD_MALLOC_VERIFY_CANARY(ld_malloc_freelist_head, ld_malloc_header_canary))
            ld_malloc_abort("invalid header canary at start of freelist");

        LD_MALLOC_FREELIST(ld_malloc_freelist_head)->last = chunk;
    }

    LD_MALLOC_FREELIST(chunk)->next = ld_malloc_freelist_head;
    LD_MALLOC_FREELIST(chunk)->last = NULL;
    ld_malloc_freelist_head = chunk;
}

static void ld_malloc_freelist_unlink(ld_malloc_freelist_t * freelist) {
    if (freelist->last == NULL) {
        ld_malloc_freelist_head = freelist->next;
    } else {
        if (!LD_MALLOC_VERIFY_CANARY(freelist->last, ld_malloc_header_canary))
            ld_malloc_abort("invalid header canary in freelist->last");

        LD_MALLOC_FREELIST(freelist->last)->next = freelist->next;
    }
    if (freelist->next != NULL) {
        if (!LD_MALLOC_VERIFY_CANARY(freelist->next, ld_malloc_header_canary))
            ld_malloc_abort("invalid header canary in freelist->next");

        LD_MALLOC_FREELIST(freelist->next)->last = freelist->last;
    }

    freelist->last = NULL;
    freelist->next = NULL;
}

static ld_malloc_chunk_header_t * ld_malloc_chunk_split(ld_malloc_chunk_header_t * first_header, size_t new_size) {
    bool orig_used = first_header->size & LD_MALLOC_CHUNK_USED;
    size_t orig_arena_size = first_header->arena_size;
    size_t rest_size = LD_MALLOC_CHUNK_SIZE(first_header) - LD_MALLOC_SIZE_FOR_USERSIZE(new_size);

    size_t first_size = new_size;
    size_t second_size = rest_size;

    if (orig_used) first_size |= LD_MALLOC_CHUNK_USED;

    if (!LD_MALLOC_VERIFY_CANARY(first_header, ld_malloc_header_canary))
        ld_malloc_abort("invalid header while splitting chunk");

    ld_malloc_chunk_footer_t * footer = LD_MALLOC_CHUNK_TO_FOOTER(first_header);
    if (!LD_MALLOC_VERIFY_CANARY(footer, ld_malloc_footer_canary))
        ld_malloc_abort("invalid footer while splitting chunk");

    first_header->size = first_size;
    first_header->arena_size = orig_arena_size & ~LD_MALLOC_ARENA_END;
    first_header->canary = ld_malloc_header_canary ^ first_size;

    ld_malloc_chunk_footer_t * first_footer = LD_MALLOC_CHUNK_TO_FOOTER(first_header);
    first_footer->size = first_size;
    first_footer->canary = ld_malloc_footer_canary ^ first_size;

    ld_malloc_chunk_header_t * second_header = LD_MALLOC_FOOTER_TO_NEXT(first_footer);
    second_header->size = second_size;
    second_header->arena_size = orig_arena_size & ~LD_MALLOC_ARENA_BEGIN;
    second_header->canary = ld_malloc_header_canary ^ second_size;

    ld_malloc_chunk_footer_t * second_footer = LD_MALLOC_CHUNK_TO_FOOTER(second_header);
    if (footer != second_footer)
        ld_malloc_abort("chunk not split correctly!");

    second_footer->size = second_size;
    second_footer->canary = ld_malloc_footer_canary ^ second_size;

    if (orig_used) {
        return second_header;
    } else {
        ld_malloc_freelist_t * first_freelist = LD_MALLOC_FREELIST(first_header);
        ld_malloc_freelist_t * second_freelist = LD_MALLOC_FREELIST(second_header);

        second_freelist->next = first_freelist->next;
        second_freelist->last = first_freelist->last;
        first_freelist->next = NULL;
        first_freelist->last = NULL;

        if (second_freelist->next != NULL) {
            if (!LD_MALLOC_VERIFY_CANARY(second_freelist->next, ld_malloc_header_canary))
                ld_malloc_abort("invalid header canary in freelist->next");

            LD_MALLOC_FREELIST(second_freelist->next)->last = second_header;
        }
        if (second_freelist->last != NULL) {
            if (!LD_MALLOC_VERIFY_CANARY(second_freelist->last, ld_malloc_header_canary))
                ld_malloc_abort("invalid header canary in freelist->last");

            LD_MALLOC_FREELIST(second_freelist->last)->next = second_header;
        }
        if (ld_malloc_freelist_head == first_header) ld_malloc_freelist_head = second_header;

        return first_header;
    }
}

static ld_malloc_chunk_header_t * ld_malloc_find_freelist(size_t size) {
    ld_malloc_chunk_header_t ** last = &ld_malloc_freelist_head;
    ld_malloc_chunk_header_t * cur = *last;

    while (cur) {
        if (cur->size >= size) {
            if (LD_MALLOC_CAN_SPLIT_CHUNK(cur, size)) {
                cur = ld_malloc_chunk_split(cur, size);
            } else {
                ld_malloc_freelist_unlink(LD_MALLOC_FREELIST(cur));
            }
            break;
        }

        if (!LD_MALLOC_VERIFY_CANARY(cur, ld_malloc_header_canary))
            ld_malloc_abort("invalid header canary in freelist");
        if (!LD_MALLOC_VERIFY_CANARY(LD_MALLOC_CHUNK_TO_FOOTER(cur), ld_malloc_footer_canary))
            ld_malloc_abort("invalid footer canary in freelist");

        last = &LD_MALLOC_FREELIST(cur)->next;
        cur = *last;
    }

    return cur;
}

static void * ld_malloc_unlocked(size_t size) {
    if (!ld_malloc_initialized) ld_malloc_init();
    if (!size) return NULL;

    ld_malloc_chunk_header_t * allocated;
    ld_malloc_chunk_footer_t * footer;

    size = LD_MALLOC_ALIGN_SIZE(size);
    allocated = ld_malloc_find_freelist(size);

    if (allocated) {
        size = allocated->size;
    } else {
        size_t chunk_size = LD_MALLOC_SIZE_FOR_USERSIZE(size);
        size_t mmap_size = (chunk_size + (LD_MALLOC_PAGE_SIZE - 1)) & ~(LD_MALLOC_PAGE_SIZE - 1);

        ld_malloc_chunk_header_t * new_chunk = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_chunk == MAP_FAILED) {
            return NULL;
        }

        size_t new_size = LD_MALLOC_USERSIZE_FOR_SIZE(mmap_size) | LD_MALLOC_CHUNK_USED;
        new_chunk->size = new_size;
        new_chunk->arena_size = mmap_size | LD_MALLOC_ARENA_BEGIN | LD_MALLOC_ARENA_END;
        new_chunk->canary = new_size ^ ld_malloc_header_canary;

        ld_malloc_chunk_footer_t * new_footer = LD_MALLOC_CHUNK_TO_FOOTER(new_chunk);
        new_footer->size = new_size;
        new_footer->canary = new_size ^ ld_malloc_footer_canary;

        allocated = new_chunk;
        if (LD_MALLOC_CAN_SPLIT_CHUNK(new_chunk, size)) {
            new_chunk = ld_malloc_chunk_split(new_chunk, size);
            ld_malloc_freelist_link(new_chunk);
        } else {
            size = new_size;
        }
    }

    size |= LD_MALLOC_CHUNK_USED;
    allocated->size = size;
    allocated->canary = size ^ ld_malloc_header_canary;

    footer = LD_MALLOC_CHUNK_TO_FOOTER(allocated);
    footer->size = size;
    footer->canary = size ^ ld_malloc_footer_canary;

    if (!(allocated->size & LD_MALLOC_CHUNK_USED))
        ld_malloc_abort("returning non-allocated chunk from ld_malloc");

    return LD_MALLOC_CHUNK_TO_USER(allocated);
}

static void ld_free_unlocked(void * ptr) {
    if (!ld_malloc_initialized) ld_malloc_init();
    if (!ptr) return;

    unsigned char arena_flags = 0;
    bool into_freelist = true;
    ld_malloc_chunk_header_t * first_header;
    ld_malloc_chunk_footer_t * last_footer;

    ld_malloc_chunk_header_t * prev_header;
    ld_malloc_chunk_footer_t * prev_footer;

    ld_malloc_chunk_header_t * freed;
    ld_malloc_chunk_footer_t * freed_footer;

    ld_malloc_chunk_header_t * next_header = NULL;
    ld_malloc_chunk_footer_t * next_footer = NULL;

    freed = first_header = LD_MALLOC_USER_TO_CHUNK(ptr);
    if (!LD_MALLOC_VERIFY_CANARY(freed, ld_malloc_header_canary))
        ld_malloc_abort("invalid header canary in freed chunk");

    if (!(freed->size & LD_MALLOC_CHUNK_USED))
        ld_malloc_abort("attempted to free already freed chunk");

    arena_flags |= first_header->arena_size & LD_MALLOC_ARENA_FLAGS;

    freed_footer = last_footer = LD_MALLOC_CHUNK_TO_FOOTER(freed);
    if (!LD_MALLOC_VERIFY_CANARY(freed_footer, ld_malloc_footer_canary))
        ld_malloc_abort("invalid footer canary in freed chunk");

    if (!(arena_flags & LD_MALLOC_ARENA_BEGIN)) {
        prev_footer = LD_MALLOC_PREV_FOOTER(freed);
        if (!LD_MALLOC_VERIFY_CANARY(prev_footer, ld_malloc_footer_canary))
            ld_malloc_abort("invalid footer canary before freed chunk");

        if (!(prev_footer->size & LD_MALLOC_CHUNK_USED)) {
            prev_header = first_header = LD_MALLOC_FOOTER_TO_CHUNK(prev_footer);
            if (!LD_MALLOC_VERIFY_CANARY(prev_header, ld_malloc_header_canary))
                ld_malloc_abort("invalid header canary before freed chunk");

            into_freelist = false;
            first_header = prev_header;
            arena_flags |= first_header->arena_size & LD_MALLOC_ARENA_FLAGS;

            prev_footer->size = 0;
            prev_footer->canary = 0;
            freed->size = 0;
            freed->arena_size = 0;
            freed->canary = 0;
        }
    }

    if (!(arena_flags & LD_MALLOC_ARENA_END)) {
        next_header = LD_MALLOC_FOOTER_TO_NEXT(freed_footer);
        if (!LD_MALLOC_VERIFY_CANARY(next_header, ld_malloc_header_canary))
            ld_malloc_abort("invalid header canary after freed chunk");

        if (!(next_header->size & LD_MALLOC_CHUNK_USED)) {
            next_footer = last_footer = LD_MALLOC_CHUNK_TO_FOOTER(next_header);
            if (!LD_MALLOC_VERIFY_CANARY(next_footer, ld_malloc_footer_canary))
                ld_malloc_abort("invalid footer canary after freed chunk");

            ld_malloc_freelist_unlink(LD_MALLOC_FREELIST(next_header));
            arena_flags |= next_header->arena_size & LD_MALLOC_ARENA_FLAGS;

            next_header->size = 0;
            next_header->arena_size = 0;
            next_header->canary = 0;
            freed_footer->size = 0;
            freed_footer->canary = 0;
        }
    }

    size_t size = ((char*)last_footer) - (char*)LD_MALLOC_CHUNK_TO_USER(first_header);
    first_header->size = size;
    first_header->arena_size |= arena_flags;
    first_header->canary = size ^ ld_malloc_header_canary;
    last_footer->size = size;
    last_footer->canary = size ^ ld_malloc_footer_canary;

    if ((arena_flags & LD_MALLOC_ARENA_BEGIN) && (arena_flags & LD_MALLOC_ARENA_END)) {
        if (!into_freelist) {
            ld_malloc_freelist_unlink(LD_MALLOC_FREELIST(first_header));
        }
        munmap(first_header, LD_MALLOC_ARENA_SIZE(first_header));
    } else if (into_freelist) {
        ld_malloc_freelist_link(first_header);
    }
}

static void * ld_realloc_unlocked(void * ptr, size_t size) {
    if (!ld_malloc_initialized) ld_malloc_init();
    if (!ptr) return ld_malloc_unlocked(size);
    if (!size) return ld_free_unlocked(ptr), NULL;
    if (((ssize_t)size) < 0) return NULL;

    unsigned char arena_flags = 0;
    ld_malloc_chunk_header_t * first_header;
    ld_malloc_chunk_footer_t * last_footer;

    ld_malloc_chunk_header_t * prev_header;
    ld_malloc_chunk_footer_t * prev_footer;

    ld_malloc_chunk_header_t * reallocd;
    ld_malloc_chunk_footer_t * reallocd_footer;

    ld_malloc_chunk_header_t * next_header = NULL;
    ld_malloc_chunk_footer_t * next_footer = NULL;

    first_header = reallocd = LD_MALLOC_USER_TO_CHUNK(ptr);
    if (!LD_MALLOC_VERIFY_CANARY(reallocd, ld_malloc_header_canary))
        ld_malloc_abort("invalid header canary in reallocd chunk");

    if (!(reallocd->size & LD_MALLOC_CHUNK_USED))
        ld_malloc_abort("attempted to realloc already freed chunk");

    arena_flags |= first_header->arena_size & LD_MALLOC_ARENA_FLAGS;

    last_footer = reallocd_footer = LD_MALLOC_CHUNK_TO_FOOTER(reallocd);
    if (!LD_MALLOC_VERIFY_CANARY(reallocd_footer, ld_malloc_footer_canary))
        ld_malloc_abort("invalid footer canary in reallocd chunk");

    if (LD_MALLOC_CHUNK_SIZE(reallocd) >= size) {
        if (LD_MALLOC_CAN_SPLIT_CHUNK(reallocd, size)) {
            ld_malloc_chunk_header_t * split = ld_malloc_chunk_split(reallocd, size);
            ld_malloc_freelist_link(split);
        }

        if (!(reallocd->size & LD_MALLOC_CHUNK_USED))
            ld_malloc_abort("returning non-allocated chunk from ld_realloc");

        return ptr;
    }

    size_t size_doubled = reallocd->size * 2;
    size_t new_size = size_doubled > size ? size_doubled : size;

    if (!(arena_flags & LD_MALLOC_ARENA_END)) {
        next_header = LD_MALLOC_FOOTER_TO_NEXT(reallocd_footer);
        if (!LD_MALLOC_VERIFY_CANARY(next_header, ld_malloc_header_canary))
            ld_malloc_abort("invalid header canary after reallocd chunk");

        if (!(next_header->size & LD_MALLOC_CHUNK_USED)) {
            next_footer = last_footer = LD_MALLOC_CHUNK_TO_FOOTER(next_header);
            if (!LD_MALLOC_VERIFY_CANARY(next_footer, ld_malloc_footer_canary))
                ld_malloc_abort("invalid footer canary after reallocd chunk");

            size_t combined_size = ((char *)last_footer) - ((char *)LD_MALLOC_CHUNK_TO_USER(first_header));
            if (combined_size >= size) {
                ld_malloc_freelist_unlink(LD_MALLOC_FREELIST(next_header));

                next_header->size = 0;
                next_header->canary = 0;
                reallocd_footer->size = 0;
                reallocd_footer->canary = 0;

                combined_size |= LD_MALLOC_CHUNK_USED;

                first_header->size = combined_size;
                first_header->canary = ld_malloc_header_canary ^ combined_size;

                last_footer->size = combined_size;
                last_footer->canary = ld_malloc_footer_canary ^ combined_size;

                if (!(reallocd->size & LD_MALLOC_CHUNK_USED))
                    ld_malloc_abort("returning non-allocated chunk from ld_realloc");

                return ptr;
            }
        }
    }

    if (!(arena_flags & LD_MALLOC_ARENA_BEGIN)) {
        prev_footer = LD_MALLOC_PREV_FOOTER(reallocd);
        if (!LD_MALLOC_VERIFY_CANARY(prev_footer, ld_malloc_footer_canary))
            ld_malloc_abort("invalid footer canary before reallocd chunk");

        if (!(prev_footer->size & LD_MALLOC_CHUNK_USED)) {
            prev_header = first_header = LD_MALLOC_FOOTER_TO_CHUNK(prev_footer);
            if (!LD_MALLOC_VERIFY_CANARY(prev_header, ld_malloc_header_canary))
                ld_malloc_abort("invalid header canary before reallocd chunk");

            size_t combined_size = ((char *)last_footer) - ((char *)LD_MALLOC_CHUNK_TO_USER(first_header));
            if (combined_size >= size) {
                if (last_footer == next_footer) {
                    ld_malloc_freelist_unlink(LD_MALLOC_FREELIST(next_header));

                    next_header->size = 0;
                    next_header->canary = 0;
                    reallocd_footer->size = 0;
                    reallocd_footer->canary = 0;
                }

                ld_malloc_freelist_unlink(LD_MALLOC_FREELIST(prev_header));

                size_t old_size = reallocd->size;

                prev_footer->size = 0;
                prev_footer->canary = 0;
                reallocd->size = 0;
                reallocd->canary = 0;

                combined_size |= LD_MALLOC_CHUNK_USED;

                first_header->size = combined_size;
                first_header->canary = ld_malloc_header_canary ^ combined_size;

                last_footer->size = combined_size;
                last_footer->canary = ld_malloc_footer_canary ^ combined_size;

                void * new_ptr = LD_MALLOC_CHUNK_TO_USER(first_header);
                memmove(new_ptr, ptr, old_size);

                if (!(first_header->size & LD_MALLOC_CHUNK_USED))
                    ld_malloc_abort("returning non-allocated chunk from ld_realloc");

                return new_ptr;
            }
        }
    }

    void * new_chunk = ld_malloc_unlocked(new_size);
    if (new_chunk == NULL) return NULL;

    memcpy(new_chunk, ptr, reallocd->size);
    ld_free_unlocked(ptr);

    if (!(LD_MALLOC_USER_TO_CHUNK(new_chunk)->size & LD_MALLOC_CHUNK_USED))
        ld_malloc_abort("returning non-allocated chunk from ld_realloc");

    return new_chunk;
}

void * ld_malloc(size_t size) {
    pthread_mutex_lock(&ld_malloc_lock);
    void * ret = ld_malloc_unlocked(size);
    pthread_mutex_unlock(&ld_malloc_lock);
    return ret;
}

void ld_free(void * ptr) {
    pthread_mutex_lock(&ld_malloc_lock);
    ld_free_unlocked(ptr);
    pthread_mutex_unlock(&ld_malloc_lock);
}

void * ld_realloc(void * ptr, size_t size) {
    pthread_mutex_lock(&ld_malloc_lock);
    void * ret = ld_realloc_unlocked(ptr, size);
    pthread_mutex_unlock(&ld_malloc_lock);
    return ret;
}

void * ld_calloc(size_t nmemb, size_t size) {
    size_t arr_size;
    if (__builtin_mul_overflow(nmemb, size, &arr_size)) return NULL;

    void * ptr = ld_malloc(arr_size);
    if (ptr != NULL) {
        memset(ptr, 0, arr_size);
    }
    return ptr;
}

__attribute__((visibility("hidden")))
void * malloc(size_t size) {
    return ld_malloc(size);
}

__attribute__((visibility("hidden")))
void free(void * ptr) {
    ld_free(ptr);
}

__attribute__((visibility("hidden")))
void * realloc(void * ptr, size_t size) {
    return ld_realloc(ptr, size);
}

__attribute__((visibility("hidden")))
void * calloc(size_t nmemb, size_t size) {
    return ld_calloc(nmemb, size);
}
