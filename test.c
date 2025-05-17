#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <dlfcn.h>

// Test configuration
#define NUM_THREADS 4
#define ALLOCATIONS_PER_THREAD 100
#define MAX_ALLOC_SIZE 1024
#define MIN_ALLOC_SIZE 16
#define MAX_TEST_SIZE (1024 * 1024) // 1MB max for testing
#define ALIGNMENT 16
#define MAX_SAFE_SIZE (SIZE_MAX / 2) // Safe maximum size for testing

// Function pointers for custom malloc implementation
static void *(*custom_malloc)(size_t) = NULL;
static void (*custom_free)(void *) = NULL;
static void *(*custom_realloc)(void *, size_t) = NULL;

// Test statistics
typedef struct {
    size_t total_allocations;
    size_t total_frees;
    size_t total_reallocs;
    size_t failed_allocations;
    double total_time;
    size_t max_allocated;
    size_t current_allocated;
    size_t peak_allocations;
    size_t total_bytes_allocated;
} t_test_stats;

// Thread test data
typedef struct {
    int thread_id;
    void **allocations;
    size_t *sizes;
    t_test_stats stats;
    pthread_mutex_t mutex;
} t_thread_data;

// Global test statistics
t_test_stats g_stats = {0};
pthread_mutex_t g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// Helper function to get random size
size_t get_random_size(void) {
    return (rand() % (MAX_ALLOC_SIZE - MIN_ALLOC_SIZE + 1)) + MIN_ALLOC_SIZE;
}

// Helper function to check alignment
int is_aligned(void *ptr) {
    return ((uintptr_t)ptr % ALIGNMENT) == 0;
}

// Helper function to measure time
double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Helper function to update global statistics
void update_global_stats(t_test_stats *local_stats) {
    pthread_mutex_lock(&g_stats_mutex);
    g_stats.total_allocations += local_stats->total_allocations;
    g_stats.total_frees += local_stats->total_frees;
    g_stats.total_reallocs += local_stats->total_reallocs;
    g_stats.failed_allocations += local_stats->failed_allocations;
    g_stats.total_time += local_stats->total_time;
    if (local_stats->current_allocated > g_stats.max_allocated) {
        g_stats.max_allocated = local_stats->current_allocated;
    }
    if (g_stats.total_allocations > g_stats.peak_allocations) {
        g_stats.peak_allocations = g_stats.total_allocations;
    }
    g_stats.total_bytes_allocated += local_stats->total_bytes_allocated;
    pthread_mutex_unlock(&g_stats_mutex);
}

// Helper function to initialize thread data
void init_thread_data(t_thread_data *data, int thread_id) {
    data->thread_id = thread_id;
    data->allocations = calloc(ALLOCATIONS_PER_THREAD, sizeof(void *));
    data->sizes = calloc(ALLOCATIONS_PER_THREAD, sizeof(size_t));
    memset(&data->stats, 0, sizeof(t_test_stats));
    pthread_mutex_init(&data->mutex, NULL);
}

// Helper function to cleanup thread data
void cleanup_thread_data(t_thread_data *data) {
    if (data->allocations) {
        for (int i = 0; i < ALLOCATIONS_PER_THREAD; i++) {
            if (data->allocations[i]) {
                custom_free(data->allocations[i]);
            }
        }
        free(data->allocations);
    }
    if (data->sizes) {
        free(data->sizes);
    }
    pthread_mutex_destroy(&data->mutex);
}

