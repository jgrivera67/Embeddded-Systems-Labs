/**
 * @file rtos_wrapper_uCosIII.c
 *
 * RTOS wrapper implementation for uCOSIII APIs
 *
 * @author German Rivera
 */

#include "rtos_wrapper.h"
#include "serial_console.h"
#include "atomic_utils.h"
#include "mem_utils.h"
#include "microcontroller.h"
#include "hw_timer_driver.h"
#include "runtime_log.h"
#include <ucosiii/os_app_hooks.h>
#include <stddef.h>
#include <board.h>

/**
 * Available stack space left when the stack is considered at risk of stack
 * overflow: 10%
 */
#define APP_TASK_STACK_UNFILLED_LIMIT   (APP_TASK_STACK_SIZE / 10)

/**
 * Initializes RTOS
 */
void rtos_init(void)
{
    OS_ERR os_err;

    OSInit(&os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSInit() failed", os_err, 0);
        fatal_error_handler(error);
    }

    OSSchedRoundRobinCfg((CPU_BOOLEAN)1, 0, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSSchedRoundRobinCfg() failed", os_err, 0);
        fatal_error_handler(error);
    }

    App_OS_SetAllHooks();
}


/**
 * Starts RTOS scheduler
 */
void rtos_scheduler_start(void)
{
    OS_ERR os_err;

    OSStart(&os_err);

    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSStart() failed", os_err, 0);
        fatal_error_handler(error);
    }

    /*Unreachable*/
    D_ASSERT(false);
}


static void rtos_tick_timer_callback(void *arg)
{
    D_ASSERT(arg == NULL);

    /*
     * Call uC/OS-III's tick timer processing:
     */
    OSTimeTick();
}


/**
 * Initializes RTOS tick timer interrupt
 */
void rtos_tick_timer_init(void)
{
    D_ASSERT(OSCfg_TickRate_Hz >= 1 && OSCfg_TickRate_Hz <= 1000);

    hw_timer_init(&g_hw_timer0,
                  1000 / OSCfg_TickRate_Hz,
                  rtos_tick_timer_callback,
                  NULL);
}


/**
 * Notify RTOS that we are entering an ISR
 */
void rtos_enter_isr(void)
{
    /*
     * Tell uC/OS-III that we are in an ISR:
     */
    uint32_t int_mask = disable_cpu_interrupts();
    OSIntEnter();
    restore_cpu_interrupts(int_mask);
}


/**
 * Notify RTOS that we are exiting an ISR
 */
void rtos_exit_isr(void)
{
    /*
     * Tell uC/OS-III that we are exiting an ISR:
     */
    OSIntExit();
}


/**
 * Create an RTOS-level task
 */
void rtos_task_create(struct rtos_task *rtos_task_p,
                      const char *task_name_p,
                      rtos_task_function_t *task_function_p,
                      void *task_arg_p,
                      rtos_task_priority_t task_prio)
{
    OS_ERR  os_err;
    error_t error;

    D_ASSERT(rtos_task_p != NULL);
    D_ASSERT(!rtos_task_p->tsk_created);
    D_ASSERT(task_function_p != NULL);
    D_ASSERT(task_prio >= HIGHEST_APP_TASK_PRIORITY &&
             task_prio <= LOWEST_APP_TASK_PRIORITY);

    rtos_task_p->tsk_signature = TASK_SIGNATURE;
    rtos_task_p->tsk_name_p = task_name_p;
    rtos_task_p->tsk_created = true;
    rtos_task_p->tsk_stack_overflow_marker = STACK_OVERFLOW_MARKER;
    rtos_task_p->tsk_stack_underflow_marker = STACK_UNDERFLOW_MARKER;
    rtos_task_p->tsk_max_stack_entries_used = 0;

    /*
     * Create the uCOS-III task
     */
    OSTaskCreate(&rtos_task_p->tsk_tcb,
                 (char *)task_name_p,
                 task_function_p,
                 task_arg_p,
                 task_prio,
                 rtos_task_p->tsk_stack,
                 APP_TASK_STACK_UNFILLED_LIMIT,
                 APP_TASK_STACK_SIZE,
                 // q_size:
                 0,
                 // time_quanta:
                 0,
                 // p_ext:
                 NULL,
                 OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 &os_err);

    if (os_err != OS_ERR_NONE) {
        error = CAPTURE_ERROR("OSTaskCreate() failed", os_err, rtos_task_p);
        fatal_error_handler(error);
    }
}


