#include "malloc.h"

void	free(void *ptr)
{
	t_block	*block;
	t_zone	*zone;

	if (!ptr)
		return ;
	pthread_mutex_lock(&g_malloc.mutex);
	block = get_block_from_ptr(ptr);
	if (!block)
	{
		pthread_mutex_unlock(&g_malloc.mutex);
		return ;
	}
	block->free = 1;
	merge_blocks(block);
	zone = get_zone_for_size(block->size);
	if (zone && zone->blocks->free && !zone->blocks->next)
	{
		if (block->size <= TINY_MAX_SIZE)
			g_malloc.tiny = zone->next;
		else if (block->size <= SMALL_MAX_SIZE)
			g_malloc.small = zone->next;
		else
			g_malloc.large = zone->next;
		munmap(zone, zone->size);
	}
	pthread_mutex_unlock(&g_malloc.mutex);
} 