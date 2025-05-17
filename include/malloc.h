#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>
# include <sys/mman.h>
# include <pthread.h>

# define TINY_MAX_SIZE 128
# define SMALL_MAX_SIZE 1024

# define TINY_ZONE_SIZE (getpagesize() * 4)
# define SMALL_ZONE_SIZE (getpagesize() * 16)

typedef struct s_block {
    size_t          size;
    int             free;
    struct s_block  *next;
    struct s_block  *prev;
} t_block;

typedef struct s_zone {
    size_t          size;
    struct s_zone   *next;
    struct s_block  *blocks;
} t_zone;

typedef struct s_malloc {
    t_zone          *tiny;
    t_zone          *small;
    t_zone          *large;
    pthread_mutex_t mutex;
} t_malloc;

// Global variables
extern t_malloc g_malloc;

// Core functions
void    *malloc(size_t size);
void    free(void *ptr);
void    *realloc(void *ptr, size_t size);

// Memory management functions
void    *allocate_memory(size_t size);
t_zone  *create_zone(size_t size);
t_block *find_free_block(t_zone *zone, size_t size);
void    split_block(t_block *block, size_t size);
void    merge_blocks(t_block *block);

// Display functions
void    show_alloc_mem(void);

// Utility functions
size_t  align_size(size_t size);
t_zone  *get_zone_for_size(size_t size);
t_block *get_block_from_ptr(void *ptr);

#endif 