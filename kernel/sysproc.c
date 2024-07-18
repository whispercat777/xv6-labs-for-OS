#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
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
  backtrace();
  argint(0, &n);
  if(n < 0)
    n = 0;
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
uint64
sys_sigalarm(void)
{
  int ticks;
  uint64 handler_va;

  // 从用户态获取系统调用参数：ticks 和 handler_va
  argint(0, &ticks);
  argaddr(1, &handler_va);

  // 获取当前进程的 proc 结构体指针
  struct proc* proc = myproc();

  // 设置报警时间间隔和报警处理函数的虚拟地址
  proc->alarm_interval = ticks;
  proc->handler_va = handler_va;

  // 设置标志表示已经设置了报警处理
  proc->have_return = 1; // true

  // 返回 0 表示成功设置报警
  return 0;
}

uint64
sys_sigreturn(void)
{
  // 获取当前进程的 proc 结构体指针
  struct proc* proc = myproc();

  // 恢复保存的中断帧，以便返回到中断代码之前的状态
  *proc->trapframe = proc->saved_trapframe;

  // 设置标志表示已经从信号处理程序返回
  proc->have_return = 1; // true

  // 返回从中断帧中获取的 a0 寄存器值
  return proc->trapframe->a0;
}

