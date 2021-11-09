/**
 * @file main.c
 *
 * Ethernet PHY Lab
 *
 * @author: German Rivera
 */
#include <building-blocks/serial_console.h>
#include <building-blocks/color_led.h>
#include <building-blocks/runtime_checks.h>
#include <building-blocks/time_utils.h>
#include <building-blocks/rtos_wrapper.h>
#include <building-blocks/command_line.h>
#include <building-blocks/atomic_utils.h>
#include <building-blocks/mem_utils.h>
#include <building-blocks/microcontroller.h>
#include <building-blocks/pin_config.h>
#include <building-blocks/cpu_reset_counter.h>
#include <building-blocks/cortex_m_startup.h>
#include <building-blocks/watchdog.h>
#include <building-blocks/nor_flash_driver.h>
#include <building-blocks/runtime_log.h>
#include <building-blocks/networking.h>
#include <building-blocks/networking_layer2.h>
#include <building-blocks/networking_layer3.h>
#include <building-blocks/networking_layer4.h>
#include <board.h>
#include <string.h>
#include <fsl_clock_manager.h>

/**
 * Stacks checker task period in milliseconds
 */
#define STACKS_CHECKING_PERIOD_MS    50

/**
 * Heartbeat timer period in milliseconds
 */
#define HEARTBEAT_PERIOD_MS        500

/**
 * Ethernet link status polling period in milliseconds
 */
#define NETWORK_STATS_POLLING_PERIOD_MS    250

/**
 * Task creation parameters:
 */
struct task_params {
    const char *name;
    rtos_task_priority_t priority;
    void *arg;
};

/**
 * Default local IPv4 address
 */
static const struct ipv4_address g_local_ipv4_addr = {
    .bytes = { 192, 168, 8, 2 }
};

/**
 * Task instances
 */
static struct rtos_task g_main_task;
static struct rtos_task g_network_stats_task;
static struct rtos_task g_stacks_checker_task;
static struct rtos_task g_console_output_task;
static struct rtos_task g_udp_server_task;

/**
 * Array of pointers to all tasks
 */
static struct rtos_task *g_all_app_tasks[] = {
    &g_main_task,
    &g_network_stats_task,
    &g_stacks_checker_task,
    &g_console_output_task,
    &g_udp_server_task,
};

#define NUM_APP_TASKS    (sizeof(g_all_app_tasks) / sizeof(g_all_app_tasks[0]))

/**
 * Periodic timer to toggle the heartbeat LED
 */
static struct rtos_timer g_heartbeat_timer;

/**
 * Heartbeat LED color
 */
static volatile led_color_t g_heartbeat_led_color = LED_COLOR_BLUE;


/**
 * Initializes the display of the network stats display
 */
static void init_network_stats_display(void)
{
    console_pos_printf(5, 1, 0, "Ethernet link");
    console_draw_box(4, 14, 3, 6, 0);
    console_pos_printf(5, 21, 0, "Ethernet MAC address");
    console_draw_box(4, 41, 3, 19, 0);
    console_pos_printf(8, 1, 0, "IPv4 address");
    console_draw_box(7, 13, 3, 17, 0);
    console_pos_printf(8, 31, 0, "IPv4 subnet mask");
    console_draw_box(7, 47, 3, 17, 0);

    console_pos_printf(11, 1, 0, "Received packets accepted at layer 2 - Enet");
    console_draw_box(10, 44, 3, 12, 0);
    console_pos_printf(11, 57, 0, "Received packets dropped at layer 2 - Enet");
    console_draw_box(10, 100, 3, 12, 0);
    console_pos_printf(11, 113, 0, "Sent packets at layer 2 - Enet");
    console_draw_box(10, 143, 3, 12, 0);

    console_pos_printf(14, 1, 0, "Received packets accepted at layer 3 - IPv4");
    console_draw_box(13, 44, 3, 12, 0);
    console_pos_printf(14, 57, 0, "Received packets dropped at layer 3 - IPv4");
    console_draw_box(13, 100, 3, 12, 0);
    console_pos_printf(14, 113, 0, "Sent packets at layer 3 - IPv4");
    console_draw_box(13, 143, 3, 12, 0);

    console_pos_printf(17, 1, 0, "Received packets accepted at layer 4 - UDP ");
    console_draw_box(16, 44, 3, 12, 0);
    console_pos_printf(17, 57, 0, "Received packets dropped at layer 4 - UDP ");
    console_draw_box(16, 100, 3, 12, 0);
    console_pos_printf(17, 113, 0, "Sent packets at layer 4 - UDP ");
    console_draw_box(16, 143, 3, 12, 0);

    console_pos_printf(20, 1, 0, "Last UDP message received");
    console_draw_box(19, 26, 3, 82, 0);
}


