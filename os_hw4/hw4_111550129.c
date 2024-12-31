/*
Student No.: 111550129
Student Name: 林彥亨
Email: yanheng.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/

#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define HEADER_SIZE 32

struct block 
{
    size_t size;
    int free;
    struct block *next;
    struct block *list_next;  // for multilevel free list
};
typedef struct block mem_block;
mem_block *mem_pool = NULL;

mem_block *freeList[12] = {NULL};  // multilevel free list with 11 levels

int get_level(size_t size) 
{
    int level = 0;
    size_t range = 32;  // lv.0->0~31; lv.1->32~63; lv.2->64~127....

    while (size > (range-1) && level < 10) 
    {
        level++;
        range *= 2;  // double every range
    }
    return level;
}


void add_to_list(mem_block *block) 
{
    int level = get_level(block->size);
    block->free = 1;
    block->list_next = NULL;
    mem_block *curr = freeList[level];
    if (curr == NULL) 
    {
        freeList[level] = block;
        return;
    }
    while (curr->list_next != NULL) 
    {
        curr = curr->list_next;
    }
    curr->list_next = block;
}

void remove_from_list(mem_block *block) 
{
    int level = get_level(block->size);
    mem_block *curr = freeList[level];
    mem_block *prev = NULL;
    while (curr != NULL) 
    {
        if (curr == block) 
        {
            if (prev == NULL) 
            {
                freeList[level] = curr->list_next;
            }
            else 
            {
                prev->list_next = curr->list_next;
            }
            break;
        }
        prev = curr;
        curr = curr->list_next;
    }
}

size_t max_chunk_size() 
{
    size_t max_size = 0;
    int level = 10;
    while (freeList[level] == NULL && level > 0) 
    {
        level--;
    }
    
    if (level == 0 && freeList[level] == NULL) 
    {
        return 0;
    }
    
    mem_block *curr = freeList[level];
    
    while (curr != NULL) 
    {
        if (curr->size > max_size) 
        {
            max_size = curr->size;
        }
        curr = curr->list_next;
    }
    return max_size;
}

void *find_split_block(size_t size) 
{
    size = (size + 31) / 32 * 32;  // align to 32 bytes
    int level = get_level(size);
    mem_block *best = NULL;

    // Traverse all levels from the initial level to the highest level
    for (int i = level; i <= 10; i++) 
    {
        mem_block *curr = freeList[i];
        while (curr != NULL) 
        {
            if (curr->size >= size && (best == NULL || curr->size < best->size)) 
            {
                best = curr;
                // Early exit if exact fit is found
                if (curr->size == size) 
                {
                    break;
                }
            }
            curr = curr->list_next;
        }
        // Early exit if exact fit is found
        if (best != NULL && best->size == size) 
        {
            break;
        }
    }

    if (best != NULL) 
    {
        // Remove the best fit block from the free list
        remove_from_list(best);

        // Split the block if it's significantly larger than needed
        if (best->size >= size + sizeof(mem_block) + 32) 
        {  // Adjusted to ensure enough space for the new block
            mem_block *new_block = (mem_block *)((char *)best + sizeof(mem_block) + size);
            new_block->size = best->size - size - sizeof(mem_block);
            new_block->free = 1;
            new_block->next = best->next;
            best->next = new_block;
            add_to_list(new_block);
            best->size = size;
        }

        best->free = 0;
        return (char *)best + sizeof(mem_block);
    }
    return NULL;
}


// malloc function using best fit
void *malloc(size_t size) 
{
    if (mem_pool == NULL) 
    {
        mem_pool = mmap(NULL, 20000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);  // allocate 20000 bytes
        if (mem_pool == MAP_FAILED) 
        {
            mem_pool = NULL;
            return NULL;
        }
        mem_block *block = (mem_block *)mem_pool;
        block->size = 20000 - sizeof(mem_block);
        block->free = 1;
        block->next = NULL;
        add_to_list(block);
    }

    if (size == 0) 
    {
        size_t max_size = max_chunk_size();
        char output[40];
        snprintf(output, sizeof(output), "Max Free Chunk Size = %zu\n", max_size);
        write(1, output, strlen(output));
        munmap(mem_pool, 20000);
        mem_pool = NULL;  // Reset memory after unmapping
        
            // re-initial freeList
        for (int i = 0; i < 12; i++) 
        {
            freeList[i] = NULL;
        }

        return NULL;

    }
    return find_split_block(size);
}

void free(void *ptr) 
{
    if (ptr == NULL || mem_pool == NULL) 
    {
        return;  // Do nothing if ptr is NULL or memory is not initialized
    }

    mem_block *block = (mem_block *)((char *)ptr - sizeof(mem_block));
    block->free = 1;

    // Merge with next block if possible
    if (block->next != NULL && (char *)block->next < (char *)mem_pool + 20000 && block->next->free) 
    {
        remove_from_list(block->next);
        block->size += block->next->size + sizeof(mem_block);
        block->next = block->next->next;
    }


    // Merge with previous block if possible
    if (block != mem_pool) 
    {
        mem_block *curr = mem_pool;
        mem_block *prev = NULL;
        while (curr != NULL && curr != block) 
        {
            prev = curr;
            curr = curr->next;
        }
        
        if (prev != NULL && prev->free) 
        {
            remove_from_list(prev);
            prev->size += block->size + sizeof(mem_block);
            prev->next = block->next;
            block = prev;
        }
    }

    // Add the merged block back to the free list
    add_to_list(block);
}

