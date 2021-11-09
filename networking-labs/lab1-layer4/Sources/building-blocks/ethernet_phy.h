/**
 * @file ethernet_phy.h
 *
 * Ethernet PHY driver interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_ETHERNET_PHY_H_
#define SOURCES_BUILDING_BLOCKS_ETHERNET_PHY_H_

#include <stdint.h>
#include "runtime_checks.h"
#include "microcontroller.h"
#include "pin_config.h"

/**
 * Const fields of an Ethernet PHY device (to be placed in flash)
 */
struct ethernet_phy_device {
	/*
	 * Signature for run-time type checking
	 */
#   define ETHERNET_PHY_DEVICE_SIGNATURE  GEN_SIGNATURE('E', 'P', 'H', 'Y')
    uint32_t signature;

    /**
     * Pointer to in-RAM control block for this PHY device
     */
    struct ethernet_phy_device_var *var_p;

    /**
     * Pointer to Ethernet MAC device physically connected
     * to this PHY
     */
    const struct ethernet_mac_device *ethernet_mac_p;

    /**
     * Address of this PHY device on the MDIO bus
     */
    uint32_t mdio_address;

    /*
     * RMII interface pins used to connect the MCU's Ethernet MAC to this PHY chip
     */
    struct pin_info rmii_rxd0_pin;
    struct pin_info rmii_rxd1_pin;
    struct pin_info rmii_crs_dv_pin;
    struct pin_info rmii_rxer_pin;
    struct pin_info rmii_txen_pin;
    struct pin_info rmii_txd0_pin;
    struct pin_info rmii_txd1_pin;
    struct pin_info mii_txer_pin;
    struct pin_info mii_intr_pin;

    /*
     * RMII management interface pins used to connect the MCU's Ethernet MAC to
     * this PHY chip over an MDIO bus
     */
    struct pin_info rmii_mdio_pin;
    struct pin_info rmii_mdc_pin;
};


void ethernet_phy_init(const struct ethernet_phy_device *ethernet_phy_p);

bool ethernet_phy_link_is_up(const struct ethernet_phy_device *ethernet_phy_p);

void ethernet_phy_set_loopback(const struct ethernet_phy_device *ethernet_phy_p,
		                       bool on);

extern const struct ethernet_phy_device g_ethernet_phy0;

#endif /* SOURCES_BUILDING_BLOCKS_ETHERNET_PHY_H_ */