/**
 * Returns that pointer to the calling task
 *
 * @return task object pointer
 */
struct rtos_task *rtos_task_self(void)
{
	if (!CPU_USING_PSP_STACK_POINTER()) {
		/*
		 * Caller is an exception handler:
		 */
		return NULL;
	}

	D_ASSERT(CPU_MODE_IS_THREAD());

    struct rtos_task *task_p = ENCLOSING_STRUCT(OSTCBCurPtr, struct rtos_task,
                                                tsk_tcb);

    D_ASSERT(task_p->tsk_signature == TASK_SIGNATURE);
    return task_p;
}


/**
 * Changes the priority of the calling task
 *
 * @param new_task_prio new task priority
 */
void rtos_task_change_self_priority(rtos_task_priority_t new_task_prio)
{
    OS_ERR os_err;

    OSTaskChangePrio(NULL, new_task_prio, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTaskChangePrio() failed", os_err, NULL);
        fatal_error_handler(error);
    }
}


/**
 * Terminate a task
 */
void rtos_task_kill(struct rtos_task *rtos_task_p)
{
    OS_ERR os_err;

    D_ASSERT(rtos_task_p->tsk_signature == TASK_SIGNATURE);
    D_ASSERT(rtos_task_p->tsk_created);

    rtos_task_p->tsk_created = false;
    OSTaskDel(&rtos_task_p->tsk_tcb, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTaskDel() failed", os_err, rtos_task_p);
        fatal_error_handler(error);
    }
}


/**
 * Terminate the calling task
 */
void rtos_task_exit(void)
{
    rtos_task_kill(rtos_task_self());

    /*
     * Unreachable
     */
    D_ASSERT(false);
}


/**
 * Delays the current task a given number of milliseconds
 */
void rtos_task_delay(uint32_t ms)
{
    OS_ERR os_err;

    D_ASSERT(ms != 0);

    OSTimeDlyHMSM(0, 0, 0, ms,
                  OS_OPT_TIME_HMSM_STRICT, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTimeDly() failed", os_err, OSTCBCurPtr);
        fatal_error_handler(error);
    }
}


/**
 * Block calling task on its built-in semaphore
 */
void rtos_task_semaphore_wait(void)
{
    OS_ERR os_err;

    OSTaskSemPend(0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTaskSemPend() failed", os_err, OSTCBCurPtr);
        fatal_error_handler(error);
    }
}


/**
 * Wake up a task by signaling its built-in semaphore
 */
void rtos_task_semaphore_signal(struct rtos_task *rtos_task_p)
{
    OS_ERR os_err;

    D_ASSERT(rtos_task_p->tsk_signature == TASK_SIGNATURE);

    OSTaskSemPost(&rtos_task_p->tsk_tcb, OS_OPT_POST_NONE, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTaskSemPost() failed", os_err, OSTCBCurPtr);
        fatal_error_handler(error);
    }
}


/**
 * Initializes an RTOS-level mutex
 */
void rtos_mutex_init(struct rtos_mutex *rtos_mutex_p,
                     const char *mutex_name_p)
{
    OS_ERR  os_err;

    D_ASSERT(rtos_mutex_p != NULL);
    rtos_mutex_p->mtx_signature = MUTEX_SIGNATURE;

    OSMutexCreate(&rtos_mutex_p->mtx_os_mutex,
                  (char *)mutex_name_p,
                  &os_err);

    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSMutexCreate() failed", os_err, rtos_mutex_p);
        fatal_error_handler(error);
    }
}


/**
 * Acquire an RTOS-level mutex
 */
void rtos_mutex_lock(struct rtos_mutex *rtos_mutex_p)
{
    OS_ERR  os_err;

    D_ASSERT(rtos_mutex_p->mtx_signature == MUTEX_SIGNATURE);
    D_ASSERT(CPU_MODE_IS_THREAD() && CPU_INTERRUPTS_ARE_ENABLED());

    OSMutexPend(&rtos_mutex_p->mtx_os_mutex,
                0, // timeout
                OS_OPT_PEND_BLOCKING,
                NULL,
                &os_err);

    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSMutexPend() failed", os_err, rtos_mutex_p);
        fatal_error_handler(error);
    }
}


/**
 * Release a RTOS-level mutex
 */
void rtos_mutex_unlock(struct rtos_mutex *rtos_mutex_p)
{
    OS_ERR  os_err;

    D_ASSERT(rtos_mutex_p->mtx_signature == MUTEX_SIGNATURE);
    D_ASSERT(CPU_MODE_IS_THREAD());

    OSMutexPost(&rtos_mutex_p->mtx_os_mutex, OS_OPT_POST_NONE, &os_err);

    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSMutexPost() failed", os_err, rtos_mutex_p);
        fatal_error_handler(error);
    }
}


