/**
 * @file main.c
 *
 * Lab - Stop watch simulated on the UART - using uCOS-III tasks
 *
 * @author: German Rivera
 */
#include <building-blocks/pin_config.h>
#include <building-blocks/gpio_driver.h>
#include <building-blocks/serial_console.h>
#include <building-blocks/runtime_checks.h>
#include <building-blocks/compile_time_checks.h>
#include <building-blocks/io_utils.h>
#include <building-blocks/rtos_wrapper.h>
#include <building-blocks/memory_protection_unit.h>
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
#define HOURS_CHANGED_MASK	0x4

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
static struct rtos_task g_console_task;

static void update_stopwatch_display(uint32_t update_mask)
{
	console_lock();
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

	console_unlock();
}

/**
 * Resets the stopwatch
 */
static void reset_stopwatch(void)
{
	struct mpu_region_descriptor old_region;

    set_private_data_region(&g_stopwatch, sizeof(g_stopwatch), READ_WRITE, &old_region);
	g_stopwatch.hours = 0;
    g_stopwatch.minutes = 0;
	g_stopwatch.seconds = 0;
	g_stopwatch.milliseconds = 0;
    restore_private_data_region(&old_region);
}

/**
 * Initializes the display of the stopwatch
 */
static void init_stopwatch_display(void)
{
	console_draw_box(10, 20, 3, 12, 0);
    console_pos_printf(11, 21, 0, "00:00:00.0");
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
	struct mpu_region_descriptor old_region;

	c = console_getchar_non_blocking();
	c = tolower(c);
    set_private_data_region(&g_stopwatch, sizeof(g_stopwatch), READ_WRITE, &old_region);
	if (c == 's') {
		g_stopwatch.running = !g_stopwatch.running;
	} else if (c == 'r') {
		reset_stopwatch();
	    update_stopwatch_display(HOURS_CHANGED_MASK | MINUTES_CHANGED_MASK | SECONDS_CHANGED_MASK);
		g_stopwatch.running = true;
	}
    restore_private_data_region(&old_region);
}

/**
 * Increment the stopwatch counters and updates the stopwatch display accordingly.
 *
 * @pre: This function is expected to run every 100 milliseconds.
 */
static void update_stopwatch(void)
{
	uint32_t update_mask = 0;
	struct mpu_region_descriptor old_region;

	D_ASSERT(g_stopwatch.running);
    set_private_data_region(&g_stopwatch, sizeof(g_stopwatch), READ_WRITE, &old_region);
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

    restore_private_data_region(&old_region);
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


static struct gpio_pin g_led_pin =
	GPIO_PIN_INITIALIZER(PIN_PORT_A, 5, PIN_MODE_GPIO_OUTPUT,
		             PIN_ALTERNATE_FUNCTION_NONE, true);

static void init_led(void) {
    gpio_configure_pin(&g_led_pin, 0x0);

    gpio_activate_output_pin(&g_led_pin);
}

int main(void)
{
    mpu_init();
    rtos_init();

	/*
	 * Initialize devices used:
	 */
    //mpu_enable();
    pin_config_init();
    init_led();//???

    gpio_deactivate_output_pin(&g_led_pin); //???
    for (unsigned int i = 0; i < 100000; i++)
        ;
    gpio_activate_output_pin(&g_led_pin); //???
    console_init(&g_console_task);
    gpio_deactivate_output_pin(&g_led_pin); //???

    /*
     * Display greeting:
     */
    console_clear();
    console_printf("Stop Watch with FreeRTOS tasks  (built " __DATE__ " " __TIME__ ")\n"
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

