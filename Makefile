# Malloc Project Makefile

# Project configuration
NAME = libft_malloc_x86_64.so
LINK_NAME = libft_malloc.so

# Directories
SRC_DIR = src/
OBJ_DIR = obj/
LIBFT_DIR = libft/

# Source files
SRC_FILES = malloc.c free.c realloc.c show_alloc_mem.c memory_management.c
SRC = $(addprefix $(SRC_DIR), $(SRC_FILES))
OBJ = $(SRC:$(SRC_DIR)%.c=$(OBJ_DIR)%.o)
D_FILES = $(SRC:$(SRC_DIR)%.c=$(OBJ_DIR)%.d)

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -fPIC
INCLUDES = -I./include -I$(LIBFT_DIR)/includes

# Colors for output
GREEN = \033[0;32m
RED = \033[0;31m
BLUE = \033[0;34m
END = \033[0m
BOLD_START = \033[1m
BOLD_END = \033[0m

# Debug flags
ifeq ($(debug),true)
	CFLAGS += -g -fsanitize=address
endif

# Default target
all: $(NAME)

# Check if libft exists and initialize if needed
check_libft:
	@if [ ! -d "$(LIBFT_DIR)" ]; then \
		echo "$(RED)libft directory not found. Initializing...$(END)"; \
		git submodule update --init --recursive; \
	fi

# Build libft
$(LIBFT_DIR)/libft.a: check_libft
	@echo "$(GREEN)Building libft...$(END)"
	$(MAKE) -C $(LIBFT_DIR)

# Create object files
$(OBJ_DIR)%.o: $(SRC_DIR)%.c
	@echo "$(BLUE)Compiling: $< -> $@ $(END)"
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build the shared library
$(NAME): $(LIBFT_DIR)/libft.a $(OBJ)
	@echo "$(GREEN)Linking $(NAME)...$(END)"
	$(CC) $(CFLAGS) -shared -o $(NAME) $(OBJ) $(LIBFT_DIR)/libft.a
	@ln -sf $(NAME) $(LINK_NAME)
	@echo "$(GREEN)$(BOLD_START)Build complete!$(BOLD_END)$(END)"

# Clean object files
clean:
	@echo "$(RED)Cleaning objects...$(END)"
	$(RM) -r $(OBJ_DIR)
	$(RM) $(D_FILES)
	@echo "$(RED)Cleaning libft...$(END)"
	$(MAKE) -C $(LIBFT_DIR) clean
	@echo "$(GREEN)$(BOLD_START)Clean done$(BOLD_END)$(END)"

# Clean everything
fclean: clean
	@echo "$(RED)Removing $(NAME)...$(END)"
	$(RM) $(NAME) $(LINK_NAME)
	@echo "$(RED)Cleaning libft...$(END)"
	$(MAKE) -C $(LIBFT_DIR) fclean
	@echo "$(GREEN)$(BOLD_START)Fclean done$(BOLD_END)$(END)"

# Rebuild everything
re: fclean all

# Include dependency files
-include $(D_FILES)

.PHONY: all clean fclean re check_libft