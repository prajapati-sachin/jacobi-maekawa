#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

// extern int toggle;

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_add(int a, int b){
	argint(0, &a);
	argint(1, &b);
	return a+b;
}

int sys_ps(void){
  ps();
  return 0;
}

int sys_toggle(void){
  if(toggle==1){
	//clear the syscall count array
  	for(int i=0;i< NELEM(syscall_count);i++) syscall_count[i]=0;
    toggle=0;
  }
  else if(toggle==0){
	//clear the syscall count array
  	for(int i=0;i< NELEM(syscall_count);i++) syscall_count[i]=0;
  	//switch the toggle
    toggle=1;
  }
  return 0;
}

//names of system call
char* name_syscalls[] = {"sys_fork", "sys_exit", "sys_wait", "sys_pipe", "sys_read", 
  "sys_kill", "sys_exec", "sys_fstat", "sys_chdir", "sys_dup", 
  "sys_getpid", "sys_sbrk", "sys_sleep", "sys_uptime", "sys_open", 
  "sys_write", "sys_mknod", "sys_unlink", "sys_link", "sys_mkdir", 
  "sys_close", "sys_add", "sys_ps", "sys_toggle", "sys_print_count"} ;


int sys_print_count(void){
  //print the non-zero counts of system calls
  for(int i=0;i< NELEM(syscall_count);i++){
  	if(syscall_count[i]>0) cprintf("%s %d\n", name_syscalls[i], syscall_count[i]);
  }
  return 0;
}