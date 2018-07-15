/**
 * @file rtos_wrapper_FreeRTOS.c
 *
 * RTOS wrapper implementation for FreeRTOS APIs
 *
 * @author German Rivera
 */

#include "rtos_wrapper.h"
#include "atomic_utils.h"
#include "mem_utils.h"
#include "microcontroller.h"
#include "runtime_log.h"
#include <stddef.h>
#include "memory_protection_unit.h"

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

/**
 * Available stack space left when the stack is considered at risk of stack
 * overflow: 10%
 */
#define APP_TASK_STACK_UNFILLED_LIMIT   (APP_TASK_STACK_SIZE / 10)

/**
 * Counter of nested ISRs
 */
static volatile uint8_t g_nested_ISR_count = 0;

/**
 * Flag to track when a task context switch needs to be done during
 * an ISR exit
 */
static BaseType_t g_rtos_task_context_switch_required = pdFALSE;

/**
 * saved writable state for the global background data region, for each possible
 * interrupted context for nested interrupts.
 */
static bool g_interrupted_background_region_writable_state[MCU_NUM_INTERRUPT_PRIORITIES];

/**
 * Initializes RTOS
 */
void rtos_init(void)
{
}


/**
 * Starts RTOS scheduler
 */
void rtos_scheduler_start(void)
{
    (void)set_writable_background_region(true);
    vTaskStartScheduler();

    /*Unreachable*/
    D_ASSERT(false);
}


/**
 * Notify RTOS that we are entering an ISR
 */
void rtos_enter_isr(void)
{
	D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

	bool old_writable = set_writable_background_region(true);
    uint_fast8_t prev_nested_ISR_count = ATOMIC_POST_INCREMENT_UINT8(&g_nested_ISR_count);

    D_ASSERT(prev_nested_ISR_count < MCU_NUM_INTERRUPT_PRIORITIES);
    g_interrupted_background_region_writable_state[prev_nested_ISR_count] = old_writable;
    (void)set_writable_background_region(false);
}


/**
 * Notify RTOS that we are exiting an ISR
 */
void rtos_exit_isr(void)
{
	D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

	bool old_writable = set_writable_background_region(true);
    uint_fast8_t prev_nested_ISR_count = ATOMIC_POST_DECREMENT_UINT8(&g_nested_ISR_count);

    D_ASSERT(!old_writable);
    D_ASSERT(prev_nested_ISR_count >= 1);
    if (prev_nested_ISR_count == 1) {
        portEND_SWITCHING_ISR(g_rtos_task_context_switch_required);
        g_rtos_task_context_switch_required = pdFALSE;
    }

    (void)set_writable_background_region(
    	g_interrupted_background_region_writable_state[prev_nested_ISR_count - 1]);
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
    error_t error;
	struct mpu_region_descriptor old_region;
	bool old_write_enabled;


    D_ASSERT(rtos_task_p != NULL);
    D_ASSERT(!rtos_task_p->tsk_created);
    D_ASSERT(task_function_p != NULL);
    D_ASSERT(task_prio >= LOWEST_APP_TASK_PRIORITY &&
             task_prio <= HIGHEST_APP_TASK_PRIORITY);

    set_private_data_region(rtos_task_p, sizeof(*rtos_task_p), READ_WRITE, &old_region);
    rtos_task_p->tsk_signature = TASK_SIGNATURE;
    rtos_task_p->tsk_name_p = task_name_p;
    rtos_task_p->tsk_created = true;
    rtos_task_p->tsk_max_stack_entries_used = 0;

    /*
     * Create the FreeRTOS task:
     */
    old_write_enabled = set_writable_background_region(true);
    rtos_task_p->tsk_handle = xTaskCreateStatic(task_function_p,
    						task_name_p,
						APP_TASK_STACK_SIZE,
						task_arg_p,
						task_prio,
						rtos_task_p->tsk_stack,
						&rtos_task_p->tsk_tcb);

    if (rtos_task_p->tsk_handle == NULL) {
        error = CAPTURE_ERROR("xTaskCreate() failed", rtos_task_p, task_name_p);
        fatal_error_handler(error);
    }

    vTaskSetApplicationTaskTag(rtos_task_p->tsk_handle, (void *)rtos_task_p);
    (void)set_writable_background_region(old_write_enabled);
}


/**
 * Returns that pointer to the current task
 *
 * @return task object pointer
 */