static void heartbeat_set_led_color(led_color_t color)
{
    uint32_t old_primask = disable_cpu_interrupts();

    g_heartbeat_led_color = color;
    color_led_set(color);

    restore_cpu_interrupts(old_primask);
}


static struct net_layer4_end_point g_udp_server_end_point;

static void udp_server_task_func(void *arg)
{
#   define MY_UDP_SERVER_PORT    8887

    error_t error;
    uint32_t in_msg_count = 0;
    uint_fast8_t byte = 'A';

    D_ASSERT(arg == NULL);

    struct network_packet *tx_packet_p = net_layer2_allocate_tx_packet(false);

    net_layer4_udp_end_point_init(&g_udp_server_end_point);

    error = net_layer4_udp_end_point_bind(&g_udp_server_end_point,
                                          hton16(MY_UDP_SERVER_PORT));
    if (error != 0) {
        console_printf("ERROR: binding UDP server to port %u failed (error %#x)\n",
                       MY_UDP_SERVER_PORT, error);
        goto exit;
    }

    for ( ; ; ) {
        struct network_packet *rx_packet_p = NULL;
        struct ipv4_address client_ip_addr;
        uint16_t client_port;
        const uint8_t *in_msg_p;
        size_t in_msg_size;
        uint8_t *out_msg_p;
        uint_fast8_t tmp_byte;

        error = net_layer4_receive_udp_datagram_over_ipv4(&g_udp_server_end_point,
                                                          0,
                                                          &client_ip_addr,
                                                          &client_port,
                                                          &rx_packet_p);
        if (error != 0) {
            console_printf("ERROR: receiving UDP datagram failed (error %#x)\n",
                           error);
            goto exit;
        }

        in_msg_count ++;
        in_msg_size = get_ipv4_udp_data_payload_length(rx_packet_p);
        in_msg_p = get_ipv4_udp_data_payload_area(rx_packet_p);

        out_msg_p = get_ipv4_udp_data_payload_area(tx_packet_p);
        memcpy(out_msg_p, in_msg_p, in_msg_size);

        /*
         * We don't need the received packet anymore, as we have
         * copied its content to a Tx packet:
         */
        net_recycle_rx_packet(rx_packet_p);

        if (in_msg_size > 80) {
            /*
             * Truncate message text, to print the first 80 characters
             */
            tmp_byte = out_msg_p[80];
            out_msg_p[80] = '\0';
        } else {
            /*
             * Just add the null terminator at the end
             */
            out_msg_p[in_msg_size] = '\0';

            /*
             * Process command in the message, if any:
             */
            const char *cmd_s = (char *)out_msg_p;

            if (strcmp(cmd_s, "red") == 0) {
                heartbeat_set_led_color(LED_COLOR_RED);
            } else if (strcmp(cmd_s, "green") == 0) {
                heartbeat_set_led_color(LED_COLOR_GREEN);
            } else if (strcmp(cmd_s, "blue") == 0) {
                heartbeat_set_led_color(LED_COLOR_BLUE);
            } else if (strcmp(cmd_s, "yellow") == 0) {
                heartbeat_set_led_color(LED_COLOR_YELLOW);
            } else if (strcmp(cmd_s, "cyan") == 0) {
                heartbeat_set_led_color(LED_COLOR_CYAN);
            } else if (strcmp(cmd_s, "magenta") == 0) {
                heartbeat_set_led_color(LED_COLOR_MAGENTA);
            } else if (strcmp(cmd_s, "white") == 0) {
                heartbeat_set_led_color(LED_COLOR_WHITE);
            }
        }

        console_lock();
        console_pos_printf(20, 27, 0, "%-80s", out_msg_p);
        console_unlock();

        /*
         * Untruncate message text, to transmit it back to the sender:
         */
        if (in_msg_size > 80) {
            out_msg_p[80] = tmp_byte;
        }

        /*
         * Append a letter at the end of the message text before sending it:
         */
        out_msg_p[in_msg_size] = byte;
        if (byte < 'Z') {
            byte ++;
        } else {
            byte = 'A';
        }

        /*
         * Send received message back to UDP client:
         */
        error = net_layer4_send_udp_datagram_over_ipv4(&g_udp_server_end_point,
                                                       &client_ip_addr,
                                                       client_port,
                                                       tx_packet_p,
                                                       in_msg_size + 1);
        if (error != 0) {
            console_printf("ERROR: sending UDP datagram failed (error %#x)\n",
                           error);
            goto exit;
        }
    }

exit:
    net_layer2_free_tx_packet(tx_packet_p);
    console_printf("Task %s terminated\n", rtos_task_self()->tsk_name_p);
}


