#include "malloc.h"
#include "../libft/includes/libft.h"

static size_t	print_zone(t_zone *zone, const char *name)
{
	size_t	total;
	t_block	*block;
	char	*size_str;

	if (!zone)
		return (0);
	total = 0;
	ft_putstr_fd_2((char *)name, 1);
	ft_putstr_fd_2((char *)" : ", 1);
	ft_putstr_fd_2((char *)"0x", 1);
	ft_puthexalow((unsigned long)zone, 1);
	ft_putchar_fd('\n', 1);
	block = zone->blocks;
	while (block)
	{
		if (!block->free)
		{
			ft_putstr_fd_2((char *)"0x", 1);
			ft_puthexalow((unsigned long)((char *)block + sizeof(t_block)), 1);
			ft_putstr_fd_2((char *)" - ", 1);
			ft_putstr_fd_2((char *)"0x", 1);
			ft_puthexalow((unsigned long)((char *)block + sizeof(t_block) + block->size), 1);
			ft_putstr_fd_2((char *)" : ", 1);
			size_str = ft_itoa(block->size);
			ft_putstr_fd_2(size_str, 1);
			free(size_str);
			ft_putstr_fd_2((char *)" bytes\n", 1);
			total += block->size;
		}
		block = block->next;
	}
	return (total);
}

void	show_alloc_mem(void)
{
	size_t	total;
	char	*total_str;

	total = 0;
	pthread_mutex_lock(&g_malloc.mutex);
	total += print_zone(g_malloc.tiny, "TINY");
	total += print_zone(g_malloc.small, "SMALL");
	total += print_zone(g_malloc.large, "LARGE");
	ft_putstr_fd_2((char *)"Total : ", 1);
	total_str = ft_itoa(total);
	ft_putstr_fd_2(total_str, 1);
	free(total_str);
	ft_putstr_fd_2((char *)" bytes\n", 1);
	pthread_mutex_unlock(&g_malloc.mutex);
} 