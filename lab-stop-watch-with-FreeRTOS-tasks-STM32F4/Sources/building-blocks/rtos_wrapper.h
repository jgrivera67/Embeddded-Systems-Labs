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
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <timers.h>
#include "compile_time_checks.h"
#include "runtime_checks.h"
#include "mem_utils.h"
#include "memory_protection_unit.h"

/**
 * For FreeRTOS, lower number means lower priority.
 */
#define HIGHEST_APP_TASK_PRIORITY    (configMAX_PRIORITIES - 1)
#define LOWEST_APP_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)

C_ASSERT(HIGHEST_APP_TASK_PRIORITY > LOWEST_APP_TASK_PRIORITY);

/**
 * application task default stack size in number of 32-bit entries
 */
#define APP_TASK_STACK_SIZE UINT32_C(256)

C_ASSERT(APP_TASK_STACK_SIZE >= configMINIMAL_STACK_SIZE);

/**
 * application task default stack size in bytes
 */
#define APP_TASK_STACK_SIZE_BYTES (APP_TASK_STACK_SIZE * sizeof(uint32_t))

/**
 * Number of milliseconds per RTOS timer tick
 */
#define MS_PER_TIMER_TICK    portTICK_PERIOD_MS

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
    /**
     * Task's stack (must be the first field of the structure)
     */
    StackType_t tsk_stack[APP_TASK_STACK_SIZE];

#   define      TASK_SIGNATURE  GEN_SIGNATURE('T', 'A', 'S', 'K')
    uint32_t    tsk_signature;

    /**
     * Task control block
     */
    StaticTask_t tsk_tcb;

    /**
     * FreeRTOS task handle
     */
    TaskHandle_t tsk_handle;

    /**
     * Pointer to task name
     */
    const char *tsk_name_p;

    /**
     *Flag indicating that the task has been created
     */
    volatile bool tsk_created;

    /**
     * Maximum number of stack entries used (high water mark)
     */
    uint16_t tsk_max_stack_entries_used;

    /**
     * Saved state of thread-specific MPU region descriptors
     */
    struct thread_regions tsk_mpu_regions;

} __attribute__ ((aligned(2*APP_TASK_STACK_SIZE_BYTES)));

/**
 * Wrapper for an RTOS mutex object
 */
struct rtos_mutex
{
#   define      MUTEX_SIGNATURE  GEN_SIGNATURE('M', 'U', 'T', 'X')
    uint32_t    mtx_signature;
    const char *mtx_name;
    StaticSemaphore_t mtx_os_mutex_var;

    /**
     * Handle returned by xSemaphoreCreateMutexStatic()
     */
    SemaphoreHandle_t mtx_os_mutex_handle;
};

/**
 * Wrapper for an RTOS semaphore object
 */
struct rtos_semaphore
{
#   define      SEMAPHORE_SIGNATURE  GEN_SIGNATURE('S', 'E', 'M', 'A')
    uint32_t    sem_signature;
    const char *sem_name;
    StaticSemaphore_t sem_os_semaphore_var;

    /**
     * Handle returned by xSemaphoreCreateBinaryStatic() or
     * xSemaphoreCreateCountingStatic()
     */
    SemaphoreHandle_t sem_os_semaphore_handle;
};

struct rtos_timer;

/**
 * Signature of a timer callback function
 */
typedef void rtos_timer_callback_t(struct rtos_timer *rtos_timer_p, void *arg);

/**
 * Wrapper for an RTOS timer object
 */
struct rtos_timer
{
#   define      TIMER_SIGNATURE  GEN_SIGNATURE('T', 'I', 'M', 'R')
    uint32_t    tmr_signature;

    StaticTimer_t tmr_os_timer_var;

    /**
     * Handle returned by xTimerCreateStatic()
     */
    TimerHandle_t tmr_os_timer_handle;

    rtos_timer_callback_t *tmr_callback_p;
    void *tmr_arg;
};

/**
 * Task priority type
 */
typedef UBaseType_t rtos_task_priority_t;

/**
 * Signature of a task function
 */
typedef void rtos_task_function_t(void *arg);

void rtos_init(void);

void rtos_scheduler_start(void);

void rtos_tick_timer_init(void);

void rtos_task_create(struct rtos_task *rtos_task_p,
                      const char *task_name_p,
                      rtos_task_function_t *task_function_p,
                      void *task_arg_p,
                      rtos_task_priority_t task_prio);

struct rtos_task *rtos_task_get_current(void);

struct rtos_task *rtos_task_self(void);

void rtos_task_change_self_priority(rtos_task_priority_t new_task_prio);

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
