#ifndef __DEBUG_sysproc 
#undef DEBUG 
#endif 

#define __module_name__ 	"sysproc"

#include "types.h"
#include "hal/riscv.h"
#include "param.h"
#include "memlayout.h"
#include "sync/spinlock.h"
#include "sched/proc.h"
#include "syscall.h"
#include "timer.h"
#include "mm/pm.h"
#include "mm/vm.h"
#include "mm/usrmm.h"
#include "printf.h"
#include "utils/string.h"
#include "utils/debug.h"
#include "resource.h"
#include "errno.h"

extern int execve(char *path, char **argv, char **envp);

uint64
sys_exec(void)
{
	char path[MAXPATH];
	uint64 argv;

	if(argstr(0, path, MAXPATH) < 0 || argaddr(1, &argv) < 0){
		return -1;
	}

	return execve(path, (char **)argv, 0);
}

uint64
sys_execve(void)
{
	char path[MAXPATH];
	uint64 argv, envp;

	if(argstr(0, path, MAXPATH) < 0 || argaddr(1, &argv) < 0 || argaddr(2, &envp)){
		return -1;
	}

	return execve(path, (char **)argv, (char **)envp);
}

uint64
sys_exit(void)
{
	int n;
	if(argint(0, &n) < 0)
		return -1;
	// since exit never return, we print the trace-info here
	if (myproc()->tmask/* & (1 << (SYS_exit - 1))*/) {
		printf(")\n");
	}
	exit(n);
	return 0;  // not reached
}

uint64
sys_getpid(void)
{
	return myproc()->pid;
}

uint64 
sys_getppid(void) {
	struct proc *p = myproc();
	int pid;

	__debug_assert("sys_getppid", NULL != p->parent, 
			"NULL == p->parent\n");
	pid = p->parent->pid;

	return pid;
}

uint64
sys_fork(void)
{
	extern int fork_cow();
	return fork_cow();
	// return fork();
}

uint64 
sys_clone(void) {
	uint64 flag, stack;
	if (argaddr(0, &flag) < 0) 
		return -1;
	if (argaddr(1, &stack) < 0) 
		return -1;
	
	return clone(flag, stack);
}

uint64
sys_wait(void)
{
	uint64 p;
	if(argaddr(0, &p) < 0)
		return -1;
	// since wait suspends the proc, we print the left trace-info here
	// and when coming back, we re-print the leading trace-info for a clear view
	struct proc *pr = myproc();
	int mask = pr->tmask;// & (1 << (SYS_wait - 1));
	if (mask) {
		printf(") ...\n");
	}
	__debug_info("sys_wait", "p = %p\n", p);
	int ret = wait4(-1, p, 0);
	if (mask) {
		printf("pid %d: return from wait(0x%x", pr->pid, p);
	}
	return ret;
}

uint64 sys_wait4(void) {
	int pid;
	uint64 status, options;

	// extract syscall args 
	if (argint(0, &pid) < 0) 
		return -1;
	if (argaddr(1, &status) < 0) 
		return -1;
	if (argaddr(2, &options) < 0) 
		return -1;

	struct proc *pr = myproc();
	int mask = pr->tmask;
	if (mask) {
		printf(") ...\n");
	}
	int ret = wait4(pid, status, options);
	if (mask) {
		printf("pid %d: return from wait4(%p", pr->pid, status);
	}

	return ret;
}

// yield has no arg
uint64 sys_sched_yield(void) {
	myproc()->vswtch += 1;
	yield();
	return 0;
}

uint64
sys_sleep(void)
{
	int n;
	int ret = 0;
	uint ticks0;

	if(argint(0, &n) < 0)
		return -1;
	struct proc *p = myproc();
	int mask = p->tmask;// & (1 << (SYS_sleep - 1));
	if (mask) {
		printf(") ...\n");
	}
	acquire(&p->lk);
	ticks0 = ticks;
	while(ticks - ticks0 < n){
		if(p->killed){
			ret = -1;
			break;
		}
		sleep(&ticks, &p->lk);
	}
	release(&p->lk);

	if (mask) {
		printf("pid %d: return from sleep(%d", p->pid, n);
	}
	return ret;
}

uint64 sys_nanosleep(void) {
	uint64 addr_sec, addr_usec;

	if (argaddr(0, &addr_sec) < 0) 
		return -1;
	if (argaddr(1, &addr_usec) < 0) 
		return -1;

	struct proc *p = myproc();
	uint64 sec, usec;
	if (copyin2((char*)&sec, addr_sec, sizeof(sec)) < 0) 
		return -1;
	if (copyin2((char*)&usec, addr_usec, sizeof(usec)) < 0) 
		return -1;
	int n = sec * 20 + usec / 50000000;

	int mask = p->tmask;
	if (mask) {
		printf(") ...\n");
	}
	acquire(&p->lk);
	uint64 tick0 = ticks;
	while (ticks - tick0 < n) {
		if (p->killed) {
			return -1;
		}
		sleep(&ticks, &p->lk);
	}
	release(&p->lk);

	return 0;
}

uint64
sys_kill(void)
{
	int pid;

	if(argint(0, &pid) < 0)
		return -1;
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
sys_trace(void)
{
	// int mask;
	// if(argint(0, &mask) < 0) {
	//   return -1;
	// }
	// myproc()->tmask = mask;
	myproc()->tmask = 1;
	return 0;
}

uint64
sys_getuid(void)
{
	return 0;
}

uint64 
sys_prlimit64(void) {
	// for now it's not very necessary to implement this syscall 
	// may be implemented later 
	return 0;
}

uint64
sys_getrusage(void)
{
	int who;
	uint64 addr;
	struct rusage r;
	struct proc *p = myproc();

	if (argint(0, &who) < 0 || argaddr(1, &addr) < 0)
		return -1;

	memset(&r, 0, sizeof(r));
	switch (who)
	{
		case RUSAGE_SELF:
		case RUSAGE_THREAD:
			convert_to_timeval(p->proc_tms.utime, &r.ru_utime);
			convert_to_timeval(p->proc_tms.stime, &r.ru_stime);
			r.ru_nvcsw = p->vswtch;
			r.ru_nivcsw = p->ivswtch;
			__debug_info("sys_getrusage", "{u: %ds %dus | s: %ds %dus}, nvcsw=%d, nivcsw=%d\n", 
						r.ru_utime.sec, r.ru_utime.usec, r.ru_stime.sec, r.ru_stime.usec, p->vswtch, p->ivswtch);
			break;
		case RUSAGE_CHILDREN:
			convert_to_timeval(p->proc_tms.cutime, &r.ru_utime);
			convert_to_timeval(p->proc_tms.cstime, &r.ru_stime);
			break;
		default:
			return -EINVAL;
	}

	if (copyout2(addr, (char *)&r, sizeof(r)) < 0)
		return -EFAULT;

	return 0;
}
