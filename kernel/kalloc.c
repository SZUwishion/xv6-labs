// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.


struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  int counts[PHYSTOP / PGSIZE];
  struct spinlock lock;
} points;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&points.lock, "points");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    points.counts[(uint64)p / PGSIZE] = 1;
    kfree(p);
  }
    
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&points.lock);
  points.counts[(uint64)pa / PGSIZE]--;
  // printf("point_count[pa]: %d\n", points.counts[(uint64)pa / PGSIZE]);
  if(points.counts[(uint64)pa / PGSIZE] == 0) {
    release(&points.lock);
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  } else {
    release(&points.lock);
  }
  
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&points.lock);
    points.counts[(uint64)r / PGSIZE] = 1;
    release(&points.lock);
  }
    
  return (void*)r;
}

uint64* cowalloc(uint64 va, pagetable_t pagetable) {
  if(va % PGSIZE != 0 || va >= MAXVA) {
    return 0;
  }

  uint64 pa_old = walkaddr(pagetable, va);
  if(pa_old == 0) {
    return 0;
  }

  pte_t* pte_old = walk(pagetable, va, 0);
  if(pte_old == 0) {
    return 0;
  }
  if((*pte_old & PTE_V) == 0 || (*pte_old & SSTATUS_SPP) == 0) {
    return 0;
  }
  
  if (get_point_count(pa_old) == 1) {
    *pte_old = *pte_old & (~SSTATUS_SPP);
    *pte_old = *pte_old | PTE_W;
    return (uint64*)pa_old;
  } else {
    
    if(get_point_count(pa_old) == 1) {
      *pte_old = *pte_old & (~SSTATUS_SPP);
      *pte_old = *pte_old | PTE_W;
    }
    uint64* pa_new = kalloc();
    if(pa_new == 0) {
      return 0;
    }
    memmove(pa_new, (char*)pa_old, PGSIZE);
    *pte_old = *pte_old & ~PTE_V;
    va = PGROUNDDOWN(va);
    uint flags = PTE_FLAGS(*pte_old) & (~SSTATUS_SPP);
    
    if(mappages(pagetable, va, PGSIZE, (uint64)pa_new, flags | PTE_W) != 0) {
      kfree((void*)pa_new);
      *pte_old = *pte_old | PTE_V;
      return 0;
    }
    kfree((void*)PGROUNDDOWN(pa_old));
    return pa_new;
  }
}

void add_point_count(uint64 pa) {
  acquire(&points.lock);
  if((char*)pa >= end && pa < PHYSTOP) {
    
    points.counts[pa / PGSIZE]++;
  }
  release(&points.lock);
}

void sub_point_count(uint64 pa) {
  acquire(&points.lock);
  points.counts[pa / PGSIZE]--;
  release(&points.lock);
}

int get_point_count(uint64 pa) {
  return points.counts[pa / PGSIZE];
}