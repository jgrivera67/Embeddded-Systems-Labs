/**
 * @file main.c
 *
 * Lab - Stop watch simulated on the UART - using uCOS-III tasks
 *
 * @author: German Rivera
 */
#include <building-blocks/pin_config.h>
#include <building-blocks/serial_console.h>
#include <building-blocks/runtime_checks.h>
#include <building-blocks/compile_time_checks.h>
#include <building-blocks/rtos_wrapper.h>
#include <stddef.h>
#include <ctype.h>

/**
 * stopwatch buttons polling period in milliseconds
 */
#define POLL_STOPWATCH_BUTTONS_PERIOD_MS	10

/**
 * stopwatch update period in milliseconds
 */
#define UPDATE_STOPWATCH_PERIOD_MS	100

C_ASSERT(UPDATE_STOPWATCH_PERIOD_MS % POLL_STOPWATCH_BUTTONS_PERIOD_MS == 0);

/*
 * Bit masks for updating stopwatch display cells:
 */
#define SECONDS_CHANGED_MASK	0x1
#define MINUTES_CHANGED_MASK	0x2
#define HOURS_CHANGED_MASK		0x4

/**
 * State variables of a stopwatch object
 */
struct stopwatch {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint16_t milliseconds;
	volatile bool running;
};

static struct stopwatch g_stopwatch = {
		.running = false
};

/**
 * Task creation parameters:
 */
struct task_params {
    const char *name;
    rtos_task_priority_t priority;
    void *arg;
};

/**
 * Task instances
 */
static struct rtos_task g_stop_watch_buttons_reader_task;
static struct rtos_task g_stop_watch_updater_task;

static void update_stopwatch_display(uint32_t update_mask)
{
	if (update_mask & HOURS_CHANGED_MASK) {
        console_pos_printf(11, 21, 0, "%02u:%02u:%02u.%u",
                           g_stopwatch.hours,
                           g_stopwatch.minutes,
                           g_stopwatch.seconds,
                           g_stopwatch.milliseconds / 100);
    } else if (update_mask & MINUTES_CHANGED_MASK) {
        console_pos_printf(11, 24, 0, "%02u:%02u.%u",
                           g_stopwatch.minutes,
                           g_stopwatch.seconds,
                           g_stopwatch.milliseconds / 100);
    } else if (update_mask & SECONDS_CHANGED_MASK) {
        console_pos_printf(11, 27, 0, "%02u.%u",
                           g_stopwatch.seconds,
                           g_stopwatch.milliseconds / 100);

    } else {
        console_pos_printf(11, 30, 0, "%u",
                           g_stopwatch.milliseconds / 100);
    }
}

/**
 * Resets the stopwatch
 */
static void reset_stopwatch(void)
{
	g_stopwatch.hours = 0;
    g_stopwatch.minutes = 0;
	g_stopwatch.seconds = 0;
	g_stopwatch.milliseconds = 0;
	update_stopwatch_display(HOURS_CHANGED_MASK | MINUTES_CHANGED_MASK | SECONDS_CHANGED_MASK);
}

/**
 * Initializes the display of the stopwatch
 */
static void init_stopwatch_display(void)
{
	console_draw_box(10, 20, 3, 12, 0);
}

/**
 * Initializes the stopwatch
 */
static void init_stopwatch(void)
{
	init_stopwatch_display();
	reset_stopwatch();
}

/**
 * Reads the stopwatch buttons.
 * For the purpose of this lab, we will be using  serial console input to simulate
 * input from the stop watch buttons. Pressing the 'R' key on the serial console
 * will represent pressing the stopwatch 'restart button'. Pressing the 'S' key
 * will represent pressing the stopwatch 'start/stop button'.
 *
 * @pre: This function is expected to run every 10 milliseconds.
 */
static void read_stopwatch_buttons(void)
{
	int c;

	c = console_getchar_non_blocking();
	c = tolower(c);
	if (c == 's') {
		g_stopwatch.running = !g_stopwatch.running;
	} else if (c == 'r') {
		reset_stopwatch();
		g_stopwatch.running = true;
	}
}

/**
 * Increment the stopwatch counters and updates the stopwatch display accordingly.
 *
 * @pre: This function is expected to run every 100 milliseconds.
 */
static void update_stopwatch(void)
{
	uint32_t update_mask = 0;

	D_ASSERT(g_stopwatch.running);
    g_stopwatch.milliseconds += 100;
    if (g_stopwatch.milliseconds == 1000) {
        g_stopwatch.milliseconds = 0;
        update_mask |= SECONDS_CHANGED_MASK;
        g_stopwatch.seconds ++;
        if (g_stopwatch.seconds == 60) {
            g_stopwatch.seconds = 0;
            update_mask |= MINUTES_CHANGED_MASK;
			g_stopwatch.minutes ++;
			if (g_stopwatch.minutes == 60) {
                g_stopwatch.minutes = 0;
				update_mask |= HOURS_CHANGED_MASK;
				g_stopwatch.hours ++;
				if (g_stopwatch.hours == 100) {
					g_stopwatch.hours = 0;
				}
			}
		}
	}

	update_stopwatch_display(update_mask);
}


/**
 * Task function to read the stop watch buttons periodically
 */
static void stop_watch_buttons_reader_task_func(void *arg)
{
    D_ASSERT(arg == NULL);

    for ( ; ; ) {
    	read_stopwatch_buttons();
        rtos_task_delay(POLL_STOPWATCH_BUTTONS_PERIOD_MS);
    }
}


/**
 * Task function to update the stop watch periodically
 */
static void stop_watch_updater_task_func(void *arg)
{
    D_ASSERT(arg == NULL);

    for ( ; ; ) {
    	if (g_stopwatch.running) {
			update_stopwatch();
    	}

        rtos_task_delay(UPDATE_STOPWATCH_PERIOD_MS);
    }
}


int main(void)
{
    rtos_init();

	/*
	 * Initialize devices used:
	 */
	pin_config_init();
	console_init();

	/*
	 * Display greeting:
	 */
	console_clear();
	console_printf("lab - Stop Watch with FreeRTOS tasks  (built " __DATE__ " " __TIME__ ")\n"
	  		       "Reference solution\n"
	    		   "Buttons: s - start/stop    r - reset\n");

	init_stopwatch();

	/*
	 * Create other tasks:
	 */

	rtos_task_create(&g_stop_watch_buttons_reader_task,
	                 "Stop watch buttons reader task",
	                 stop_watch_buttons_reader_task_func,
	                 NULL,
	                 HIGHEST_APP_TASK_PRIORITY);

	rtos_task_create(&g_stop_watch_updater_task,
	                 "Stop watch updater task",
	                 stop_watch_updater_task_func,
	                 NULL,
	                 LOWEST_APP_TASK_PRIORITY);

    rtos_scheduler_start();

    /*UNREACHABLE*/
    D_ASSERT(false);
    return 1;
}
