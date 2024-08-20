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
    5. explore alignment
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

static mem_chunk *allocated_list;


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
        // insert b/w t and t->next

        // coalescing w/ the free chunk ahead
        if(t->next && (ptr + ptr->size == t->next)) {
            ptr->size += t->next->size;
            ptr->next = t->next->next;
        } else {
            ptr->next = t->next;
        }

        // coalescing w/ the free chunk before
        if(t + t->size == ptr) {
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
    bytes = (bytes + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1); // alignment to the nearest (upper) multiple of 8
    bytes += sizeof(mem_chunk);

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
    mem_chunk *pt = free_list, *t = free_list->next;
    bool found = 0;

    mem_chunk *mem;

    while(t) {
        if(t->size >= size) {
            mem = t;
            if(t->size == size) {
                pt->next = t->next;
                t->next = allocated_list;
                allocated_list = t;
                
            } else {
                // break the big chunk
                mem->next = allocated_list;
                allocated_list = mem;

                t->size -= size;
                t  = (mem_chunk *)((char *)t + size);
                pt->next = t;
            }
            found = 1;
            break;
        }

        pt = t;
        t = t->next;
    }

    if(!found) {
        mem_chunk *p = incr_brk(size);
        if(p == NULL) {
            err("Unable to allocate more memory");
            return NULL;
        }
        return (char *)p + sizeof(mem_chunk);
    }

    return (char *)mem + sizeof(mem_chunk);
}

int main() {
    void *p1 = memalloc(8);
    printf("free list contains: %zu chunks\n", get_free_list());
    printf("usable at: %p\n", p1);
    

    return 0;
}