// Test 1: Basic functionality
void test_basic_functionality(void) {
    printf("\n=== Test 1: Basic Functionality ===\n");
    
    // Test malloc with size 0
    void *ptr1 = custom_malloc(0);
    printf("malloc(0): %p\n", ptr1);
    if (ptr1) custom_free(ptr1);
    
    // Test malloc with reasonable large size
    void *ptr2 = custom_malloc(MAX_TEST_SIZE);
    printf("malloc(%d): %p\n", MAX_TEST_SIZE, ptr2);
    if (ptr2) custom_free(ptr2);
    
    // Test alignment
    void *ptr3 = custom_malloc(1);
    printf("malloc(1) alignment: %p (aligned: %s)\n", ptr3, is_aligned(ptr3) ? "yes" : "no");
    if (ptr3) custom_free(ptr3);
    
    // Test multiple small allocations
    void *ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = custom_malloc(16);
        if (ptrs[i]) {
            printf("malloc(16) #%d: %p (aligned: %s)\n", i, ptrs[i], is_aligned(ptrs[i]) ? "yes" : "no");
        }
    }
    for (int i = 0; i < 100; i++) {
        if (ptrs[i]) custom_free(ptrs[i]);
    }
}

// Test 2: Memory content preservation
void test_memory_content(void) {
    printf("\n=== Test 2: Memory Content Preservation ===\n");
    
    // Test malloc and content
    char *str = custom_malloc(100);
    if (str) {
        strcpy(str, "Hello, World!");
        printf("Original content: %s\n", str);
        custom_free(str);
    }
    
    // Test realloc and content
    char *str2 = custom_malloc(10);
    if (str2) {
        strcpy(str2, "Hello");
        printf("Before realloc: %s\n", str2);
        str2 = custom_realloc(str2, 20);
        if (str2) {
            strcat(str2, " World!");
            printf("After realloc: %s\n", str2);
            custom_free(str2);
        }
    }
    
    // Test memory pattern
    int *arr = custom_malloc(100 * sizeof(int));
    if (arr) {
        for (int i = 0; i < 100; i++) {
            arr[i] = i;
        }
        int *arr2 = custom_realloc(arr, 200 * sizeof(int));
        if (arr2) {
            int success = 1;
            for (int i = 0; i < 100; i++) {
                if (arr2[i] != i) {
                    success = 0;
                    break;
                }
            }
            printf("Memory pattern preserved: %s\n", success ? "yes" : "no");
            custom_free(arr2);
        }
    }
}

// Test 3: Edge cases
void test_edge_cases(void) {
    printf("\n=== Test 3: Edge Cases ===\n");
    
    // Test free(NULL)
    custom_free(NULL);
    printf("free(NULL) completed\n");
    
    // Test realloc(NULL, size)
    void *ptr = custom_realloc(NULL, 100);
    if (ptr) {
        printf("realloc(NULL, 100) succeeded\n");
        custom_free(ptr);
    }
    
    // Test realloc(ptr, 0) - should be equivalent to free
    void *ptr2 = custom_malloc(100);
    if (ptr2) {
        void *ptr3 = custom_realloc(ptr2, 0);
        if (ptr3 == NULL) {
            printf("realloc(ptr, 0) returned NULL as expected\n");
        } else {
            printf("realloc(ptr, 0) returned non-NULL: %p\n", ptr3);
            custom_free(ptr3);
        }
    }
    
    // Test boundary conditions with safe maximum size
    void *ptr4 = custom_malloc(MAX_SAFE_SIZE);
    printf("malloc(MAX_SAFE_SIZE): %p\n", ptr4);
    if (ptr4) custom_free(ptr4);
    
    // Test alignment with different sizes
    for (size_t size = 1; size <= 128; size *= 2) {
        void *ptr5 = custom_malloc(size);
        if (ptr5) {
            printf("malloc(%zu) alignment: %s\n", size, is_aligned(ptr5) ? "yes" : "no");
            custom_free(ptr5);
        }
    }
}

// Test 4: Fragmentation
void test_fragmentation(void) {
    printf("\n=== Test 4: Fragmentation ===\n");
    
    void *ptrs[10] = {NULL};
    for (int i = 0; i < 10; i++) {
        ptrs[i] = custom_malloc(100);
    }
    
    // Free every other block
    for (int i = 0; i < 10; i += 2) {
        custom_free(ptrs[i]);
        ptrs[i] = NULL;
    }
    
    // Try to allocate larger blocks
    void *large = custom_malloc(200);
    printf("Allocation after fragmentation: %p\n", large);
    if (large) custom_free(large);
    
    // Try to allocate blocks of different sizes
    void *blocks[5];
    for (int i = 0; i < 5; i++) {
        blocks[i] = custom_malloc(50 * (i + 1));
        printf("malloc(%d) after fragmentation: %p\n", 50 * (i + 1), blocks[i]);
    }
    
    // Free remaining blocks
    for (int i = 0; i < 10; i++) {
        if (ptrs[i]) custom_free(ptrs[i]);
    }
    for (int i = 0; i < 5; i++) {
        if (blocks[i]) custom_free(blocks[i]);
    }
}