static void stats_update_link_state(bool *link_is_up_p)
{
    led_color_t led_color;
    const char *link_state_s;
    bool new_link_is_up =
        net_layer2_end_point_link_is_up(&g_net_layer2.local_layer2_end_points[0]);

    if (*link_is_up_p != new_link_is_up) {
        *link_is_up_p = new_link_is_up;
        if (new_link_is_up) {
            link_state_s = "up  ";
            led_color = LED_COLOR_GREEN;
        } else {
            link_state_s = "down";
            led_color = LED_COLOR_RED;
        }

        console_pos_puts(5, 15, 0, link_state_s);
        heartbeat_set_led_color(led_color);
    }
}


static void stats_update_ipv4_addr(
    struct ipv4_address *ipv4_addr_p,
    struct ipv4_address *subnet_mask_p)
{
    struct ipv4_address new_ipv4_addr;
    struct ipv4_address new_subnet_mask;

    net_layer3_get_local_ipv4_address(&new_ipv4_addr, &new_subnet_mask);
    if (ipv4_addr_p->value != new_ipv4_addr.value ||
        subnet_mask_p->value != new_subnet_mask.value) {
        ipv4_addr_p->value = new_ipv4_addr.value;
        subnet_mask_p->value = new_subnet_mask.value;

        console_pos_printf(8, 14, 0, "%u.%u.%u.%u\n",
                           new_ipv4_addr.bytes[0],
                           new_ipv4_addr.bytes[1],
                           new_ipv4_addr.bytes[2],
                           new_ipv4_addr.bytes[3]);

        console_pos_printf(8, 48, 0, "%u.%u.%u.%u\n",
                           new_subnet_mask.bytes[0],
                           new_subnet_mask.bytes[1],
                           new_subnet_mask.bytes[2],
                           new_subnet_mask.bytes[3]);
    }
}


static void stats_update_layer2_packet_count(
    uint32_t *rx_packet_accepted_count_p,
    uint32_t *rx_packet_dropped_count_p,
    uint32_t *tx_packet_count_p)
{
    uint32_t new_rx_packet_accepted_count = g_net_layer2.rx_packets_accepted_count;
    uint32_t new_rx_packet_dropped_count = g_net_layer2.rx_packets_dropped_count;
    uint32_t new_tx_packet_count = g_net_layer2.sent_packets_count;

    if (*rx_packet_accepted_count_p != new_rx_packet_accepted_count) {
        *rx_packet_accepted_count_p = new_rx_packet_accepted_count;
        console_pos_printf(11, 45, 0, "%10u", new_rx_packet_accepted_count);
    }

    if (*rx_packet_dropped_count_p != new_rx_packet_dropped_count) {
        *rx_packet_dropped_count_p = new_rx_packet_dropped_count;
        console_pos_printf(11, 101, 0, "%10u", new_rx_packet_dropped_count);
    }

    if (*tx_packet_count_p != new_tx_packet_count) {
        *tx_packet_count_p = new_tx_packet_count;
        console_pos_printf(11, 144, 0, "%10u", new_tx_packet_count);
    }
}


static void stats_update_layer3_ipv4_packet_count(
    uint32_t *rx_packet_accepted_count_p,
    uint32_t *rx_packet_dropped_count_p,
    uint32_t *tx_packet_count_p)
{
    uint32_t new_rx_packet_accepted_count = g_net_layer3.ipv4.rx_packets_accepted_count;
    uint32_t new_rx_packet_dropped_count = g_net_layer3.ipv4.rx_packets_dropped_count;
    uint32_t new_tx_packet_count = g_net_layer3.ipv4.sent_packets_count;

    if (*rx_packet_accepted_count_p != new_rx_packet_accepted_count) {
        *rx_packet_accepted_count_p = new_rx_packet_accepted_count;
        console_pos_printf(14, 45, 0, "%10u", new_rx_packet_accepted_count);
    }

    if (*rx_packet_dropped_count_p != new_rx_packet_dropped_count) {
        *rx_packet_dropped_count_p = new_rx_packet_dropped_count;
        console_pos_printf(14, 101, 0, "%10u", new_rx_packet_dropped_count);
    }

    if (*tx_packet_count_p != new_tx_packet_count) {
        *tx_packet_count_p = new_tx_packet_count;
        console_pos_printf(14, 144, 0, "%10u", new_tx_packet_count);
    }
}