struct rtos_task *rtos_task_get_current(void)
{
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    struct rtos_task *task_p = (void *)xTaskGetApplicationTaskTag(task_handle);

    D_ASSERT(task_p->tsk_signature == TASK_SIGNATURE);
    return task_p;
}

/**
 * Returns that pointer to the calling task
 *
 * @return task object pointer
 */
struct rtos_task *rtos_task_self(void)
{
    if (!CALLER_IS_THREAD()) {
        /*
         * Caller is an exception handler:
         */
        return NULL;
    }

    return rtos_task_get_current();
}


/**
 * Changes the priority of the calling task
 *
 * @param new_task_prio new task priority
 */
void rtos_task_change_self_priority(rtos_task_priority_t new_task_prio)
{
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();

    vTaskPrioritySet(task_handle, new_task_prio);
}


/**
 * Delays the current task a given number of milliseconds
 */
void rtos_task_delay(uint32_t ms)
{
    D_ASSERT(ms != 0);
    vTaskDelay(ms / MS_PER_TIMER_TICK);
}


/**
 * Block calling task on its built-in semaphore
 */
void rtos_task_semaphore_wait(void)
{
    error_t error = CAPTURE_ERROR("function not supported for FreeRTOS", rtos_task_semaphore_wait, 0);
    /* TODO: Implement this using ulTaskNotifyTake() */
    fatal_error_handler(error);
}


/**
 * Wake up a task by signaling its built-in semaphore
 */
void rtos_task_semaphore_signal(struct rtos_task *rtos_task_p)
{
    error_t error = CAPTURE_ERROR("function not supported for FreeRTOS" ,
    		                      rtos_task_semaphore_signal, rtos_task_p);

    /* TODO: Implement this using ulTaskNotifyGive() */
    fatal_error_handler(error);
}


/**
 * Initializes an RTOS-level mutex
 */
void rtos_mutex_init(struct rtos_mutex *rtos_mutex_p,
                     const char *mutex_name_p)
{
	struct mpu_region_descriptor old_region;

    D_ASSERT(rtos_mutex_p != NULL);
    set_private_data_region(rtos_mutex_p, sizeof(*rtos_mutex_p), READ_WRITE, &old_region);
    rtos_mutex_p->mtx_signature = MUTEX_SIGNATURE;
    rtos_mutex_p->mtx_name = mutex_name_p;

    bool old_writable = set_writable_background_region(true);
    rtos_mutex_p->mtx_os_mutex_handle =
	xSemaphoreCreateMutexStatic(&rtos_mutex_p->mtx_os_mutex_var);
    (void)set_writable_background_region(old_writable);

    D_ASSERT(rtos_mutex_p->mtx_os_mutex_handle != NULL);
    restore_private_data_region(&old_region);
}


/**
 * Acquire an RTOS-level mutex
 */
void rtos_mutex_lock(struct rtos_mutex *rtos_mutex_p)
{
    BaseType_t rtos_status;
    error_t error;

    D_ASSERT(rtos_mutex_p->mtx_signature == MUTEX_SIGNATURE);
    D_ASSERT(CALLER_IS_THREAD() && CPU_INTERRUPTS_ARE_ENABLED());

	bool old_writable = set_writable_background_region(true);
    rtos_status = xSemaphoreTake(rtos_mutex_p->mtx_os_mutex_handle, portMAX_DELAY);
    (void)set_writable_background_region(old_writable);

    if (rtos_status != pdPASS) {
        error = CAPTURE_ERROR("xSemaphoreTake() failed", rtos_status, rtos_mutex_p);
        fatal_error_handler(error);
    }
}


/**
 * Release a RTOS-level mutex
 */
void rtos_mutex_unlock(struct rtos_mutex *rtos_mutex_p)
{
    BaseType_t rtos_status;
    error_t error;

    D_ASSERT(rtos_mutex_p->mtx_signature == MUTEX_SIGNATURE);
    D_ASSERT(CALLER_IS_THREAD());

	bool old_writable = set_writable_background_region(true);
    rtos_status = xSemaphoreGive(rtos_mutex_p->mtx_os_mutex_handle);
    (void)set_writable_background_region(old_writable);

    if (rtos_status != pdPASS) {
        error = CAPTURE_ERROR("xSemaphoreGive() failed", rtos_status, rtos_mutex_p);
        fatal_error_handler(error);
    }
}


/**
 * Tell if the calling task owns a given mutex
 */