// Test 5: Thread safety
void *thread_safety_test(void *arg) {
    t_thread_data *data = (t_thread_data *)arg;
    double start_time = get_time();
    
    for (int i = 0; i < ALLOCATIONS_PER_THREAD; i++) {
        pthread_mutex_lock(&data->mutex);
        size_t size = get_random_size();
        data->sizes[i] = size;  // Store size for later use
        data->allocations[i] = custom_malloc(size);
        if (data->allocations[i]) {
            memset(data->allocations[i], 0x42, size);
            data->stats.total_allocations++;
            data->stats.current_allocated += size;
            data->stats.total_bytes_allocated += size;
            if (data->stats.current_allocated > data->stats.max_allocated) {
                data->stats.max_allocated = data->stats.current_allocated;
            }
            if (data->stats.total_allocations > data->stats.peak_allocations) {
                data->stats.peak_allocations = data->stats.total_allocations;
            }
        }
        pthread_mutex_unlock(&data->mutex);
        
        usleep(1000); // Add some delay to increase chance of race conditions
        
        pthread_mutex_lock(&data->mutex);
        if (data->allocations[i]) {
            custom_free(data->allocations[i]);
            data->allocations[i] = NULL;
            data->stats.total_frees++;
            data->stats.current_allocated -= data->sizes[i];
        }
        pthread_mutex_unlock(&data->mutex);
    }
    
    data->stats.total_time = get_time() - start_time;
    return NULL;
}

// Test 6: Stress test with mixed operations
void test_stress_mixed(void) {
    printf("\n=== Test 6: Stress Test with Mixed Operations ===\n");
    
    void **ptrs = calloc(1000, sizeof(void *));
    if (!ptrs) return;
    
    double start_time = get_time();
    size_t total_allocated = 0;
    size_t max_allocated = 0;
    size_t peak_allocations = 0;
    size_t current_allocations = 0;
    
    for (int i = 0; i < 1000; i++) {
        int op = rand() % 3;
        switch (op) {
            case 0: // malloc
                ptrs[i] = custom_malloc(rand() % 1000 + 1);
                if (ptrs[i]) {
                    size_t size = rand() % 1000 + 1;
                    total_allocated += size;
                    current_allocations++;
                    if (current_allocations > peak_allocations) {
                        peak_allocations = current_allocations;
                    }
                    if (total_allocated > max_allocated) {
                        max_allocated = total_allocated;
                    }
                }
                break;
            case 1: // realloc
                if (i > 0 && ptrs[i-1]) {
                    size_t old_size = rand() % 1000 + 1;
                    size_t new_size = rand() % 1000 + 1;
                    void *new_ptr = custom_realloc(ptrs[i-1], new_size);
                    if (new_ptr) {
                        ptrs[i-1] = new_ptr;
                        total_allocated = total_allocated - old_size + new_size;
                        if (total_allocated > max_allocated) {
                            max_allocated = total_allocated;
                        }
                    }
                }
                break;
            case 2: // free
                if (i > 0 && ptrs[i-1]) {
                    custom_free(ptrs[i-1]);
                    ptrs[i-1] = NULL;
                    total_allocated -= rand() % 1000 + 1;
                    current_allocations--;
                }
                break;
        }
    }
    
    double end_time = get_time();
    printf("Stress test completed in %.2f seconds\n", end_time - start_time);
    printf("Maximum memory allocated: %zu bytes\n", max_allocated);
    printf("Peak number of allocations: %zu\n", peak_allocations);
    
    // Cleanup
    for (int i = 0; i < 1000; i++) {
        if (ptrs[i]) custom_free(ptrs[i]);
    }
    free(ptrs);
}