/**
 * Tell if the calling task owns a given mutex
 */
bool rtos_mutex_is_mine(const struct rtos_mutex *rtos_mutex_p)
{
    return rtos_mutex_p->mtx_os_mutex.OwnerTCBPtr == OSTCBCurPtr;
}


/**
 * Initializes an RTOS-level semaphore
 */
void rtos_semaphore_init(struct rtos_semaphore *rtos_semaphore_p,
                         const char *semaphore_name_p,
                         uint32_t initial_count)
{
    OS_ERR  os_err;

    D_ASSERT(rtos_semaphore_p != NULL);
    rtos_semaphore_p->sem_signature = SEMAPHORE_SIGNATURE;

    OSSemCreate(&rtos_semaphore_p->sem_os_semaphore,
                (char *)semaphore_name_p,
                (OS_SEM_CTR)initial_count,
                &os_err);

    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSSemCreate() failed", os_err, rtos_semaphore_p);
        fatal_error_handler(error);
    }
}


/**
 * Waits on an RTOS-level semaphore
 */
void rtos_semaphore_wait(struct rtos_semaphore *rtos_semaphore_p)
{
    OS_ERR  os_err;

    D_ASSERT(rtos_semaphore_p->sem_signature == SEMAPHORE_SIGNATURE);
    D_ASSERT(CPU_MODE_IS_THREAD() && CPU_INTERRUPTS_ARE_ENABLED());

    OSSemPend(&rtos_semaphore_p->sem_os_semaphore,
              0,  // timeout
              OS_OPT_PEND_BLOCKING,
              NULL,
              &os_err);

    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSSemPend() failed", os_err, rtos_semaphore_p);
        fatal_error_handler(error);
    }
}


/**
 * Waits with timeout on an RTOS-level semaphore
 *
 * @param rtos_semaphore_p  pointer to the semaphore
 * @param timeout_ms        timeout in milliseconds
 *
 * @return true     if the semaphore was signaled before the timeout expired
 * @return false    otherwise
 */
bool rtos_semaphore_wait_timeout(struct rtos_semaphore *rtos_semaphore_p,
                                 uint32_t timeout_ms)
{
    OS_ERR  os_err;

    D_ASSERT(rtos_semaphore_p->sem_signature == SEMAPHORE_SIGNATURE);
    D_ASSERT(CPU_MODE_IS_THREAD() && CPU_INTERRUPTS_ARE_ENABLED());
    D_ASSERT(timeout_ms >= MS_PER_TIMER_TICK);

    OSSemPend(&rtos_semaphore_p->sem_os_semaphore,
              timeout_ms / MS_PER_TIMER_TICK,
              OS_OPT_PEND_BLOCKING,
              NULL,
              &os_err);

    if (os_err == OS_ERR_TIMEOUT) {
        return false;
    }

    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSSemPend() failed", os_err,
                                      rtos_semaphore_p);
        fatal_error_handler(error);
        /*UNREACHABLE*/
    }

    return true;
}


/**
 * Signal an RTOS-level semaphore. It wakes up the highest priority waiter
 */
void rtos_semaphore_signal(struct rtos_semaphore *rtos_semaphore_p)
{
    OS_ERR  os_err;

    D_ASSERT(rtos_semaphore_p->sem_signature == SEMAPHORE_SIGNATURE);

    OSSemPost(&rtos_semaphore_p->sem_os_semaphore, OS_OPT_POST_1, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSSemPost() failed", os_err, rtos_semaphore_p);
        fatal_error_handler(error);
    }
}


/**
 * Broadcast an RTOS-level semaphore. It wakes up all waiters
 */
void rtos_semaphore_broadcast(struct rtos_semaphore *rtos_semaphore_p)
{
    OS_ERR  os_err;

    D_ASSERT(rtos_semaphore_p->sem_signature == SEMAPHORE_SIGNATURE);

    OSSemPost(&rtos_semaphore_p->sem_os_semaphore, OS_OPT_POST_ALL, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSSemPost() failed", os_err, rtos_semaphore_p);
        fatal_error_handler(error);
    }
}


/**
 * Initializes an RTOS-level timer
 */
