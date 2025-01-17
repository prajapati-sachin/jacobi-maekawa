#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;


  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  //copying the signal handler from parent to child
  p->handler = 0;
  (p->pending_signals).head=0;
  (p->pending_signals).tail=0; 
  //not handling a signal
  p->handling =0;
  // for unicast queue 
  (p->recv_queue).head=0;
  (p->recv_queue).tail=0;  

  //for multicast queue
  (p->recv_multi_queue).head=0;
  (p->recv_multi_queue).tail=0;
  // (p->signal_lock).locked=0;
  // initlock(&(p->signal_lock), "multicast");
  
  // initlock(np->queue_lock, "q_lock");


  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  // p->handling = 0;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));
  
  np->handler = curproc->handler; // copying  parent's signal handler
  // initlock(&(np->recv_multi_queue.lock), "multicast");
  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;


  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

void ps(void){
   static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  // int i;
  struct proc *p;
  // char *state;
  // uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state]){
	    cprintf("pid:%d name:%s\n", p->pid, p->name);
      // state = states[p->state];
    }
    
  } 
}

// struct receiver_q msg_queue[64];

int send_mess(int sender_pid, int rec_pid, char* mess){
  // cprintf("sender_pid: %d\n", sender_pid);
  // cprintf("rec_pid: %d\n", rec_pid);
  int success=-1;
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == rec_pid){

    // strncpy(((p->recv_queue).messages)[(p->recv_queue).tail], mess, 8); 
    memmove(((p->recv_queue).messages)[(p->recv_queue).tail], mess, 8); 
    // cprintf("Sent msg: %s\n",  ((p->recv_queue)->messages)[(p->recv_queue)->tail]);
    ((p->recv_queue).sender_id)[(p->recv_queue).tail] = sender_pid; 
    (p->recv_queue).tail = ((p->recv_queue).tail+1)%NUM_MSG;
    // cprintf("New tail: %d\n", (p->recv_queue)->tail);
    wakeup1(p->chan);
    success=1;   
    // cprintf("This is in queu: %s\n", (p->recv_head)->message);
    }
  }
  
  release(&ptable.lock);
  if(success==1) return 0;
  else return -1; 
}

int recv_mess(int rec_pid, char* mess){
  struct proc *p;
  // cprintf("rec_pid IN: %d\n", rec_pid);
  int success=-1;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == rec_pid){
      while(1){
          //Queue is empty. sleep the process
          // cprintf("My head: %d", (p->recv_queue).head);
          // cprintf("My tail: %d", (p->recv_queue).tail);

        if((p->recv_queue).head==(p->recv_queue).tail){
          // cprintf("I am going to sleep\n");
          sleep(&(p->recv_queue), &ptable.lock);
          // sleep(p->chan, p->queue_lock);
        } 
        else{
          // cprintf("My head: %d", (p->recv_queue).head);
          // cprintf("This is in queu: %s\n", ((p->recv_queue).messages)[(p->recv_queue).head]);
          // strncpy(mess, ((p->recv_queue).messages)[(p->recv_queue).head], 8);
          memmove(mess, ((p->recv_queue).messages)[(p->recv_queue).head], 8);
          
          (p->recv_queue).head = (((p->recv_queue).head)+1)%NUM_MSG;
          // p->recv_head = (p->recv_head)->next;
          break;
        }  
      }
      success=1;

    }
  }
  release(&ptable.lock);    
  if(success==1) return 0;
  else return -1;
}

void set_signal(signal_handler s){
  struct proc *p = myproc();
  acquire(&ptable.lock);
  
  p->handler = s; 
  // cprintf("%d", (int)s);
  release(&ptable.lock);    
}


int send_multicast(int sender_pid, int* rec_pids, char* msg, int length){
  int success=-1;
  for(int i=0;i<length;i++){
	  struct proc *p;
	  acquire(&ptable.lock);
	  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	    if(p->pid == rec_pids[i]){
	    // cprintf("rec pis: %d\n", rec_pids[i]);
	    // cprintf("head tail: %d | %d \n", (p->recv_multi_queue).head, (p->recv_multi_queue).tail);

	    //Updating the message queue of the proccess
	    // strncpy(((p->recv_queue).messages)[(p->recv_queue).tail], mess, 8); 
	    memmove(((p->recv_multi_queue).messages)[(p->recv_multi_queue).tail], msg, 8); 
	    // cprintf("Sent msg: %s\n",  ((p->recv_queue)->messages)[(p->recv_queue)->tail]);
	    ((p->recv_multi_queue).sender_id)[(p->recv_multi_queue).tail] = sender_pid; 
	    (p->recv_multi_queue).tail = ((p->recv_multi_queue).tail+1)%NUM_MSG;
	    // cprintf("New tail: %d\n", (p->recv_queue)->tail);
	    
	    //Signalling the proccess by Updating the pending signals
	    // cprintf("To Pid: %d\n", p->pid);
	    send_signal(p->pid, 1);
      success=1;
	    // cprintf("signal head tail: %d | %d \n", p->pending_signals.head, p->pending_signals.tail);
	    // cprintf("Signals: %d\n", p->pending_signals.sig_num[p->pending_signals.head]);
	    // cprintf("Signals: %d\n", p->pending_signals.s_pid[p->pending_signals.head]);
	    
	    // cprintf("Sent msg: %s\n",  ((p->recv_queue)->messages)[(p->recv_queue)->tail]);
	    // ((p->pending_signals).sig_num)[(p->pending_signals).tail] = 1; 
	    // ((p->pending_signals).s_pid)[(p->pending_signals).tail] = sender_pid; 
	    // (p->pending_signals).tail = ((p->pending_signals).tail+1)%MAX_SIG;
	    
	    // cprintf("This is in queu: %s\n", (p->recv_head)->message);
	    }
	  }
	  release(&ptable.lock);
	}
  if(success==1) return 0;
  else return -1;  
}