// Test 7: Memory leak detection
void test_memory_leaks(void) {
    printf("\n=== Test 7: Memory Leak Detection ===\n");
    
    void *leak = custom_malloc(100);
    printf("Intentionally leaked memory: %p\n", leak);
    // Don't free to test leak detection
}

// Test 8: Large allocations
void test_large_allocations(void) {
    printf("\n=== Test 8: Large Allocations ===\n");
    
    size_t sizes[] = {1024, 4096, 8192, 16384, 32768, 65536};
    for (int i = 0; i < 6; i++) {
        void *ptr = custom_malloc(sizes[i]);
        printf("malloc(%zu): %p (aligned: %s)\n", sizes[i], ptr, is_aligned(ptr) ? "yes" : "no");
        if (ptr) {
            // Test memory access
            memset(ptr, 0x42, sizes[i]);
            printf("Memory access test passed for size %zu\n", sizes[i]);
            custom_free(ptr);
        }
    }
    
    // Test multiple large allocations
    void *large_ptrs[10];
    for (int i = 0; i < 10; i++) {
        large_ptrs[i] = custom_malloc(32768);
        if (large_ptrs[i]) {
            printf("Large allocation #%d: %p\n", i, large_ptrs[i]);
        }
    }
    
    for (int i = 0; i < 10; i++) {
        if (large_ptrs[i]) custom_free(large_ptrs[i]);
    }
}

// Test 9: Realloc edge cases
void test_realloc_edge_cases(void) {
    printf("\n=== Test 9: Realloc Edge Cases ===\n");
    
    // Test realloc with same size
    void *ptr = custom_malloc(100);
    if (ptr) {
        void *ptr2 = custom_realloc(ptr, 100);
        printf("realloc same size: %p (aligned: %s)\n", ptr2, is_aligned(ptr2) ? "yes" : "no");
        if (ptr2) custom_free(ptr2);
    }
    
    // Test realloc with smaller size
    void *ptr3 = custom_malloc(100);
    if (ptr3) {
        void *ptr4 = custom_realloc(ptr3, 50);
        printf("realloc smaller size: %p (aligned: %s)\n", ptr4, is_aligned(ptr4) ? "yes" : "no");
        if (ptr4) custom_free(ptr4);
    }
    
    // Test realloc with larger size
    void *ptr5 = custom_malloc(50);
    if (ptr5) {
        void *ptr6 = custom_realloc(ptr5, 100);
        printf("realloc larger size: %p (aligned: %s)\n", ptr6, is_aligned(ptr6) ? "yes" : "no");
        if (ptr6) custom_free(ptr6);
    }
    
    // Test realloc with very large size
    void *ptr7 = custom_malloc(100);
    if (ptr7) {
        void *ptr8 = custom_realloc(ptr7, MAX_TEST_SIZE);
        printf("realloc to very large size: %p\n", ptr8);
        if (ptr8) custom_free(ptr8);
    }
}

// Test 10: Thread safety with multiple operations
void test_thread_safety_complex(void) {
    printf("\n=== Test 10: Thread Safety with Multiple Operations ===\n");
    
    pthread_t threads[NUM_THREADS];
    t_thread_data thread_data[NUM_THREADS];
    
    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        init_thread_data(&thread_data[i], i);
    }
    
    double start_time = get_time();
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, thread_safety_test, &thread_data[i]) != 0) {
            printf("Error creating thread %d\n", i);
            continue;
        }
    }
    
    // Wait for threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double end_time = get_time();
    
    // Print statistics
    printf("Thread safety test completed in %.2f seconds\n", end_time - start_time);
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Thread %d:\n", i);
        printf("  Allocations: %zu\n", thread_data[i].stats.total_allocations);
        printf("  Frees: %zu\n", thread_data[i].stats.total_frees);
        printf("  Max allocated: %zu bytes\n", thread_data[i].stats.max_allocated);
        printf("  Total bytes allocated: %zu\n", thread_data[i].stats.total_bytes_allocated);
        printf("  Peak allocations: %zu\n", thread_data[i].stats.peak_allocations);
        printf("  Time: %.2f seconds\n", thread_data[i].stats.total_time);
    }
    
    // Cleanup thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        cleanup_thread_data(&thread_data[i]);
    }
}

