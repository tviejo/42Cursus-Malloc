#include "malloc.h"

void	*malloc(size_t size)
{
	t_zone	*zone;
	t_block	*block;
	void	*ptr;

	if (size == 0)
		return (NULL);
	pthread_mutex_lock(&g_malloc.mutex);
	size = align_size(size);
	zone = get_zone_for_size(size);
	block = find_free_block(zone, size);
	if (block)
	{
		split_block(block, size);
		block->free = 0;
		ptr = (void *)((char *)block + sizeof(t_block));
	}
	else
	{
		zone = create_zone(size);
		if (!zone)
		{
			pthread_mutex_unlock(&g_malloc.mutex);
			return (NULL);
		}
		if (size <= TINY_MAX_SIZE)
		{
			zone->next = g_malloc.tiny;
			g_malloc.tiny = zone;
		}
		else if (size <= SMALL_MAX_SIZE)
		{
			zone->next = g_malloc.small;
			g_malloc.small = zone;
		}
		else
		{
			zone->next = g_malloc.large;
			g_malloc.large = zone;
		}
		block = zone->blocks;
		split_block(block, size);
		block->free = 0;
		ptr = (void *)((char *)block + sizeof(t_block));
	}
	pthread_mutex_unlock(&g_malloc.mutex);
	return (ptr);
} 