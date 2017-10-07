/**
 * @file rtos_wrapper_FreeRTOS.c
 *
 * RTOS wrapper implementation for FreeRTOS APIs
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
#include <stddef.h>

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
    vTaskStartScheduler();

    /*Unreachable*/
    D_ASSERT(false);
}


/**
 * Notify RTOS that we are entering an ISR
 */
void rtos_enter_isr(void)
{
    uint_fast8_t prev_nested_ISR_count = ATOMIC_POST_INCREMENT_UINT8(&g_nested_ISR_count);

    D_ASSERT(prev_nested_ISR_count < UINT8_MAX);
}


/**
 * Notify RTOS that we are exiting an ISR
 */
void rtos_exit_isr(void)
{
    uint_fast8_t prev_nested_ISR_count = ATOMIC_POST_DECREMENT_UINT8(&g_nested_ISR_count);

    D_ASSERT(prev_nested_ISR_count >= 1);
    if (prev_nested_ISR_count == 1) {
        portEND_SWITCHING_ISR(g_rtos_task_context_switch_required);
        g_rtos_task_context_switch_required = pdFALSE;
    }
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
    BaseType_t rtos_status;
    error_t error;

    D_ASSERT(rtos_task_p != NULL);
    D_ASSERT(!rtos_task_p->tsk_created);
    D_ASSERT(task_function_p != NULL);
    D_ASSERT(task_prio >= LOWEST_APP_TASK_PRIORITY &&
             task_prio <= HIGHEST_APP_TASK_PRIORITY);

    rtos_task_p->tsk_signature = TASK_SIGNATURE;
    rtos_task_p->tsk_name_p = task_name_p;
    rtos_task_p->tsk_created = true;
    rtos_task_p->tsk_max_stack_entries_used = 0;

    /*
     * Create the FreeRTOS task:
     */
    TaskParameters_t task_params = {
		.pvTaskCode = task_function_p,
    	.pcName = task_name_p,
		.usStackDepth = APP_TASK_STACK_SIZE,
		.pvParameters = task_arg_p,
    	.uxPriority = task_prio,
    	.puxStackBuffer = rtos_task_p->tsk_stack,
    };

    rtos_status = xTaskCreateRestricted(&task_params,
    		                            &rtos_task_p->tsk_handle);

    if (rtos_status != pdPASS) {
        error = CAPTURE_ERROR("xTaskCreate() failed", rtos_status, rtos_task_p);
        fatal_error_handler(error);
    }

    D_ASSERT(rtos_task_p->tsk_handle != NULL);

    vTaskSetApplicationTaskTag(rtos_task_p->tsk_handle, (void *)rtos_task_p);
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

    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    struct rtos_task *task_p = (void *)xTaskGetApplicationTaskTag(task_handle);

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

    fatal_error_handler(error);
}


/**
 * Wake up a task by signaling its built-in semaphore
 */
void rtos_task_semaphore_signal(struct rtos_task *rtos_task_p)
{
    error_t error = CAPTURE_ERROR("function not supported for FreeRTOS" , rtos_task_semaphore_signal, 0);

    fatal_error_handler(error);
}


/**
 * Initializes an RTOS-level mutex
 */
void rtos_mutex_init(struct rtos_mutex *rtos_mutex_p,
                     const char *mutex_name_p)
{
    D_ASSERT(rtos_mutex_p != NULL);
    rtos_mutex_p->mtx_signature = MUTEX_SIGNATURE;
    rtos_mutex_p->mtx_name = mutex_name_p;

    rtos_mutex_p->mtx_os_mutex = xSemaphoreCreateMutex();
    D_ASSERT(rtos_mutex_p->mtx_os_mutex != NULL);
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

    rtos_status = xSemaphoreTake(rtos_mutex_p->mtx_os_mutex, portMAX_DELAY);
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

    rtos_status = xSemaphoreGive(rtos_mutex_p->mtx_os_mutex);
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

    TaskHandle_t owner = xSemaphoreGetMutexHolder(rtos_mutex_p->mtx_os_mutex);

    return owner == xTaskGetCurrentTaskHandle();
}


/**
 * Initializes an RTOS-level semaphore
 */
void rtos_semaphore_init(struct rtos_semaphore *rtos_semaphore_p,
                         const char *semaphore_name_p,
                         uint32_t initial_count)
{
    uint32_t max_count;

    D_ASSERT(rtos_semaphore_p != NULL);
    rtos_semaphore_p->sem_signature = SEMAPHORE_SIGNATURE;
    rtos_semaphore_p->sem_name = semaphore_name_p;
    if (initial_count == 0) {
        max_count = 1;
    } else {
        max_count = initial_count;
    }

    rtos_semaphore_p->sem_os_semaphore = xSemaphoreCreateCounting(max_count, initial_count);
    D_ASSERT(rtos_semaphore_p->sem_os_semaphore != NULL);
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

    rtos_status = xSemaphoreTake(rtos_semaphore_p->sem_os_semaphore, portMAX_DELAY);
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

    rtos_status = xSemaphoreTake(rtos_semaphore_p->sem_os_semaphore,
                                 timeout_ms / MS_PER_TIMER_TICK);
    return rtos_status == pdPASS;
}


/**
 * Signal an RTOS-level semaphore. It wakes up the highest priority waiter
 */
void rtos_semaphore_signal(struct rtos_semaphore *rtos_semaphore_p)
{
    BaseType_t rtos_status;
    error_t error;

    D_ASSERT(rtos_semaphore_p->sem_signature == SEMAPHORE_SIGNATURE);
    if (CALLER_IS_THREAD()) {
        rtos_status = xSemaphoreGive(rtos_semaphore_p->sem_os_semaphore);
    } else {
        rtos_status = xSemaphoreGiveFromISR(rtos_semaphore_p->sem_os_semaphore,
                                            &g_rtos_task_context_switch_required);
    }

    if (rtos_status != pdPASS) {
        error = CAPTURE_ERROR("xSemaphoreGive() failed", rtos_status, rtos_semaphore_p);
        fatal_error_handler(error);
    }
}


/**
 * Broadcast an RTOS-level semaphore. It wakes up all waiters
 */
void rtos_semaphore_broadcast(struct rtos_semaphore *rtos_semaphore_p)
{
    error_t error = CAPTURE_ERROR("function not supported for FreeRTOS", rtos_semaphore_broadcast, 0);

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

    rtos_timer_p->tmr_os_timer = xTimerCreate(timer_name_p,
                                              ticks,
                                              periodic,
                                              rtos_timer_p,
                                              rtos_timer_internal_callback);
    D_ASSERT(rtos_timer_p->tmr_os_timer != NULL);
}


/**
 * Starts an RTOS-level timer
 */
void rtos_timer_start(struct rtos_timer *rtos_timer_p)
{
    BaseType_t rtos_status;
    error_t error;

    D_ASSERT(rtos_timer_p->tmr_signature == TIMER_SIGNATURE);
    rtos_status = xTimerStart(rtos_timer_p->tmr_os_timer, portMAX_DELAY);
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
	rtos_status = xTimerStop(rtos_timer_p->tmr_os_timer, portMAX_DELAY);
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