static void stats_update_layer4_udp_packet_count(
    uint32_t *rx_packet_accepted_count_p,
    uint32_t *rx_packet_dropped_count_p,
    uint32_t *tx_packet_count_p)
{
    uint32_t new_rx_packet_accepted_count = g_net_layer4.udp.rx_packets_accepted_count;
    uint32_t new_rx_packet_dropped_count = g_net_layer4.udp.rx_packets_dropped_count;
    uint32_t new_tx_packet_count = g_net_layer4.udp.sent_packets_over_ipv4_count;

    if (*rx_packet_accepted_count_p != new_rx_packet_accepted_count) {
        *rx_packet_accepted_count_p = new_rx_packet_accepted_count;
        console_pos_printf(17, 45, 0, "%10u", new_rx_packet_accepted_count);
    }

    if (*rx_packet_dropped_count_p != new_rx_packet_dropped_count) {
        *rx_packet_dropped_count_p = new_rx_packet_dropped_count;
        console_pos_printf(17, 101, 0, "%10u", new_rx_packet_dropped_count);
    }

    if (*tx_packet_count_p != new_tx_packet_count) {
        *tx_packet_count_p = new_tx_packet_count;
        console_pos_printf(17, 144, 0, "%10u", new_tx_packet_count);
    }
}


static void network_stats_task_func(void *arg)
{
    bool link_is_up = false;
    struct ethernet_mac_address local_mac_addr;
    struct ipv4_address ipv4_addr;
    struct ipv4_address subnet_mask;

    D_ASSERT(arg == NULL);

    console_lock();
    init_network_stats_display();
    console_pos_puts(5, 15, 0, "down");
    heartbeat_set_led_color(LED_COLOR_RED);

    net_layer2_get_mac_addr(&g_net_layer2.local_layer2_end_points[0], &local_mac_addr);
    console_pos_printf(5, 42, 0, "%02x:%02x:%02x:%02x:%02x:%02x\n",
                       local_mac_addr.bytes[0],
                       local_mac_addr.bytes[1],
                       local_mac_addr.bytes[2],
                       local_mac_addr.bytes[3],
                       local_mac_addr.bytes[4],
                       local_mac_addr.bytes[5]);

    net_layer3_get_local_ipv4_address(&ipv4_addr, &subnet_mask);
    console_pos_printf(8, 14, 0, "%u.%u.%u.%u\n",
                       ipv4_addr.bytes[0],
                       ipv4_addr.bytes[1],
                       ipv4_addr.bytes[2],
                       ipv4_addr.bytes[3]);

    console_pos_printf(8, 48, 0, "%u.%u.%u.%u\n",
                       subnet_mask.bytes[0],
                       subnet_mask.bytes[1],
                       subnet_mask.bytes[2],
                       subnet_mask.bytes[3]);
    console_unlock();

    uint32_t layer2_rx_packet_accepted_count = 0;
    uint32_t layer2_rx_packet_dropped_count = 0;
    uint32_t layer2_tx_packet_count = 0;
    uint32_t ipv4_rx_packet_accepted_count = 0;
    uint32_t ipv4_rx_packet_dropped_count = 0;
    uint32_t ipv4_tx_packet_count = 0;
    uint32_t udp_rx_packet_accepted_count = 0;
    uint32_t udp_rx_packet_dropped_count = 0;
    uint32_t udp_tx_packet_count = 0;

    for ( ; ; ) {
        console_lock();
        stats_update_link_state(&link_is_up);
        stats_update_ipv4_addr(&ipv4_addr, &subnet_mask);
        stats_update_layer2_packet_count(&layer2_rx_packet_accepted_count,
                                         &layer2_rx_packet_dropped_count,
                                         &layer2_tx_packet_count);
        stats_update_layer3_ipv4_packet_count(&ipv4_rx_packet_accepted_count,
                                              &ipv4_rx_packet_dropped_count,
                                              &ipv4_tx_packet_count);
        stats_update_layer4_udp_packet_count(&udp_rx_packet_accepted_count,
                                             &udp_rx_packet_dropped_count,
                                             &udp_tx_packet_count);
        console_unlock();

        rtos_task_delay(NETWORK_STATS_POLLING_PERIOD_MS);
    }
}