void rtos_timer_init(struct rtos_timer *rtos_timer_p,
                     const char *timer_name_p,
                     uint32_t milliseconds,
                     bool periodic,
                     rtos_timer_callback_t *timer_callback_p,
                     void *arg)
{
    OS_ERR  os_err;
    OS_TICK ticks;

    D_ASSERT(milliseconds >= MS_PER_TIMER_TICK);
    ticks = milliseconds / MS_PER_TIMER_TICK;

    D_ASSERT(rtos_timer_p != NULL);
    rtos_timer_p->tmr_signature = TIMER_SIGNATURE;

    if (periodic) {
        OSTmrCreate(&rtos_timer_p->tmr_os_timer,
                    (char *)timer_name_p,
                    0, /* delay */
                    ticks,
                    OS_OPT_TMR_PERIODIC,
                    (OS_TMR_CALLBACK_PTR)timer_callback_p,
                    arg,
                    &os_err);
    } else {
        OSTmrCreate(&rtos_timer_p->tmr_os_timer,
                    (char *)timer_name_p,
                    ticks,
                    0, /* period */
                    OS_OPT_TMR_ONE_SHOT,
                    (OS_TMR_CALLBACK_PTR)timer_callback_p,
                    arg,
                    &os_err);
    }

    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTmrCreate() failed", os_err, rtos_timer_p);
        fatal_error_handler(error);
    }
}


/**
 * Starts an RTOS-level timer
 */
void rtos_timer_start(struct rtos_timer *rtos_timer_p)
{
    OS_ERR  os_err;
    bool timer_started;

    D_ASSERT(rtos_timer_p->tmr_signature == TIMER_SIGNATURE);
    timer_started = OSTmrStart(&rtos_timer_p->tmr_os_timer, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTmrStart() failed", os_err, rtos_timer_p);
        fatal_error_handler(error);
    }

    D_ASSERT(timer_started);
}


/**
 * Stops an RTOS-level timer
 */
void rtos_timer_stop(struct rtos_timer *rtos_timer_p)
{
    OS_ERR  os_err;
    bool timer_stopped;

    D_ASSERT(rtos_timer_p->tmr_signature == TIMER_SIGNATURE);
    timer_stopped = OSTmrStop(&rtos_timer_p->tmr_os_timer, OS_OPT_TMR_NONE,
                              NULL, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTmrStop() failed", os_err, rtos_timer_p);
        fatal_error_handler(error);
    }

    D_ASSERT(timer_stopped);
}


/**
 * Check if stack overflow has occurred on the task stack.
 * It updates task-p->tsk_max_stack_entries_used.
 */
void rtos_task_check_stack(struct rtos_task *task_p)
{
    OS_ERR os_err;
    CPU_STK_SIZE  free_entries;
    CPU_STK_SIZE  used_entries;

    D_ASSERT(task_p->tsk_created);

    OSTaskStkChk(&task_p->tsk_tcb, &free_entries, &used_entries, &os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTaskStkChk() failed", os_err, task_p);
        fatal_error_handler(error);
    }

    if (used_entries > task_p->tsk_max_stack_entries_used) {
        task_p->tsk_max_stack_entries_used = used_entries;
    }

    if (used_entries >= APP_TASK_STACK_SIZE ||
        task_p->tsk_stack_overflow_marker != STACK_OVERFLOW_MARKER) {
        /*
         * Stack overflow detected
         */

        /*
         * Disable interrupts to ensure that we don't get preempted here,
         * by the offending task:
         */
        uint32_t old_primask = disable_cpu_interrupts();

        /*
         * Kill the offending task:
         */
        rtos_task_kill(task_p);
        restore_cpu_interrupts(old_primask);

        ERROR_PRINTF("\n*** Stack overflow detected for task '%s': %u stack entries used.\n",
                       task_p->tsk_name_p, used_entries);
        ERROR_PRINTF("*** Task '%s' killed.\n", task_p->tsk_name_p);
    }
}


/**
 * Return Time since boot in ticks
 */
uint32_t rtos_get_ticks_since_boot(void)
{
    OS_ERR os_err;
    OS_TICK ticks;

    ticks = OSTimeGet(&os_err);
    if (os_err != OS_ERR_NONE) {
        error_t error = CAPTURE_ERROR("OSTimeGet() failed", os_err, ticks);
        fatal_error_handler(error);
        /*UNREACHABLE*/
    }

    return ticks;
}


/**
 * Return Time since boot in seconds
 */
uint32_t rtos_get_time_since_boot(void)
{
    return rtos_get_ticks_since_boot() / OSCfg_TickRate_Hz;
}
