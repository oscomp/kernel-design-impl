#include <sys/syscall.h>
#include <stdint.h>
#include <type.h>
#include <os/sched.h>

void sys_testdisk()
{
    invoke_syscall(SYSCALL_TESTDISK, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE, IGNORE);
}

void sys_futex_wait(volatile uint64_t *val_addr, uint64_t val)
{
    invoke_syscall(SYSCALL_FUTEX_WAIT, (uintptr_t)val_addr, val, IGNORE, IGNORE, IGNORE);
}

void sys_futex_wakeup(volatile uint64_t *val_addr, int num_wakeup)
{
    invoke_syscall(SYSCALL_FUTEX_WAKEUP, (uintptr_t)val_addr, num_wakeup, IGNORE, IGNORE, IGNORE);
}

long sys_get_timebase()
{
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_exit()
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode)
{
    return invoke_syscall(SYSCALL_EXEC, file_name, argc, argv, mode, IGNORE);
}

int8_t sys_test(const char* filename)
{
    return invoke_syscall(SYSCALL_TEST, filename, IGNORE, IGNORE, IGNORE, IGNORE);
}
