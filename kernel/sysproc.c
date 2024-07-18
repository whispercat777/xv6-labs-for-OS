#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 va;          // 虚拟地址
  int pagenum;        // 要检查的页面数量
  uint64 abitsaddr;   // 存放访问位掩码的地址
  
  // 获取用户传入的参数：虚拟地址、页面数量、存放掩码的地址
  argaddr(0, &va);          
  argint(1, &pagenum);      
  argaddr(2, &abitsaddr);   

  uint64 maskbits = 0;       // 用于存放页面访问结果的掩码
  struct proc *proc = myproc(); // 获取当前进程的结构体指针

  // 遍历每个页面并检查访问位
  for (int i = 0; i < pagenum; i++) {
    // 获取虚拟地址对应的页表条目 (PTE)
    pte_t *pte = walk(proc->pagetable, va + i * PGSIZE, 0);
    if (pte == 0) {
      // 如果页表条目不存在，触发 panic
      panic("page not exist.");
    }
    // 检查页表条目中的访问位 (PTE_A) 是否被设置
    if (PTE_FLAGS(*pte) & PTE_A) {
      // 如果访问位被设置，在结果掩码的相应位置上设置 1
      maskbits = maskbits | (1L << i);
    }
    // 清除页表条目中的访问位 (PTE_A)
    *pte = *pte & ~PTE_A;
  }

  // 将页面访问结果的掩码拷贝到用户空间指定的地址
  if (copyout(proc->pagetable, abitsaddr, (char *)&maskbits, sizeof(maskbits)) < 0) {
    // 如果拷贝失败，触发 panic
    panic("sys_pgacess copyout error");
  }

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
