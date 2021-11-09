/**
 * @file rtos_wrapper.h
 *
 * RTOS wrapper interface for uCOS-III
 *
 * @author German Rivera 
 */ 
#ifndef __RTOS_WRAPPER_H
#define __RTOS_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <ucosiii/os.h>
#include <ucosiii/os_cfg_app.h>
#include "compile_time_checks.h"
#include "runtime_checks.h"
#include "mem_utils.h"

/**
 * For uCOS-III, application task priorities must be in the range
 * 2 .. OS_CFG_PRIO_MAX-3. Lower number means higher priority.
 */
#define HIGHEST_APP_TASK_PRIORITY    2
#define LOWEST_APP_TASK_PRIORITY     (OS_CFG_PRIO_MAX - 3)

C_ASSERT(HIGHEST_APP_TASK_PRIORITY < LOWEST_APP_TASK_PRIORITY);

/**
 * application task default stack size in number of 32-bit entries
 */
#define APP_TASK_STACK_SIZE UINT32_C(256)

/**
 * Number of milliseconds per RTOS timer tick
 */
#define MS_PER_TIMER_TICK    (1000 / OS_CFG_TMR_TASK_RATE_HZ)

C_ASSERT(OS_CFG_TMR_TASK_RATE_HZ < 1000);
C_ASSERT(1000 % OS_CFG_TMR_TASK_RATE_HZ == 0);

/**
 * Calculate difference between two values of RTOS timer ticks
 */
#define RTOS_TICKS_DELTA(_begin_ticks, _end_ticks) \
        ((uint32_t)((int32_t)(_end_ticks) - (int32_t)(_begin_ticks)))

#define MILLISECONDS_TO_TICKS(_milli_secs) \
        ((uint32_t)HOW_MANY(_milli_secs, MS_PER_TIMER_TICK))

/**
 * Wrapper for an RTOS task object
 */
struct rtos_task
{
#   define      TASK_SIGNATURE  GEN_SIGNATURE('T', 'A', 'S', 'K')
    uint32_t    tsk_signature;

    /**
     * uCOS-III task control block
     */
    OS_TCB      tsk_tcb;

    /**
     * Pointer to task name
     */
    const char *tsk_name_p;

    /**
     *Flag indicating that the task has been created (and not killed yet)
     */
    volatile bool tsk_created;

    uint32_t    tsk_stack_overflow_marker;

    /**
     * Task's stack
     */
    CPU_STK     tsk_stack[APP_TASK_STACK_SIZE];

    uint32_t    tsk_stack_underflow_marker;

    /*
     * Maximum number of stack entries used (high water mark)
     */
    uint32_t tsk_max_stack_entries_used;
};

/**
 * Wrapper for an RTOS mutex object
 */
struct rtos_mutex 
{
#   define      MUTEX_SIGNATURE  GEN_SIGNATURE('M', 'U', 'T', 'X')
    uint32_t    mtx_signature;

    OS_MUTEX    mtx_os_mutex;
};

/**
 * Wrapper for an RTOS semaphore object
 */
struct rtos_semaphore 
{
#   define      SEMAPHORE_SIGNATURE  GEN_SIGNATURE('S', 'E', 'M', 'A')
    uint32_t    sem_signature;

    OS_SEM      sem_os_semaphore;
};

/**
 * Wrapper for an RTOS timer object
 */
struct rtos_timer
{
    /**
     * The tmr_os_timer field must be first, since pointers to this struct are type-casted
     * to OS_TMR *
     */
    OS_TMR      tmr_os_timer;

#   define      TIMER_SIGNATURE  GEN_SIGNATURE('T', 'I', 'M', 'R')
    uint32_t    tmr_signature;
};

C_ASSERT(offsetof(struct rtos_timer, tmr_os_timer) == 0);

/**
 * Task priority type
 */
typedef OS_PRIO rtos_task_priority_t;

/**
 * Signature of a task function
 */
typedef void rtos_task_function_t(void *arg);

/**
 * Signature of a timer callback function
 */
typedef void rtos_timer_callback_t(struct rtos_timer *rtos_timer_p, void *arg);

void rtos_init(void);

void rtos_scheduler_start(void);

void rtos_tick_timer_init(void);

void rtos_task_create(struct rtos_task *rtos_task_p,
                      const char *task_name_p,
                      rtos_task_function_t *task_function_p,
                      void *task_arg_p,
                      rtos_task_priority_t task_prio);

void rtos_task_kill(struct rtos_task *rtos_task_p);

struct rtos_task *rtos_task_self(void);

void rtos_task_change_self_priority(rtos_task_priority_t new_task_prio);

void rtos_task_exit(void);

void rtos_task_delay(uint32_t ms);

void rtos_task_semaphore_wait(void);

void rtos_task_semaphore_signal(struct rtos_task *rtos_task_p);

void rtos_mutex_init(struct rtos_mutex *rtos_mutex_p,
                     const char *mutex_name_p);
 
void rtos_mutex_lock(struct rtos_mutex *rtos_mutex_p);
 
void rtos_mutex_unlock(struct rtos_mutex *rtos_mutex_p);

bool rtos_mutex_is_mine(const struct rtos_mutex *rtos_mutex_p);

void rtos_semaphore_init(struct rtos_semaphore *rtos_semaphore_p,
                         const char *semaphore_name_p,
                         uint32_t initial_count);
 
void rtos_semaphore_wait(struct rtos_semaphore *rtos_semaphore_p);
 
bool rtos_semaphore_wait_timeout(struct rtos_semaphore *rtos_semaphore_p,
		                         uint32_t timeout_ms);

void rtos_semaphore_signal(struct rtos_semaphore *rtos_semaphore_p);

void rtos_semaphore_broadcast(struct rtos_semaphore *rtos_semaphore_p);

void rtos_timer_init(struct rtos_timer *rtos_timer_p,
                     const char *timer_name_p,
                     uint32_t milliseconds,
                     bool periodic,
                     rtos_timer_callback_t *timer_callback_p,
                     void *arg);

void rtos_timer_start(struct rtos_timer *rtos_timer_p);

void rtos_timer_stop(struct rtos_timer *rtos_timer_p);

void rtos_task_check_stack(struct rtos_task *task_p);

void rtos_enter_isr(void);

void rtos_exit_isr(void);

uint32_t rtos_get_ticks_since_boot(void);

uint32_t rtos_get_time_since_boot(void);

#endif /*  __RTOS_WRAPPER_H */