static void cmd_print_help(void)
{
    static const char help_msg[] =
        "Available commands are:\n"
        "\thang - Cause an artificial hang\n"
        "\treset - Reset microcontroller\n"
        "\tstats (or st) - prints stats\n"
        "\tlog <log name: info, error, debug> - Dumps the given runtime log\n"
        "\tset ip4 addr <IPv4 address>/<subnet prefix>\n"
        "\tset trace <net, layer2, layer3 or layer4> <on or off>\n"
        "\tset loopback <on or off>\n"
        "\tget ip4 addr\n"
        "\tping <IPv4 address>\n"
        "\thelp (or h) - prints this message\n";

    D_ASSERT(console_is_locked());
    console_puts(help_msg);
}


/**
 * Cause artificial hang
 */
static void cmd_hang(void)
{
    (void)disable_cpu_interrupts();

    for ( ; ; )
        ;
}


/**
 * Reset microcontroller
 */
static void cmd_reset(void)
{
    reset_cpu();
}


/**
 * Prints stats
 */
static void cmd_print_stats(void)
{
    static const char *const reset_cause_strings[] = {
        [INVALID_RESET_CAUSE] = "Invalid reset",
        [POWER_ON_RESET] =  "Power-on reset",
        [EXTERNAL_PIN_RESET] =  "External pin reset",
        [WATCHDOG_RESET] =  "Watchdog reset",
        [SOFTWARE_RESET] =  "Software reset",
        [LOCKUP_EVENT_RESET] =  "Lockup reset",
        [EXTERNAL_DEBUGGER_RESET] =  "External debugger reset",
        [OTHER_HW_REASON_RESET] =  "Other hardware-reason reset",
        [STOP_ACK_ERROR_RESET] =  "Stop ack error reset",
    };
    uint32_t max_time_us;
    uintptr_t code_addr;
    enum cpu_reset_causes reset_cause;
    struct ethernet_mac_address local_mac_addr;
    struct ipv4_address local_ipv4_addr;
    struct ipv4_address ipv4_subnet_mask;

    D_ASSERT(console_is_locked());
    console_printf("Elapsed time since last boot: %u s\n", rtos_get_time_since_boot());
    console_printf("Startup time: %u us\n", get_starup_time_us());
    get_max_interrupts_disabled_stats_us(&max_time_us, &code_addr);
    console_printf("Maximum interrupts disabled time: %u us (in code near address %#x)\n",
                   max_time_us, code_addr);
    console_printf("Reset count: %u\n", read_cpu_reset_counter());
    reset_cause = find_cpu_reset_cause();
    D_ASSERT(reset_cause != INVALID_RESET_CAUSE &&
             reset_cause < ARRAY_SIZE(reset_cause_strings));

    console_printf("Last reset cause: %s\n", reset_cause_strings[reset_cause]);
    if (reset_cause == WATCHDOG_RESET) {
        uint32_t old_expected_liveness_events;
        uint32_t old_signaled_liveness_events;

        watchdog_get_before_reset_info(&old_expected_liveness_events,
                                       &old_signaled_liveness_events);

        console_printf("                  Expected liveness events mask: %#x\n"
                       "                  Signaled liveness events mask: %#x\n",
                       old_expected_liveness_events,
                       old_signaled_liveness_events);
    }

    console_printf("Flash used: %u bytes\n", get_flash_used());
    console_printf("SRAM used: %u bytes\n", get_sram_used());
    console_printf("CPU core clock frequency: %u MHz\n", CLOCK_SYS_GetCoreClockFreq() / 1000000u);
    console_printf("System clock frequency: %u MHz\n", CLOCK_SYS_GetSystemClockFreq() / 1000000u);
    console_printf("Bus clock frequency: %u MHz\n", CLOCK_SYS_GetBusClockFreq() / 1000000u);

    net_layer2_get_mac_addr(&g_net_layer2.local_layer2_end_points[0], &local_mac_addr);
    console_printf("Local Ethernet MAC address: %x:%x:%x:%x:%x:%x\n",
                   local_mac_addr.bytes[0],
                   local_mac_addr.bytes[1],
                   local_mac_addr.bytes[2],
                   local_mac_addr.bytes[3],
                   local_mac_addr.bytes[4],
                   local_mac_addr.bytes[5]);

    net_layer3_get_local_ipv4_address(&local_ipv4_addr, &ipv4_subnet_mask);
    console_printf("Local IPv4 address: %u.%u.%u.%u (subnet mask: %u.%u.%u.%u)\n",
                   local_ipv4_addr.bytes[0],
                   local_ipv4_addr.bytes[1],
                   local_ipv4_addr.bytes[2],
                   local_ipv4_addr.bytes[3],
                   ipv4_subnet_mask.bytes[0],
                   ipv4_subnet_mask.bytes[1],
                   ipv4_subnet_mask.bytes[2],
                   ipv4_subnet_mask.bytes[3]);

    bool ethernet_link =
        net_layer2_end_point_link_is_up(&g_net_layer2.local_layer2_end_points[0]);

    console_printf("Ethernet link state: %s\n", ethernet_link ? "up" : "down");

    console_puts("\nTask                                 Max stack entries used\n"
                   "===========================================================\n");

    for (uint8_t i = 0; i < NUM_APP_TASKS; i++) {
        struct rtos_task *task_p = g_all_app_tasks[i];

        if (task_p->tsk_created) {
            console_printf("%-35s  %u\n", task_p->tsk_name_p,
                           task_p->tsk_max_stack_entries_used);
        }
    }
}


