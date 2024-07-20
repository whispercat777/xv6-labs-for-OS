// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 13  // 可以根据需要选择适当的桶数


struct {
  struct spinlock lock;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
  struct buf hash[NBUCKET][NBUF];
  struct spinlock hashlock[NBUCKET];
} bcache;

void
binit(void)
{
    struct buf* b;

    initlock(&bcache.lock, "bcache");
    //初始化每个bucket的锁和缓冲区链表
    for (int i = 0; i < NBUCKET; i++) {
        initlock(&bcache.hashlock[i], "bcache");

        // Create linked list of buffers
        bcache.head[i].prev = &bcache.head[i];
        bcache.head[i].next = &bcache.head[i];
        for (b = bcache.hash[i]; b < bcache.hash[i] + NBUF; b++) {
            b->next = bcache.head[i].next;
            b->prev = &bcache.head[i];
            initsleeplock(&b->lock, "buffer");
            bcache.head[i].next->prev = b;
            bcache.head[i].next = b;
        }
    }
}
// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
    struct buf* b;
    //计算哈希码并锁定桶
    uint hashcode = blockno % NBUCKET;
    acquire(&bcache.hashlock[hashcode]);

    // Is the block already cached?
    for (b = bcache.head[hashcode].next; b != &bcache.head[hashcode]; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&bcache.hashlock[hashcode]);//释放桶的自旋锁
            acquiresleep(&b->lock);//取缓冲区的自旋锁
            return b;
        }
    }

    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    for (b = bcache.head[hashcode].prev; b != &bcache.head[hashcode]; b = b->prev) {
        if (b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            release(&bcache.hashlock[hashcode]);
            acquiresleep(&b->lock);
            return b;
        }
    }
    release(&bcache.hashlock[hashcode]);
    panic("bget: no buffers");
}


// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf* b)
{
    if (!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);

    uint hashcode = b->blockno % NBUCKET;//计算缓冲区所在的桶
    acquire(&bcache.hashlock[hashcode]);//获取该桶的自旋锁
    b->refcnt--;
    if (b->refcnt == 0) {
        // no one is waiting for it.
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head[hashcode].next;
        b->prev = &bcache.head[hashcode];
        bcache.head[hashcode].next->prev = b;
        bcache.head[hashcode].next = b;
    }

    release(&bcache.hashlock[hashcode]);//释放桶的自旋锁
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


