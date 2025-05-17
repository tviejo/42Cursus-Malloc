#include "malloc.h"
#include <sys/resource.h>

t_malloc	g_malloc = {NULL, NULL, NULL, PTHREAD_MUTEX_INITIALIZER};

size_t	align_size(size_t size)
{
	return ((size + 15) & ~15);
}

t_zone	*create_zone(size_t size)
{
	t_zone	*zone;
	size_t	zone_size;

	if (size <= TINY_MAX_SIZE)
		zone_size = TINY_ZONE_SIZE;
	else if (size <= SMALL_MAX_SIZE)
		zone_size = SMALL_ZONE_SIZE;
	else
		zone_size = size + sizeof(t_zone) + sizeof(t_block);
	zone = mmap(NULL, zone_size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (zone == MAP_FAILED)
		return (NULL);
	zone->size = zone_size;
	zone->next = NULL;
	zone->blocks = (t_block *)((char *)zone + sizeof(t_zone));
	zone->blocks->size = zone_size - sizeof(t_zone) - sizeof(t_block);
	zone->blocks->free = 1;
	zone->blocks->next = NULL;
	zone->blocks->prev = NULL;
	return (zone);
}

t_block	*find_free_block(t_zone *zone, size_t size)
{
	t_block	*block;

	while (zone)
	{
		block = zone->blocks;
		while (block)
		{
			if (block->free && block->size >= size)
				return (block);
			block = block->next;
		}
		zone = zone->next;
	}
	return (NULL);
}

void	split_block(t_block *block, size_t size)
{
	t_block	*new_block;
	size_t	remaining_size;

	if (block->size <= size + sizeof(t_block) + 16)
		return ;
	remaining_size = block->size - size - sizeof(t_block);
	new_block = (t_block *)((char *)block + sizeof(t_block) + size);
	new_block->size = remaining_size;
	new_block->free = 1;
	new_block->next = block->next;
	new_block->prev = block;
	if (block->next)
		block->next->prev = new_block;
	block->next = new_block;
	block->size = size;
}

void	merge_blocks(t_block *block)
{
	if (block->next && block->next->free)
	{
		block->size += block->next->size + sizeof(t_block);
		block->next = block->next->next;
		if (block->next)
			block->next->prev = block;
	}
}

t_zone	*get_zone_for_size(size_t size)
{
	if (size <= TINY_MAX_SIZE)
		return (g_malloc.tiny);
	else if (size <= SMALL_MAX_SIZE)
		return (g_malloc.small);
	return (g_malloc.large);
}

t_block	*get_block_from_ptr(void *ptr)
{
	t_zone	*zone;
	t_block	*block;

	zone = g_malloc.tiny;
	while (zone)
	{
		block = zone->blocks;
		while (block)
		{
			if ((char *)block + sizeof(t_block) == ptr)
				return (block);
			block = block->next;
		}
		zone = zone->next;
	}
	zone = g_malloc.small;
	while (zone)
	{
		block = zone->blocks;
		while (block)
		{
			if ((char *)block + sizeof(t_block) == ptr)
				return (block);
			block = block->next;
		}
		zone = zone->next;
	}
	zone = g_malloc.large;
	while (zone)
	{
		block = zone->blocks;
		if ((char *)block + sizeof(t_block) == ptr)
			return (block);
		zone = zone->next;
	}
	return (NULL);
} 