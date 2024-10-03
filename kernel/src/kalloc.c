// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include <asm/x86.h>

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  uint ref_counts[NUM_PHYS_PAGES];  // number of references to each physical page
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Acquire Lock
  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;

  // Decrement # of references
  if (kmem.ref_counts[V2PPN(v)])
    kmem.ref_counts[V2PPN(v)]--;

  // If there are 0 references left, FREE THE PAGES
  if (!kmem.ref_counts[V2PPN(v)]){
    // Fill with junk to catch dangling refs.
    // memset(v, 1, PGSIZE);
    r->next = kmem.freelist;
    kmem.freelist = r;
  }

  // Release Lock
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  char *va = (char *) r;
  // If the next free page is not a NULL ptr, then move down the free-list, add a reference count
  if(va) {
    kmem.freelist = r->next;
    kmem.ref_counts[V2PPN(va)] = 1;
  }
  if(kmem.use_lock)
    release(&kmem.lock);
  return va;
}

void add_reference(uint pa) {
  acquire(&kmem.lock);
  kmem.ref_counts[PPN(pa)]++;
  // cprintf("PA = 0x%x has %d references now\n", pa, kmem.ref_counts[PPN(pa)]);
  release(&kmem.lock);
}

void remove_reference(uint pa) {
  acquire(&kmem.lock);
  kmem.ref_counts[PPN(pa)]--;
  // cprintf("PA = 0x%x has %d references now\n", pa, kmem.ref_counts[PPN(pa)]);
  release(&kmem.lock);
}

uint num_references(uint pa) {
  acquire(&kmem.lock);
  uint count = kmem.ref_counts[PPN(pa)];
  release(&kmem.lock);
  return count;
}