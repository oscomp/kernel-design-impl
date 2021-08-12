#include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <type.h>

#include <stdio.h>
#include <os/sched.h>

LIST_HEAD(timers);
LIST_HEAD(available_timers);
timer_t all_timers[NUM_TIMER];

uint64_t time_elapsed = 0;
uint32_t time_base = 0;

void init_timers()
{
    for (int32_t i = 0; i < NUM_TIMER; ++i)
        list_add_tail(&all_timers[i].list, &available_timers);
}

/* if no available timer, return NULL */
static inline timer_t *alloc_timer()
{
    return (available_timers.next == &available_timers)? NULL : available_timers.next;
}

/* create a timer to sleep */
/* call func(parameter) when timeout */
/* tick is sleep interval ticks */
void timer_create(TimerCallback func, void* parameter, uint64_t tick)
{
    disable_preempt();

    timer_t *newtimer = alloc_timer();
    assert(newtimer != NULL);

    newtimer->timeout_tick = get_ticks() + tick;
    newtimer->callback_func = func;
    newtimer->parameter = parameter;
    newtimer->list.ptr = newtimer;
    list_del(&newtimer->list);
    list_add_tail(&newtimer->list,&timers);

    enable_preempt();
}

void timer_check()
{
    disable_preempt();
    timer_t *handling_timer, *next_timer;
    uint64_t nowtick = get_ticks();
    // check all timers
    list_for_each_entry_safe(handling_timer, next_timer, &timers, list)
    {
        if (handling_timer->timeout_tick < nowtick)
        {            
            list_del(&handling_timer->list);
            list_add_tail(&handling_timer->list, &available_timers);
            (*handling_timer->callback_func)(handling_timer->parameter);
        }
    }
    enable_preempt();
}

uint64_t do_times(struct tms *tms)
{
    uint64_t now_tick = get_ticks();
    tms->tms_utime = current_running->utime;
    tms->tms_stime = current_running->stime;
    tms->tms_cutime = 0; tms->tms_cstime = 0;
    for (uint i = 0; i < NUM_MAX_TASK; ++i)
        if (pcb[i].parent.parent == current_running){
            tms->tms_cutime += pcb[i].utime;
            tms->tms_cstime += pcb[i].stime;
        }
    return now_tick;
}

/* 成功返回0， 失败返回-1 */
int8_t do_gettimeofday(struct timespec *ts)
{
    debug();
    uint64_t nowtick = get_ticks();

    ts->tv_sec = (uint32)(nowtick / time_base);

    uint64_t left = nowtick % time_base;
    ts->tv_nsec = 0;

    for (uint i = 0; i < NANO; ++i)
    {
        ts->tv_nsec = 10*ts->tv_nsec + left * 10 / time_base;
        left = (left * 10) % time_base;
    }
    ts->tv_sec=1628129642; ts->tv_nsec=613489360;
    // log(0, "%d %d\n", ts->tv_sec, ts->tv_nsec);
    return 0;
}

/* 成功返回0， 失败返回-1 */
int32_t do_clock_gettime(uint64_t clock_id, struct timespec *tp)
{
    assert(clock_id == CLOCK_REALTIME);
    if (clock_id == CLOCK_REALTIME){
        return do_gettimeofday(tp);
    }
    else
        ;
}

/* success return 0 */
uint8 do_nanosleep(struct timespec *ts)
{
    debug();
    // 1. block the current_running
    // 2. create a timer which calls `do_unblock` when timeout
    // 3. reschedule because the current_running is blocked.
    do_block(&current_running->list,&general_block_queue);

    uint64_t sleep_ticks = 0, nsec = ts->tv_nsec;
    // printk_port("time: %d, %d\n", ts->tv_sec, ts->tv_nsec);
    for (uint i = 0; i < NANO; ++i){
        sleep_ticks = (sleep_ticks / 10)+ time_base * (nsec % 10);
        nsec /= 10;
    }

    sleep_ticks += ts->tv_sec * time_base;
    timer_create(&do_unblock, &current_running->list, sleep_ticks);
    do_scheduler();
    return 0;
}


/* start and end counter */
/* for cpu compute SYS cpu time and USER cpu time */
/* start: enter kernel, end: exit kernel */
static uint64_t last_time;
void kernel_time_count()
{
    uint64_t now_tick = get_ticks();
    current_running->stime += now_tick - last_time;
    last_time = now_tick;
}
void user_time_count()
{
    uint64_t now_tick = get_ticks();
    current_running->utime += now_tick - last_time;
    last_time = now_tick;
}


uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}

void get_regular_time_from_spec(struct regular_time *mytp, struct timespec *tp)
{
    if (!mytp) return ;
    mytp->nano_seconds = tp->tv_nsec;

    time_t sec = tp->tv_sec;

    mytp->seconds = sec % SECONDS_PER_MIN;
    sec -= mytp->seconds;

    mytp->min = (sec / SECONDS_PER_MIN) % MIN_PER_HOUR;
    sec -= mytp->min * SECONDS_PER_MIN;

    mytp->hour = (sec / MIN_PER_HOUR) % HOUR_PER_DAY;
    sec -= mytp->hour * MIN_PER_HOUR * SECONDS_PER_MIN;

    return ;
}