bool rtos_mutex_is_mine(const struct rtos_mutex *rtos_mutex_p)
{
    D_ASSERT(rtos_mutex_p->mtx_signature == MUTEX_SIGNATURE);
    D_ASSERT(CALLER_IS_THREAD());

    TaskHandle_t owner = xSemaphoreGetMutexHolder(rtos_mutex_p->mtx_os_mutex_handle);

    return owner == xTaskGetCurrentTaskHandle();
}

/**
 * Initializes an RTOS-level semaphore
 */
void rtos_semaphore_init(struct rtos_semaphore *rtos_semaphore_p,
                         const char *semaphore_name_p,
                         uint32_t initial_count)
{
	bool old_writable = set_writable_background_region(true);

    D_ASSERT(rtos_semaphore_p != NULL);
    rtos_semaphore_p->sem_signature = SEMAPHORE_SIGNATURE;
    rtos_semaphore_p->sem_name = semaphore_name_p;
    if (initial_count == 0) {
	rtos_semaphore_p->sem_os_semaphore_handle =
	   xSemaphoreCreateBinaryStatic(
		&rtos_semaphore_p->sem_os_semaphore_var);
    } else {
	rtos_semaphore_p->sem_os_semaphore_handle =
	    xSemaphoreCreateCountingStatic(
		initial_count, initial_count,
		&rtos_semaphore_p->sem_os_semaphore_var);
    }

    (void)set_writable_background_region(old_writable);

    D_ASSERT(rtos_semaphore_p->sem_os_semaphore_handle != NULL);
}


/**
 * Waits on an RTOS-level semaphore
 */
