#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

/*
    Req:
    1. alloc mem as user asks for it, also implement a free mem functionality
    2. handle fragmentation - maintain a list of currently allocated and free chunks - chunk metadata moves b/w these 2
    3. merge/colesce consectutive (wrt addr) free chunks to avoid fragmentation 
    4. should handle 0 size mem alloc req, that can be later passed to free(void *ptr)
*/ 

void err(const char *message) {
    perror(message);
}


// embedded list
typedef struct mem_chunk {
    // bool               is_alloc; 
    size_t             size;
    struct mem_chunk   *next;
} mem_chunk;

static mem_chunk sink; // 0 size sink for the free list
// init the free_list (on Data Segment)
static mem_chunk *free_list = &sink;


static mem_chunk sink2;
static mem_chunk *allocated_list;

size_t align(size_t bytes) {
    return ((bytes + sizeof(mem_chunk) - 1) & ~(sizeof(mem_chunk) - 1));
}

// ordered
// insert: O(N), merge: O(1)
void insert_on_free_list(mem_chunk *ptr) {
    mem_chunk *t = free_list;
    bool found = 0;

    while(t) {    
        if(ptr > t) {
            if(t->next == NULL || (t->next && t->next > ptr)) {
                found = 1;
                break;
            }
        }
        t = t->next;
    }

    if(found) {
        // coalescing w/ the free chunk ahead
        if(t->next && (ptr + ptr->size + sizeof(mem_chunk) == t->next)) {
            ptr->size += t->next->size;
            ptr->next = t->next->next;
        } else {
            ptr->next = t->next;
        }

        // coalescing w/ the free chunk before
        if(t + t->size + sizeof(mem_chunk) == ptr) {
            t->size += ptr->size;
            t->next = ptr->next;
        } else {
            t->next = ptr;
        }
    }
}

// tested: OK insertion based on order
// TODO: test merge by fragmenting a chunk and then adding it back to free_list
void test_insert_on_free_list() {}

size_t get_free_list(void) {
    size_t s = -1;
    mem_chunk *t = free_list;
    while(t) {
        s++;
        printf("ptr: %p, size: %zu, next: %p\n", t, t->size, t->next);
        t = t->next;
    }
    return s;
}

// calls sbrk
mem_chunk *incr_brk(size_t bytes) {
    bytes = align(bytes);
    bytes += sizeof(mem_chunk);

    printf("sbrk: %zu\n", bytes);
    void *ret;
    if((ret = sbrk(bytes)) == (void *)-1) {
        return NULL;
    }

    mem_chunk *cast_ret = (mem_chunk *)ret;
    cast_ret->size = bytes - sizeof(mem_chunk);

    insert_on_free_list(cast_ret);
    return cast_ret;
} 

void *memalloc(size_t size) {
    size = align(size);
    printf("looking for %zu\n", size);

    mem_chunk *pt = free_list, *t = free_list->next;
    bool found = 0;

    mem_chunk *mem = NULL;

    while(t) {
        if(t->size >= size) {
            printf("found: %p with size %zu\n", t, t->size);
            mem = t;
            if(t->size == size) {
                pt->next = t->next;
                // mem->next = allocated_list;
                // allocated_list = mem;
                
            } else {
                // break the big chunk
                // mem->next = allocated_list;
                // allocated_list = mem;

                mem_chunk *new_chunk = (mem_chunk *)((char *)t + sizeof(mem_chunk) + size); // t -> |metadata|available mem|
                new_chunk->size = t->size - size;
                pt->next = new_chunk;
                new_chunk->next = t->next;
            }

            return (char *)mem + sizeof(mem_chunk);
        }

        pt = t;
        t = t->next;
    }

    mem_chunk *p = incr_brk(size);
    if(p == NULL) {
        err("Unable to allocate more memory");
        return NULL;
    }
    return (char *)p + sizeof(mem_chunk);
    
}

int main() {
    // void *p = memalloc(10);
    // void *p1 = memalloc(30);
    // void *p2 = memalloc(35);
    void *p3 = memalloc(50);
    void *p4 = memalloc(70);
    printf("free list contains: %zu chunks\n", get_free_list());
    void *p = memalloc(32);
    printf("\n\n");
    printf("free list contains: %zu chunks\n", get_free_list());

    return 0;
}