void send_signal(int to_pid, int signum){
  struct proc *myp = myproc();
  int sender_pid = myp->pid;  
  struct proc *p;
  // acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == to_pid){
      p->pending_signals.sig_num[p->pending_signals.tail] = signum;
      p->pending_signals.s_pid[p->pending_signals.tail] = sender_pid;
      p->pending_signals.tail = (p->pending_signals.tail+1)%MAX_SIG;
      wakeup1(p->chan);
    }     
  } 
  // release(&ptable.lock);    
}

void ret_signal(){
  acquire(&ptable.lock);
  struct proc *p = myproc();
  uint codesize = (uint)&sigret_code_end  - (uint)&sigret_code_start;  
  uint original_trapframe = p->tf->esp;
  original_trapframe+=12;
  original_trapframe+=codesize;
  memmove((void*)p->tf, (void*)original_trapframe, sizeof(struct trapframe));
  p->handling=0;
  release(&ptable.lock);    

}

void pause_signal(){
	struct proc *p = myproc();	
	acquire(&ptable.lock);
	// cprintf("%d %d", p->pending_signals.head, p->pending_signals.tail);  	
	if (p->pending_signals.head == p->pending_signals.tail){
	  // proc->paused = 1;
	  // sched();
	  sleep(&(p->pending_signals), &ptable.lock);
	}
  	release(&ptable.lock);    

}

/*
User stack before going to signal handler

-----------------------------
|                           |
| Old user stack            |
-----------------------------
|                           |
| Original Trapframe        |
-----------------------------
|                           |
| Sigret's Code             |
-----------------------------
| msg[7]                    |
-----------------------------
| msg[6]                    |
-----------------------------
| msg[5]                    |
-----------------------------
| msg[4]                    |
-----------------------------
| msg[3]                    |
-----------------------------
| msg[2]                    |
-----------------------------
| msg[1]                    |
-----------------------------
| msg[0]                    |
-----------------------------
| void *msg                 |
-----------------------------
| return address            |
-----------------------------


*/

void handle_signals(struct trapframe *tf){
  // pushcli();
  struct proc *p = myproc();
  // acquire(&p->signal_lock);
  // acquire(&((p->recv_multi_queue).lock));

  //no signal found to handle
  if (p == 0)
    return;
  if(p->handling==1)
    return;  
  if((tf->cs & 3) != DPL_USER)
    return; // CPU isn't at privilege level 3, hence in user mode
  if(p->pending_signals.head == p->pending_signals.tail)
    return;
  ///////////////preparing user stack for handling signal////////////////////////////

  p->handling = 1;
  uint sp = p->tf->esp;
  //pushing original trapframe
  // cprintf("Initial esp:%d\n", sp);
  // cprintf("Size of trapframe: %d\n", sizeof(struct trapframe));

  sp-= sizeof(struct trapframe);
  memmove((void*)sp, (void*)p->tf, sizeof(struct trapframe));

  //pushing the code of sigret
  uint codesize = (uint)&sigret_code_end  - (uint)&sigret_code_start;
  // cprintf("esp val1:%d\n", sp);
  // cprintf("Code size:%d\n", codesize);
  
  sp -= codesize;
  uint code_pointer = sp;
  memmove((void*)sp, (void*)sigret_code_start, codesize);

  //pushing the message
  // cprintf("esp val2:%d\n", sp);
  // cprintf("mess: %d\n", 8);

  sp-= 8;
  // cprintf("esp val3:%d\n", sp);
  // cprintf("messages: %d\n", 8);

  char* m = p->recv_multi_queue.messages[p->recv_multi_queue.head];
  // cprintf("Sent message:%s\n", m);
  p->recv_multi_queue.head = (p->recv_multi_queue.head+1)%NUM_MSG;
  memmove((void*)sp, (void*)m, 8);

  //pushing the pointer to msg
  *((int*)(sp - 4)) = sp;
  
  //pushing the address of sigret code(which will act as a return address)
  *((int*)(sp - 8)) = code_pointer;

  //final esp of the user stack
  sp -= 8;
  // cprintf("esp val4:%d\n", sp);
  // cprintf("pointer+ code_pointer: %d\n", 8);

  p->tf->esp = sp;
  p->tf->eip = (uint)p->handler;
  // cprintf("esp val4:%d\n", p->tf->esp);


  p->pending_signals.head = (p->pending_signals.head+1)%MAX_SIG; 
  // popcli();
  // release(&ptable.lock);
  // release(&((p->recv_multi_queue).lock));
  // release(&p->signal_lock);

  // release(&(p->recv_multi_queue).lock);

}