static void cmd_dump_log(int argc, const char *argv[])
{
    if (argc != 1) {
        console_printf("Invalid syntax for command 'log'\n");
        return;
    }

    if (strcmp(argv[0], "debug") == 0) {
        runtime_log_dump(RUNTIME_DEBUG_LOG);
    } else if (strcmp(argv[0], "error") == 0) {
        runtime_log_dump(RUNTIME_ERROR_LOG);
    } else if (strcmp(argv[0], "info") == 0) {
        runtime_log_dump(RUNTIME_INFO_LOG);
    } else {
        console_printf("The log '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_set_ip4_addr(int argc, const char *argv[])
{
    if (argc != 1) {
        console_printf("Invalid syntax for command 'set ip4 addr'\n");
        return;
    }

    struct ipv4_address ipv4_addr;
    uint8_t subnet_prefix;

    if (!net_layer3_parse_ipv4_addr(argv[0], &ipv4_addr, &subnet_prefix)) {
        console_printf("Invalid syntax for IPv4 address with subnet prefix: '%s'\n",
                       argv[0]);
        return;
    }

    net_layer3_set_local_ipv4_address(&ipv4_addr, subnet_prefix);
}


static void cmd_set_ip4(int argc, const char *argv[])
{
    if (argc < 1) {
        console_printf("Invalid syntax for command 'set ip4'\n");
        return;
    }

    if (strcmp(argv[0], "addr") == 0) {
        cmd_set_ip4_addr(argc - 1, argv + 1);
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_trace_layer2(int argc, const char *argv[])
{
    if (argc != 1) {
        console_printf("Invalid syntax for command 'trace layer2'\n");
        return;
    }

    if (strcmp(argv[0], "on") == 0) {
        net_layer2_start_tracing();
    } else if (strcmp(argv[0], "off") == 0) {
        net_layer2_stop_tracing();
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_trace_layer3(int argc, const char *argv[])
{
    if (argc != 1) {
        console_printf("Invalid syntax for command 'trace layer3'\n");
        return;
    }

    if (strcmp(argv[0], "on") == 0) {
        net_layer3_start_tracing();
    } else if (strcmp(argv[0], "off") == 0) {
        net_layer3_stop_tracing();
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_trace_layer4(int argc, const char *argv[])
{
    if (argc != 1) {
        console_printf("Invalid syntax for command 'trace layer4'\n");
        return;
    }

    if (strcmp(argv[0], "on") == 0) {
        net_layer4_start_tracing();
    } else if (strcmp(argv[0], "off") == 0) {
        net_layer4_stop_tracing();
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_trace_net(int argc, const char *argv[])
{
    if (argc != 1) {
        console_printf("Invalid syntax for command 'trace net'\n");
        return;
    }

    if (strcmp(argv[0], "on") == 0) {
        net_layer2_start_tracing();
        net_layer3_start_tracing();
        net_layer4_start_tracing();
    } else if (strcmp(argv[0], "off") == 0) {
        net_layer2_stop_tracing();
        net_layer3_stop_tracing();
        net_layer4_stop_tracing();
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_trace(int argc, const char *argv[])
{
    if (argc < 1) {
        console_printf("Invalid syntax for command 'trace'\n");
        return;
    }

    if (strcmp(argv[0], "layer2") == 0) {
        cmd_trace_layer2(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "layer3") == 0) {
        cmd_trace_layer3(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "layer4") == 0) {
        cmd_trace_layer4(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "net") == 0) {
        cmd_trace_net(argc - 1, argv + 1);
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_loopback(int argc, const char *argv[])
{
    if (argc != 1) {
        console_printf("Invalid syntax for command 'loopback'\n");
        return;
    }

    if (strcmp(argv[0], "on") == 0) {
        net_layer2_end_point_set_loopback(&g_net_layer2.local_layer2_end_points[0],
                                          true);
    } else if (strcmp(argv[0], "off") == 0) {
        net_layer2_end_point_set_loopback(&g_net_layer2.local_layer2_end_points[0],
                                          false);
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_set(int argc, const char *argv[])
{
    if (argc < 1) {
        console_printf("Invalid syntax for command 'set'\n");
        return;
    }

    if (strcmp(argv[0], "ip4") == 0) {
        cmd_set_ip4(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "trace") == 0) {
        cmd_trace(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "loopback") == 0) {
        cmd_loopback(argc - 1, argv + 1);
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_get_ip4_addr(void)
{
    struct ipv4_address ipv4_addr;
    struct ipv4_address subnet_mask;

    net_layer3_get_local_ipv4_address(&ipv4_addr, &subnet_mask);
    console_printf("Local IPv4 address %u.%u.%u.%u\n",
                   ipv4_addr.bytes[0],
                   ipv4_addr.bytes[1],
                   ipv4_addr.bytes[2],
                   ipv4_addr.bytes[3]);

    console_printf("Subnet mask %u.%u.%u.%u\n",
                   subnet_mask.bytes[0],
                   subnet_mask.bytes[1],
                   subnet_mask.bytes[2],
                   subnet_mask.bytes[3]);
}


static void cmd_get_ip4(int argc, const char *argv[])
{
    if (argc != 1) {
        console_printf("Invalid syntax for command 'get ip4'\n");
        return;
    }

    if (strcmp(argv[0], "addr") == 0) {
        cmd_get_ip4_addr();
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


static void cmd_get(int argc, const char *argv[])
{
    if (argc < 1) {
        console_printf("Invalid syntax for command 'get'\n");
        return;
    }

    if (strcmp(argv[0], "ip4") == 0) {
        cmd_get_ip4(argc - 1, argv + 1);
    } else {
        console_printf("Subcommand '%s' is not recognized\n", argv[0]);
    }
}


/**
 * IPv4 ping command
 */
static void cmd_ping(int argc, const char *argv[])
{
    error_t error;
    struct ipv4_address dest_ip_addr;
    struct ipv4_address remote_ip_addr;
    uint16_t req_seq_num = 0;
    uint16_t reply_seq_num;
    uint16_t reply_identifier;
    struct rtos_task *calling_task_p = rtos_task_self();
    uint16_t identifier = (uintptr_t)calling_task_p;

    if (argc != 1) {
         console_printf("Invalid syntax for command 'ping'\n");
         return;
     }

    if (!net_layer3_parse_ipv4_addr(argv[0], &dest_ip_addr, NULL)) {
        console_printf("Invalid syntax for IPv4 address: '%s'\n", argv[0]);
        return;
    }

    for (uint_fast8_t i = 0; i < 8; i ++) {
        error = net_layer3_send_ipv4_ping_request(&dest_ip_addr, identifier,
                                                  req_seq_num);
        if (error != 0) {
            console_printf("sending ping request failed with error %#x\n",
                           error);
            return;
        }

        error = net_layer3_receive_ipv4_ping_reply(3000,
                                                   &remote_ip_addr,
                                                   &reply_identifier,
                                                   &reply_seq_num);

        if (error != 0) {
            console_printf("Ping %d for %u.%u.%u.%u timed-out\n",
                           req_seq_num,
                           dest_ip_addr.bytes[0],
                           dest_ip_addr.bytes[1],
                           dest_ip_addr.bytes[2],
                           dest_ip_addr.bytes[3]);
            return;
        }

        D_ASSERT(remote_ip_addr.value == dest_ip_addr.value);
        D_ASSERT(reply_identifier == identifier);
        D_ASSERT(reply_seq_num == req_seq_num);

        console_printf("Ping %d replied by %u.%u.%u.%u\n",
                       reply_seq_num,
                       remote_ip_addr.bytes[0],
                       remote_ip_addr.bytes[1],
                       remote_ip_addr.bytes[2],
                       remote_ip_addr.bytes[3]);

        req_seq_num ++;
        rtos_task_delay(500);
    }
}



static void command_parser(int argc, const char *argv[])
{
    if (argc == 0) {
        return;
    }

    console_putchar('\n');
    if (strcmp(argv[0], "help") == 0 ||
        strcmp(argv[0], "h") == 0) {
        cmd_print_help();
    } else if (strcmp(argv[0], "hang") == 0) {
        cmd_hang();
    } else if (strcmp(argv[0], "reset") == 0) {
        cmd_reset();
    } else if (strcmp(argv[0], "stats") == 0 ||
               strcmp(argv[0], "st") == 0) {
        cmd_print_stats();
    } else if (strcmp(argv[0], "log") == 0) {
        cmd_dump_log(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "set") == 0) {
        cmd_set(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "get") == 0) {
        cmd_get(argc - 1, argv + 1);
    } else if (strcmp(argv[0], "ping") == 0) {
        cmd_ping(argc - 1, argv + 1);
    } else {
        console_printf("The command '%s' is not recognized\n",
                       argv[0]);
    }
}


/**
 * Low priority task that checks the stacks of all tasks continuously.
 */
static void stacks_checker_task_func(void *arg)
{
    D_ASSERT(arg == NULL);

    for ( ; ; ) {
        for (uint8_t i = 0; i < NUM_APP_TASKS; i++) {
            struct rtos_task *task_p = g_all_app_tasks[i];

            if (task_p->tsk_created) {
                rtos_task_check_stack(task_p);
            }
        }

        rtos_task_delay(STACKS_CHECKING_PERIOD_MS);
    }
}


/**
 * Hearbeat timer callback. It toggles the heartbeat LED
 */
static void heartbeat_timer_callback(struct rtos_timer *timer_p, void *arg)
{
    D_ASSERT(timer_p->tmr_signature == TIMER_SIGNATURE);
    volatile led_color_t *led_color_p = arg;

    color_led_toggle(*led_color_p);
}


static void init_heartbeat_timer(void)
{
    rtos_timer_init(&g_heartbeat_timer, "Heartbeat timer", HEARTBEAT_PERIOD_MS,
                    true, heartbeat_timer_callback, (void *)&g_heartbeat_led_color);

    rtos_timer_start(&g_heartbeat_timer);
}


/**
 * Main task. It is responsible for creating the other tasks,
 * and then it handles the command-line input.
 */
static void main_task_func(void *arg)
{
    D_ASSERT(arg == NULL);

    /*
     * Initializes uCOSIII uC/CPU services:
     *
     * NOTE: This cannot be done in rtos_init(), as uCOSIII requires
     * that this function be called after OSStart() has been called
     */
    CPU_Init();

    /*
     * Start RTOS tick timer interrupt:
     *
     * NOTE: This cannot be done in rtos_init(), as uCOSIII requires
     * that this function be called after OSStart() has been called
     */
    rtos_tick_timer_init();

    /*
     * Initialize devices used:
     */
    pin_config_init();
    color_led_init();
    console_init(&g_console_output_task);
    nor_flash_init();
    networking_init();

    /*
     * Start timer for heartbeat LED:
     */
    init_heartbeat_timer();

    /*
     * Display greeting:
     */
    console_clear();
    console_printf("Lab2 - Networking Layer 3 (built " __DATE__ " " __TIME__ ")\n"
                   "Reference solution\n");

    /*
     * Set default local IPv4 address:
     */
    net_layer3_set_local_ipv4_address(&g_local_ipv4_addr, 24);

    /*
     * Create other tasks:
     */
    rtos_task_create(&g_network_stats_task,
                     "Network stats display task",
                     network_stats_task_func,
                     NULL,
                     LOWEST_APP_TASK_PRIORITY - 1);

    rtos_task_create(&g_stacks_checker_task,
                     "Stacks checker task",
                     stacks_checker_task_func,
                     NULL,
                     LOWEST_APP_TASK_PRIORITY);

    rtos_task_create(&g_udp_server_task,
                        "UDP server task",
                        udp_server_task_func,
                        NULL,
                        HIGHEST_APP_TASK_PRIORITY + 3);
    /*
     * Handle command-line input at a lower priority:
     */

    rtos_task_change_self_priority(LOWEST_APP_TASK_PRIORITY - 1);
    console_lock();
    console_set_scroll_region(22, 0);
    console_set_cursor_and_attributes(22, 1, 0, false);
    command_line_init("lab2>", command_parser);
    console_unlock();

    for ( ; ; ) {
        command_line_process_input(true);
    }
}


int main(void)
{
    hardware_init();
    pin_config_init();
    rtos_init();

    rtos_task_create(&g_main_task,
                     "main task",
                     main_task_func,
                     NULL,
                     HIGHEST_APP_TASK_PRIORITY);

    rtos_scheduler_start();

    /*UNREACHABLE*/
    D_ASSERT(false);
    return 1;
}
