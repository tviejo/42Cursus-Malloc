#include "malloc.h"

void	*realloc(void *ptr, size_t size)
{
	t_block	*block;
	void	*new_ptr;
	size_t	copy_size;

	if (!ptr)
		return (malloc(size));
	if (size == 0)
	{
		free(ptr);
		return (NULL);
	}
	pthread_mutex_lock(&g_malloc.mutex);
	block = get_block_from_ptr(ptr);
	if (!block)
	{
		pthread_mutex_unlock(&g_malloc.mutex);
		return (NULL);
	}
	size = align_size(size);
	if (block->size >= size)
	{
		pthread_mutex_unlock(&g_malloc.mutex);
		return (ptr);
	}
	if (block->next && block->next->free &&
		block->size + sizeof(t_block) + block->next->size >= size)
	{
		merge_blocks(block);
		pthread_mutex_unlock(&g_malloc.mutex);
		return (ptr);
	}
	pthread_mutex_unlock(&g_malloc.mutex);
	new_ptr = malloc(size);
	if (!new_ptr)
		return (NULL);
	copy_size = block->size < size ? block->size : size;
	for (size_t i = 0; i < copy_size; i++)
		((char *)new_ptr)[i] = ((char *)ptr)[i];
	free(ptr);
	return (new_ptr);
} 