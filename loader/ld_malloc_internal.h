#pragma once

#include <stddef.h>
#include <string.h>
#include <unistd.h>

typedef struct _ld_malloc_chunk_header {
    size_t size;
    size_t arena_size;
    size_t _pad;
    size_t canary;
    // char user_data[X] or ld_malloc_freelist_t * freelist
} ld_malloc_chunk_header_t;

typedef struct _ld_malloc_freelist {
    ld_malloc_chunk_header_t * next;
    ld_malloc_chunk_header_t * last;
} ld_malloc_freelist_t;

typedef struct _malloc_chunk_footer {
    size_t canary;
    size_t size;
} ld_malloc_chunk_footer_t;

#define LD_MALLOC_CHUNK_USED 1
#define LD_MALLOC_ARENA_BEGIN 1
#define LD_MALLOC_ARENA_END 2
#define LD_MALLOC_ARENA_FLAGS (LD_MALLOC_ARENA_BEGIN | LD_MALLOC_ARENA_END)

#define LD_MALLOC_PAGE_SIZE 0x1000

/// print the parameter on stderr
///
/// @param s a char * to a string to be printed
#define LD_MALLOC_ERRPRINT(s) \
    write(STDERR_FILENO, s, strlen(s))

/// get the size of a chunk
///
/// @param c an ld_malloc_chunk_header_t * or ld_malloc_chunk_footer_t *
/// @returns the size of the chunk without the USED flag
#define LD_MALLOC_CHUNK_SIZE(c) \
    ((c)->size & ~LD_MALLOC_CHUNK_USED)

/// get the size of an arena
///
/// @param c an ld_malloc_chunk_header_t * with a set LD_MALLOC_ARENA_BEGIN flag
/// @returns the size of the arena
#define LD_MALLOC_ARENA_SIZE(c) \
    ((c)->arena_size & ~LD_MALLOC_ARENA_FLAGS)

/// check if an ld_malloc chunk is large enough to be split into multiple chunks
///
/// @param c an ld_malloc_chunk_header_t * or ld_malloc_chunk_footer_t * to be split
/// @param sz the new size of the chunk to be resized
/// @returns whether the chunk can be split
#define LD_MALLOC_CAN_SPLIT_CHUNK(c, sz) \
    (LD_MALLOC_CHUNK_SIZE(c) > (sz) && (LD_MALLOC_CHUNK_SIZE(c) - (sz)) >= LD_MALLOC_SIZE_FOR_USERSIZE(1))

/// get the size needed for metadata for a user allocation of size
///
/// @param size a size the user wants to allocate
/// @returns the size needed for ld_malloc metadata and chunk structures
#define LD_MALLOC_SIZE_FOR_USERSIZE(size) \
    (LD_MALLOC_ALIGN_SIZE((size_t)(size)) + sizeof (ld_malloc_chunk_header_t) + sizeof (ld_malloc_chunk_footer_t))

/// subtract the size needed for metadata for an allocation of size
///
/// @param size a size of a malloc chunk
/// @returns the size available to the user in this chunk
#define LD_MALLOC_USERSIZE_FOR_SIZE(size) \
    ((size_t)(size) - sizeof (ld_malloc_chunk_header_t) - sizeof (ld_malloc_chunk_footer_t))

/// get the pointer to the freelist entries of this chunk
///
/// @param c an ld_malloc_chunk_header_t * that is part of the freelist
/// @returns an ld_malloc_freelist_t * pointing to the next and last chunks in the list
#define LD_MALLOC_FREELIST(c) \
    ((ld_malloc_freelist_t *)LD_MALLOC_CHUNK_TO_USER(c))

/// verify whether the canary of the header of footer is correct
///
/// @param c is an ld_malloc_chunk_header_t * or an ld_malloc_chunk_footer_t *
/// @param correct is the correct canary
/// @returns true if the canary is correct, false otherwise
#define LD_MALLOC_VERIFY_CANARY(c, correct) \
    (((c)->size ^ (c)->canary) == (correct))

/// convert an ld_malloc chunk pointer to a user pointer
///
/// @param c an ld_malloc_chunk_header_t *
/// @returns a void * to the user area of the ld_malloc chunk
#define LD_MALLOC_CHUNK_TO_USER(c) \
    ((void *)(((char *)(c)) + sizeof (ld_malloc_chunk_header_t)))

/// convert a user pointer to the corresponding ld_malloc chunk
///
/// @param c a void * to the user area of the ld_malloc chunk
/// @returns an ld_malloc_chunk_header_t *
#define LD_MALLOC_USER_TO_CHUNK(c) \
    ((ld_malloc_chunk_header_t *)(((char *)(c)) - sizeof (ld_malloc_chunk_header_t)))

/// get the footer of the previous ld_malloc chunk
///
/// @param c an ld_malloc_chunk_header_t *
/// @returns an ld_malloc_chunk_footer_t *
#define LD_MALLOC_PREV_FOOTER(c) \
    ((ld_malloc_chunk_footer_t *)(((char *)(c)) - sizeof (ld_malloc_chunk_footer_t)))

/// get the footer of an ld_malloc chunk
///
/// @param c an ld_malloc_chunk_header_t *
/// @returns an ld_malloc_chunk_footer_t *
#define LD_MALLOC_CHUNK_TO_FOOTER(c) \
    ((ld_malloc_chunk_footer_t *)(((char *)LD_MALLOC_CHUNK_TO_USER(c)) + LD_MALLOC_CHUNK_SIZE(c)))

/// get the header corresponding to an ld_malloc chunk footer
///
/// @param c an ld_malloc_chunk_footer_t *
/// @returns an ld_malloc_chunk_header_t *
#define LD_MALLOC_FOOTER_TO_CHUNK(c) \
    LD_MALLOC_USER_TO_CHUNK(((char *)(c)) - LD_MALLOC_CHUNK_SIZE(c))

/// get the next ld_malloc chunk in memory from a chunk footer
///
/// @param c an ld_malloc_chunk_header_t *
/// @returns the next ld_malloc_chunk_header_t *
#define LD_MALLOC_FOOTER_TO_NEXT(c) \
    ((ld_malloc_chunk_header_t *)(((char *)(c)) + sizeof (ld_malloc_chunk_footer_t)))

/// get the next ld_malloc chunk in memory
///
/// @param c an ld_malloc_chunk_header_t *
/// @returns the next ld_malloc_chunk_header_t *
#define LD_MALLOC_NEXT_CHUNK(c) \
    LD_MALLOC_FOOTER_TO_NEXT(LD_MALLOC_CHUNK_TO_FOOTER(c))

/// round up number to fit ld_malloc_freelist_t when the chunk is free
/// aligns size to 4 * sizeof (size_t)
/// assumes sizeof (size_t) is a power of 2
///
/// @param n a number
/// @returns n rounded up to fit an ld_malloc_freelist_t
#define LD_MALLOC_ALIGN_SIZE(n) \
    (((n) + (sizeof (ld_malloc_freelist_t) - 1)) & ~(sizeof (ld_malloc_freelist_t) - 1))