// Comparison test function
void run_comparison_test(void) {
    printf("\n=== Comparison Test: Custom Malloc vs System Malloc ===\n");
    
    // Function pointers for system implementation
    void *(*system_malloc)(size_t) = malloc;
    void *(*system_free)(void *) = free;
    void *(*system_realloc)(void *, size_t) = realloc;
    
    // Test cases
    const int num_tests = 5;
    const int num_operations = 1000;
    const size_t sizes[] = {16, 64, 256, 1024, 4096};
    
    printf("Running %d operations for each size...\n", num_operations);
    printf("Size\tSystem Time\tCustom Time\tSystem Peak\tCustom Peak\n");
    
    for (int i = 0; i < num_tests; i++) {
        size_t size = sizes[i];
        double system_time = 0;
        double custom_time = 0;
        size_t system_peak = 0;
        size_t custom_peak = 0;
        
        // System malloc test
        {
            void **ptrs = calloc(num_operations, sizeof(void *));
            double start = get_time();
            size_t current_allocated = 0;
            
            for (int j = 0; j < num_operations; j++) {
                ptrs[j] = system_malloc(size);
                if (ptrs[j]) {
                    current_allocated += size;
                    if (current_allocated > system_peak) {
                        system_peak = current_allocated;
                    }
                }
            }
            
            for (int j = 0; j < num_operations; j++) {
                if (ptrs[j]) {
                    system_free(ptrs[j]);
                    current_allocated -= size;
                }
            }
            
            system_time = get_time() - start;
            free(ptrs);
        }
        
        // Custom malloc test
        {
            void **ptrs = calloc(num_operations, sizeof(void *));
            double start = get_time();
            size_t current_allocated = 0;
            
            for (int j = 0; j < num_operations; j++) {
                ptrs[j] = custom_malloc(size);
                if (ptrs[j]) {
                    current_allocated += size;
                    if (current_allocated > custom_peak) {
                        custom_peak = current_allocated;
                    }
                }
            }
            
            for (int j = 0; j < num_operations; j++) {
                if (ptrs[j]) {
                    custom_free(ptrs[j]);
                    current_allocated -= size;
                }
            }
            
            custom_time = get_time() - start;
            free(ptrs);
        }
        
        printf("%zu\t%.6f\t%.6f\t%zu\t%zu\n", 
               size, system_time, custom_time, system_peak, custom_peak);
    }
}

int main(void) {
    srand(time(NULL));
    printf("Starting comprehensive malloc test suite...\n");
    
    // Load custom malloc implementation
    void *handle = dlopen("libft_malloc.so", RTLD_NOW);
    if (!handle) {
        printf("Error: Could not load libft_malloc.so: %s\n", dlerror());
        return 1;
    }
    
    // Get function pointers
    custom_malloc = dlsym(handle, "malloc");
    custom_free = dlsym(handle, "free");
    custom_realloc = dlsym(handle, "realloc");
    
    if (!custom_malloc || !custom_free || !custom_realloc) {
        printf("Error: Could not find required symbols: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    
    printf("Custom malloc address: %p\n", (void*)custom_malloc);
    printf("Custom free address: %p\n", (void*)custom_free);
    printf("Custom realloc address: %p\n", (void*)custom_realloc);
    
    // Run the comparison test first
    run_comparison_test();
    
    // Run the rest of the test suite
    test_basic_functionality();
    test_memory_content();
    test_edge_cases();
    test_fragmentation();
    test_stress_mixed();
    test_large_allocations();
    test_realloc_edge_cases();
    test_thread_safety_complex();
    
    dlclose(handle);
    
    printf("\nTest suite completed.\n");
    return 0;
} 