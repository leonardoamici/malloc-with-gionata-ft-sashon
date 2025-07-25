#include "../includes/libft_malloc_ubuntu.h"

t_heap g_heap;

t_chunk *last_chunk(t_page *page)
{
    t_chunk *temp = page->head;

    while (temp->next)
    {
        temp = temp->next;
    }
    return temp;
}

void *split_chunks(t_page *page, size_t allocation)
{
    if (allocation == 0)
        allocation = 1;

    allocation = align16(allocation);

    t_chunk *temp = page->head;
    t_chunk *best = NULL;

    while (temp)
    {
        if (temp->available && temp->size >= allocation + sizeof(t_chunk))
        {
            if (!best || temp->size < best->size) {
                best = temp;
            }
        }
        temp = temp->next;
    }

    if (!best)
        return NULL;

    size_t remaining = best->size - allocation;
    if (remaining < sizeof(t_chunk) + 16)
    {
        best->available = 0;
        best->freed = 0;
        page->available -= best->size;
        return best->head;
    }

    t_chunk *new_chunk = (void *)((char *)best->head + allocation);
    new_chunk->head = (char *)new_chunk + sizeof(t_chunk);
    new_chunk->size = remaining - sizeof(t_chunk);
    new_chunk->next = best->next;
    new_chunk->available = 1;
    new_chunk->type = best->type;

    best->available = 0;
    best->freed = 0;
    best->size = allocation;
    best->next = new_chunk;

    page->available -= allocation + sizeof(t_chunk);

    return best->head;
}

void *big_allocation(size_t allocation_size)
{
    long page_size = sysconf(_SC_PAGESIZE);
    int nbr_pages = (allocation_size + sizeof(t_chunk) + page_size - 1) / page_size;

    void *mem = mmap(NULL, page_size * nbr_pages, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
        return NULL;

    t_chunk *new_chunk = (t_chunk *)mem;
    new_chunk->head = (void *)((char *)mem + sizeof(t_chunk));
    new_chunk->size = page_size * nbr_pages - sizeof(t_chunk);
    new_chunk->next = NULL;
    new_chunk->freed = 0;
    new_chunk->available = 0;
    new_chunk->type = LARGE;


    if (!g_heap.large) {
        g_heap.large = new_chunk;
    } else {
        t_chunk *current = g_heap.large;
        while (current->next)
            current = current->next;
        current->next = new_chunk;
    }
    return new_chunk->head;
}

void *sort_allocations(t_heap *heap, size_t size)
{
    if (size > (size_t)SMALL_ALLOC)
        return (big_allocation(size));
    else if (size > (size_t)TINY_ALLOC)
        return (split_chunks(&heap->small, size));
    return (split_chunks(&heap->tiny, size));
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr;

    ptr = malloc(nmemb * size);
    if (ptr == NULL)
        return (NULL);
    for (size_t i = 0; i < nmemb * size; i++)
    {
        ((char *)ptr)[i] = 0;
    }
    return (ptr);
}

void *malloc(size_t size)
{
    void *new_alloc;

    pthread_mutex_lock(&g_heap.mutex);

    new_alloc = sort_allocations(&g_heap, size);

    //ft_printf("malloc called with size ");
    //print_size_t(size);
    //ft_printf(" and returned ptr = %p \n", new_alloc);

    //ft_printf_fd(2, "malloc(%d): %p\n", size, new_alloc);

    pthread_mutex_unlock(&g_heap.mutex);

    //show_alloc_mem();

    return (new_alloc);
}