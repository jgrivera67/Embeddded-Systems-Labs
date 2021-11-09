/**
 * @file ethernet_mac.h
 *
 * Ethernet MAC driver interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_ETHERNET_MAC_H_
#define SOURCES_BUILDING_BLOCKS_ETHERNET_MAC_H_

#include <stdint.h>
#include "runtime_checks.h"
#include "microcontroller.h"
#include "pin_config.h"

struct ethernet_mac_address;
struct net_layer2_end_point;
struct network_packet;

/**
 * Const fields of an Ethernet MAC device (to be placed in flash)
 */
struct ethernet_mac_device {
    /*
     * Signature for run-time type checking
     */
#   define ETHERNET_MAC_DEVICE_SIGNATURE  GEN_SIGNATURE('E', 'M', 'A', 'C')
    uint32_t signature;

    /**
     * Device name (null-terminated string)
     */
    const char *name_p;

    /**
     * Pointer to in-RAM control block for this MAC device
     */
    struct ethernet_mac_device_var *var_p;

    /**
     * Pointer to MMIO registers for this MAC device
     */
    ENET_Type *mmio_registers_p;

    /**
     * Pointer to Ethernet PHY device physically connected
     * to this MAC
     */
    const struct ethernet_phy_device *ethernet_phy_p;

    /**
     * IEEE 1588 timestamp timer pins
     */
    struct pin_info ieee_1588_timer_pins[4];

    /**
     * Transmit completion IRQ number
     */
    IRQn_Type tx_irq_num;

    /**
     * Receive completion IRQ number
     */
    IRQn_Type rx_irq_num;

    /**
     * Error IRQ number
     */
    IRQn_Type error_irq_num;

    /**
     * Clock gate mask to enable the clock for this MAC
     */
    uint32_t clock_gate_mask;
};


void ethernet_mac_init(const struct ethernet_mac_device *ethernet_mac_p,
                       struct net_layer2_end_point *layer2_end_point_p);

void ethernet_mac_start(const struct ethernet_mac_device *ethernet_mac_p);

void ethernet_mac_add_multicast_addr(const struct ethernet_mac_device *ethernet_mac_p,
                                     struct ethernet_mac_address *mac_addr_p);

void ethernet_mac_remove_multicast_addr(const struct ethernet_mac_device *ethernet_mac_p,
                                        struct ethernet_mac_address *mac_addr_p);

void ethernet_mac_start_xmit(const struct ethernet_mac_device *ethernet_mac_p,
                             struct network_packet *tx_packet_p);

void ethernet_mac_repost_rx_packet(const struct ethernet_mac_device *ethernet_mac_p,
                                   struct network_packet *rx_packet_p);

extern const struct ethernet_mac_device g_ethernet_mac0;

#endif /* SOURCES_BUILDING_BLOCKS_ETHERNET_MAC_H_ */