void rtos_semaphore_wait(struct rtos_semaphore *rtos_semaphore_p)
{
    BaseType_t rtos_status;
    error_t error;

    D_ASSERT(rtos_semaphore_p->sem_signature == SEMAPHORE_SIGNATURE);
    D_ASSERT(CALLER_IS_THREAD() && CPU_INTERRUPTS_ARE_ENABLED());

	bool old_writable = set_writable_background_region(true);
    rtos_status = xSemaphoreTake(rtos_semaphore_p->sem_os_semaphore_handle,
	                         portMAX_DELAY);
    (void)set_writable_background_region(old_writable);

    if (rtos_status != pdPASS) {
        error = CAPTURE_ERROR("xSemaphoreTake() failed", rtos_status, rtos_semaphore_p);
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
    BaseType_t rtos_status;

    D_ASSERT(rtos_semaphore_p->sem_signature == SEMAPHORE_SIGNATURE);
    D_ASSERT(CALLER_IS_THREAD() && CPU_INTERRUPTS_ARE_ENABLED());

	bool old_writable = set_writable_background_region(true);
    rtos_status = xSemaphoreTake(rtos_semaphore_p->sem_os_semaphore_handle,
                                 timeout_ms / MS_PER_TIMER_TICK);
    (void)set_writable_background_region(old_writable);

    return rtos_status == pdPASS;
}


/**
 * Signal an RTOS-level semaphore. It wakes up the highest priority waiter
 */
void rtos_semaphore_signal(struct rtos_semaphore *rtos_semaphore_p)
{
    D_ASSERT(rtos_semaphore_p->sem_signature == SEMAPHORE_SIGNATURE);
	bool old_writable = set_writable_background_region(true);
    if (CALLER_IS_THREAD()) {
        (void)xSemaphoreGive(rtos_semaphore_p->sem_os_semaphore_handle);
    } else {
        (void)xSemaphoreGiveFromISR(rtos_semaphore_p->sem_os_semaphore_handle,
                                    &g_rtos_task_context_switch_required);
    }
    (void)set_writable_background_region(old_writable);
}


/**
 * Broadcast an RTOS-level semaphore. It wakes up all waiters
 */
void rtos_semaphore_broadcast(struct rtos_semaphore *rtos_semaphore_p)
{
    error_t error = CAPTURE_ERROR("function not supported for FreeRTOS",
    		                      rtos_semaphore_broadcast, rtos_semaphore_p);

    fatal_error_handler(error);
}


static void rtos_timer_internal_callback(TimerHandle_t xTimer)
{
    struct rtos_timer *rtos_timer_p = pvTimerGetTimerID(xTimer);

    D_ASSERT(rtos_timer_p->tmr_signature == TIMER_SIGNATURE);

    rtos_timer_p->tmr_callback_p(rtos_timer_p, rtos_timer_p->tmr_arg);
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
    TickType_t ticks;

    D_ASSERT(milliseconds >= MS_PER_TIMER_TICK);
    ticks = milliseconds / MS_PER_TIMER_TICK;

    D_ASSERT(rtos_timer_p != NULL);
    rtos_timer_p->tmr_signature = TIMER_SIGNATURE;
    rtos_timer_p->tmr_callback_p = timer_callback_p;
    rtos_timer_p->tmr_arg = arg;

    bool old_writable = set_writable_background_region(true);
    rtos_timer_p->tmr_os_timer_handle =
	xTimerCreateStatic(timer_name_p,
                           ticks,
                           periodic,
                           rtos_timer_p,
                           rtos_timer_internal_callback,
			   &rtos_timer_p->tmr_os_timer_var);
    (void)set_writable_background_region(old_writable);

    D_ASSERT(rtos_timer_p->tmr_os_timer_handle != NULL);
}


/**
 * Starts an RTOS-level timer
 */
void rtos_timer_start(struct rtos_timer *rtos_timer_p)
{
    BaseType_t rtos_status;
    error_t error;

    D_ASSERT(rtos_timer_p->tmr_signature == TIMER_SIGNATURE);
	bool old_writable = set_writable_background_region(true);
    rtos_status = xTimerStart(rtos_timer_p->tmr_os_timer_handle,
	                      portMAX_DELAY);
    (void)set_writable_background_region(old_writable);

    if (rtos_status != pdPASS) {
        error = CAPTURE_ERROR("xTimerStart() failed", rtos_status, rtos_timer_p);
        fatal_error_handler(error);
    }
}


/**
 * Stops an RTOS-level timer
 */
void rtos_timer_stop(struct rtos_timer *rtos_timer_p)
{
	BaseType_t rtos_status;
    error_t error;

	D_ASSERT(rtos_timer_p->tmr_signature == TIMER_SIGNATURE);
	bool old_writable = set_writable_background_region(true);
	rtos_status = xTimerStop(rtos_timer_p->tmr_os_timer_handle,
		                 portMAX_DELAY);
    (void)set_writable_background_region(old_writable);

	if (rtos_status != pdPASS) {
		error = CAPTURE_ERROR("xTimerStop() failed", rtos_status, rtos_timer_p);
		fatal_error_handler(error);
	}
}


/**
 * Check if stack overflow has occurred on the task stack.
 * It updates task-p->tsk_max_stack_entries_used.
 */
void rtos_task_check_stack(struct rtos_task *task_p)
{
    UBaseType_t high_water_mark;

    D_ASSERT(task_p->tsk_created);

    high_water_mark = uxTaskGetStackHighWaterMark(task_p->tsk_handle);
	task_p->tsk_max_stack_entries_used = APP_TASK_STACK_SIZE - high_water_mark;

    if (high_water_mark == 0) {
        /*
         * Stack overflow detected
         */
        ERROR_PRINTF("\n*** Stack overflow detected for task '%s'.\n",
                     task_p->tsk_name_p);
        ERROR_PRINTF("*** Task '%s' killed.\n", task_p->tsk_name_p);
    }
}

/**
 * Return Time since boot in ticks
 */
uint32_t rtos_get_ticks_since_boot(void)
{
    return xTaskGetTickCount();
}


/**
 * Return Time since boot in seconds
 */
uint32_t rtos_get_time_since_boot(void)
{
    return rtos_get_ticks_since_boot() / configTICK_RATE_HZ;
}

/*
 *  Callbacks invoked from FreeRTOS
 */

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
								   StackType_t **ppxIdleTaskStackBuffer,
								   uint32_t *pulIdleTaskStackSize )
{
	static struct rtos_task idle_task;

	*ppxIdleTaskTCBBuffer = &idle_task.tsk_tcb;
	*ppxIdleTaskStackBuffer = idle_task.tsk_stack;
	*pulIdleTaskStackSize =
		sizeof(idle_task.tsk_stack) / sizeof(idle_task.tsk_stack[0]);
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
		                            StackType_t **ppxTimerTaskStackBuffer,
									uint32_t *pulTimerTaskStackSize)
{
	static struct rtos_task timer_task;

	*ppxTimerTaskTCBBuffer = &timer_task.tsk_tcb;
	*ppxTimerTaskStackBuffer = timer_task.tsk_stack;
	*pulTimerTaskStackSize =
		sizeof(timer_task.tsk_stack) / sizeof(timer_task.tsk_stack[0]);

}
