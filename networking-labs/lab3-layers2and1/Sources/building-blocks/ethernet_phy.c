/**
 * @file ethernet_phy.c
 *
 * Micrel KSZ8081RNA Ethernet PHY driver
 *
 * @author German Rivera
 */
#include "ethernet_phy.h"
#include "rtos_wrapper.h"
#include "ethernet_mac.h"
#include "microcontroller.h"
#include "runtime_checks.h"
#include "io_utils.h"

/**
 * ETHERNET PHY Registers
 */
enum ethernet_phy_registers {
    ETHERNET_PHY_CONTROL_REG =              0x0, /* basic control register */
    ETHERNET_PHY_STATUS_REG =               0x1, /* basic status register */
    ETHERNET_PHY_ID1_REG =                  0x2, /* identification register 1 */
    ETHERNET_PHY_ID2_REG =                  0x3, /* identification register 2 */
    ETHERNET_PHY_INTR_CONTROL_STATUS_REG =  0x1b, /* interrupt control/status register */
    ETHERNET_PHY_CONTROL1_REG =             0x1e, /* control register 1 */
    ETHERNET_PHY_CONTROL2_REG =             0x1f, /* control register 2*/
};

/**
 * Values for the operation code field of the Ethernet MAC's MMFR register.
 * This register is used to send read/write commands to the Ethernet PHY
 * through the RMII management interface, over the MDIO bus.
 */
enum enet_mmfr_op_values {
    ENET_MMFR_OP_WRITE_NON_MII_COMPLIANT_FRAME =     0x0,
    ENET_MMFR_OP_WRITE_VALID_MII_MANAGEMENT_FRAME =  0x1,
    ENET_MMFR_OP_READ_VALID_MII_MANAGEMENT_FRAME =   0x2,
    ENET_MMFR_OP_READ_NON_MII_COMPLIANT_FRAME =      0x3,
};

/*
 * Bit masks for ETHERNET_PHY_CONTROL_REG register flags
 */
#define ETHERNET_PHY_RESET_MASK               BIT(15)
#define ETHERNET_PHY_LOOP_MASK                BIT(14)
#define ETHERNET_PHY_100_MBPS_SPEED_MASK      BIT(13)
#define ETHERNET_PHY_AUTO_NEGOTIATION_MASK    BIT(12)
#define ETHERNET_PHY_POWER_DOWN_MASK          BIT(11)
#define ETHERNET_PHY_ISOLATE_MASK             BIT(10)
#define ETHERNET_PHY_RESTART_AUTO_NEG_MASK    BIT(9)
#define ETHERNET_PHY_FULL_DUPLEX_MODE_MASK    BIT(8)
#define ETHERNET_PHY_COLLISION_TEST_MASK      BIT(7)

/*
 * Bit masks for ETHERNET_PHY_STATUS_REG register flags
 */
#define ETHERNET_PHY_AUTO_NEG_COMPLETE_MASK   BIT(5)
#define ETHERNET_PHY_AUTO_NEG_CAPABLE_MASK    BIT(3)
#define ETHERNET_PHY_LINK_UP_MASK             BIT(2)

/*
 * Bit masks for ETHERNET_PHY_INTR_CONTROL_STATUS_REG register flags
 */
#define ETHERNET_PHY_RECEIVE_ERROR_INTR_ENABLE_MASK BIT(14)
#define ETHERNET_PHY_LINK_DOWN_INTR_ENABLE_MASK     BIT(10)
#define ETHERNET_PHY_LINK_UP_INTR_ENABLE_MASK       BIT(8)
#define ETHERNET_PHY_RECEIVE_ERROR_INTR_MASK        BIT(6)
#define ETHERNET_PHY_LINK_DOWN_INTR_MASK            BIT(2)
#define ETHERNET_PHY_LINK_UP_INTR_MASK              BIT(0)

/*
 * Bit masks for ETHERNET_PHY_CONTROL2_REG register flags
 */
#define ETHERNET_PHY_POWER_SAVING_MASK        BIT(10) /* 1 = enabled, 0 = disabled, default 0 */
#define ETHERNET_PHY_INTR_LEVEL_MASK          BIT(9)  /* 1 = active high, 0 = active low, default 0 */
#define ETHERNET_PHY_DISABLE_TRANSMITTER_MASK BIT(3)  /* 1 = disabled, 0 = enabled, default 0 */

/**
 * Maximum number of iterations for a polling loop
 * waiting for response from the Ethernet PHY
 */
#define ETHERNET_PHY_MAX_POLLING_COUNT   UINT16_MAX

/**
 * Non-const fields of an Ethernet PHY device (to be placed in SRAM)
 */
struct ethernet_phy_device_var {
    /**
     * Flag indicating that ethernet_phy_init() has been called
     * for this PHY
     */
    bool initialized;

    /**
     * Mutex to serialize access to the Ethernet PHY
     */
    struct rtos_mutex mutex;
};

/**
 * Global non-const structure for Ethernet PHY device
 * (allocated in SRAM space)
 */
static struct ethernet_phy_device_var g_ethernet_phy0_var = {
    .initialized = false,
};

/**
 * Global const structure for the Ethernet PHY device
 * (allocated in flash space)
 */
const struct ethernet_phy_device g_ethernet_phy0 = {
    .signature = ETHERNET_PHY_DEVICE_SIGNATURE,
    .var_p = &g_ethernet_phy0_var,
    .ethernet_mac_p = &g_ethernet_mac0,
    .mdio_address = 0x0,
    .rmii_rxer_pin = PIN_INITIALIZER(PIN_PORT_A, 5, PIN_FUNCTION_ALT4),
    .rmii_rxd1_pin = PIN_INITIALIZER(PIN_PORT_A, 12, PIN_FUNCTION_ALT4),
    .rmii_rxd0_pin = PIN_INITIALIZER(PIN_PORT_A, 13, PIN_FUNCTION_ALT4),
    .rmii_crs_dv_pin = PIN_INITIALIZER(PIN_PORT_A, 14, PIN_FUNCTION_ALT4),
    .rmii_txen_pin = PIN_INITIALIZER(PIN_PORT_A, 15, PIN_FUNCTION_ALT4),
    .rmii_txd0_pin = PIN_INITIALIZER(PIN_PORT_A, 16, PIN_FUNCTION_ALT4),
    .rmii_txd1_pin = PIN_INITIALIZER(PIN_PORT_A, 17, PIN_FUNCTION_ALT4),
    .mii_txer_pin = PIN_INITIALIZER(PIN_PORT_A, 28, PIN_FUNCTION_ALT4),
    .rmii_mdio_pin = PIN_INITIALIZER(PIN_PORT_B, 0, PIN_FUNCTION_ALT4),
    .rmii_mdc_pin = PIN_INITIALIZER(PIN_PORT_B, 1, PIN_FUNCTION_ALT4),
};


static void
ethernet_phy_mdio_write_nolock(const struct ethernet_phy_device *ethernet_phy_p,
                               uint32_t phy_reg,
                               uint32_t data)
{
    const struct ethernet_mac_device *const ethernet_mac_p =
        ethernet_phy_p->ethernet_mac_p;
    ENET_Type *const enet_regs_p = ethernet_mac_p->mmio_registers_p;
    uint32_t reg_value;
    uint_fast16_t polling_count;

    reg_value = READ_MMIO_REGISTER(&enet_regs_p->MSCR);
    D_ASSERT((reg_value & ENET_MSCR_MII_SPEED_MASK) != 0);

    reg_value = READ_MMIO_REGISTER(&enet_regs_p->EIR);
    D_ASSERT((reg_value & ENET_EIR_MII_MASK) == 0);

    /*
     * Set write command
     */
    reg_value = 0;
    SET_BIT_FIELD(reg_value, ENET_MMFR_ST_MASK, ENET_MMFR_ST_SHIFT,
                  0x1);
    SET_BIT_FIELD(reg_value, ENET_MMFR_OP_MASK, ENET_MMFR_OP_SHIFT,
                  ENET_MMFR_OP_WRITE_VALID_MII_MANAGEMENT_FRAME);
    SET_BIT_FIELD(reg_value, ENET_MMFR_PA_MASK, ENET_MMFR_PA_SHIFT,
                  ethernet_phy_p->mdio_address);
    SET_BIT_FIELD(reg_value, ENET_MMFR_RA_MASK, ENET_MMFR_RA_SHIFT,
                  phy_reg);
    SET_BIT_FIELD(reg_value, ENET_MMFR_TA_MASK, ENET_MMFR_TA_SHIFT,
                  0x2);
    SET_BIT_FIELD(reg_value, ENET_MMFR_DATA_MASK, ENET_MMFR_DATA_SHIFT,
                  data);
    write_32bit_mmio_register(&enet_regs_p->MMFR, reg_value);

    /*
     * Wait for SMI write to complete
     * (the MMI interrupt event bit is set in the EIR, when
     *  an SMI data transfer is completed)
     */
    polling_count = ETHERNET_PHY_MAX_POLLING_COUNT;
    do {
        reg_value = READ_MMIO_REGISTER(&enet_regs_p->EIR);
        polling_count --;
    } while ((reg_value & ENET_EIR_MII_MASK) == 0 && polling_count != 0);

    if ((reg_value & ENET_EIR_MII_MASK) == 0) {
        error_t error = CAPTURE_ERROR("SMI write failed", ethernet_phy_p,
                                      reg_value);

        fatal_error_handler(error);
        /*UNREACHABLE*/
    }

    /*
     * Clear the MII interrupt event in the EIR register
     * (EIR is a w1c register)
     */
    WRITE_MMIO_REGISTER(&enet_regs_p->EIR, ENET_EIR_MII_MASK);
}


/**
 * Write a value to a given Ethernet PHY control register via
 * the RMII management interface over the MDIO bus.
 *
 * It serializes concurrent callers for the same PHY device.
 *
 * @param ethernet_phy_p    Pointer to Ethernet PHY device
 * @param phy_reg           Index of PHY register to write
 * @param data              Value to be written to the PHY register
 *
 * @pre Cannot be called interrupt context or with interrupts disabled.
 */
static void ethernet_phy_mdio_write(const struct ethernet_phy_device *ethernet_phy_p,
                                    uint32_t phy_reg,
                                    uint32_t data)
{
    struct ethernet_phy_device_var *const phy_var_p = ethernet_phy_p->var_p;

    D_ASSERT(phy_var_p->initialized);
    rtos_mutex_lock(&phy_var_p->mutex);
    ethernet_phy_mdio_write_nolock(ethernet_phy_p, phy_reg, data);
    rtos_mutex_unlock(&phy_var_p->mutex);
}


static uint32_t
ethernet_phy_mdio_read_nolock(const struct ethernet_phy_device *ethernet_phy_p,
                              uint32_t phy_reg)
{
    const struct ethernet_mac_device *const ethernet_mac_p =
        ethernet_phy_p->ethernet_mac_p;
    ENET_Type *const enet_regs_p = ethernet_mac_p->mmio_registers_p;
    uint32_t reg_value;
    uint16_t polling_count = ETHERNET_PHY_MAX_POLLING_COUNT;

    reg_value = READ_MMIO_REGISTER(&enet_regs_p->MSCR);
    D_ASSERT((reg_value & ENET_MSCR_MII_SPEED_MASK) != 0);

    reg_value = READ_MMIO_REGISTER(&enet_regs_p->EIR);
    D_ASSERT((reg_value & ENET_EIR_MII_MASK) == 0);

    /*
     * Set read command
     */
    reg_value = 0;
    SET_BIT_FIELD(reg_value, ENET_MMFR_ST_MASK, ENET_MMFR_ST_SHIFT,
                  0x1);
    SET_BIT_FIELD(reg_value, ENET_MMFR_OP_MASK, ENET_MMFR_OP_SHIFT,
                  ENET_MMFR_OP_READ_VALID_MII_MANAGEMENT_FRAME);
    SET_BIT_FIELD(reg_value, ENET_MMFR_PA_MASK, ENET_MMFR_PA_SHIFT,
                  ethernet_phy_p->mdio_address);
    SET_BIT_FIELD(reg_value, ENET_MMFR_RA_MASK, ENET_MMFR_RA_SHIFT,
                  phy_reg);
    SET_BIT_FIELD(reg_value, ENET_MMFR_TA_MASK, ENET_MMFR_TA_SHIFT,
                  0x2);
    WRITE_MMIO_REGISTER(&enet_regs_p->MMFR, reg_value);

    /*
     * Wait for SMI read to complete
     * (the MMI interrupt event bit is set in the EIR, when
     *  an SMI data transfer is completed)
     */
    do {
        reg_value = READ_MMIO_REGISTER(&enet_regs_p->EIR);
        polling_count --;
    } while ((reg_value & ENET_EIR_MII_MASK) == 0 && polling_count != 0);

    if ((reg_value & ENET_EIR_MII_MASK) == 0) {
        error_t error = CAPTURE_ERROR("SMI read failed", ethernet_phy_p,
                                      reg_value);

        fatal_error_handler(error);
        /*UNREACHABLE*/
    }

    reg_value = READ_MMIO_REGISTER(&enet_regs_p->MMFR);

    /*
     * Clear the MII interrupt event in the EIR register
     * (EIR is a w1c register)
     */
    WRITE_MMIO_REGISTER(&enet_regs_p->EIR, ENET_EIR_MII_MASK);

    return GET_BIT_FIELD(reg_value, ENET_MMFR_DATA_MASK, ENET_MMFR_DATA_SHIFT);
}


/**
 * Reads a value from a given Ethernet PHY control register via
 * the RMII management interface over the MDIO bus.
 *
 * It serializes concurrent callers for the same PHY device.
 *
 * @param ethernet_phy_p    Pointer to Ethernet PHY device
 * @param phy_reg           Index of PHY register to read
 *
 * @return Value read from the PHY register
 *
 * @pre Cannot be called interrupt context or with interrupts disabled.
 */
static uint32_t ethernet_phy_mdio_read(
            const struct ethernet_phy_device *ethernet_phy_p,
            uint32_t phy_reg)
{
    uint32_t reg_value;
    struct ethernet_phy_device_var *const phy_var_p = ethernet_phy_p->var_p;

    D_ASSERT(phy_var_p->initialized);
    rtos_mutex_lock(&phy_var_p->mutex);
    reg_value = ethernet_phy_mdio_read_nolock(ethernet_phy_p, phy_reg);
    rtos_mutex_unlock(&phy_var_p->mutex);
    return reg_value;
}


static void
ether_phy_mdio_init(const struct ethernet_phy_device *ethernet_phy_p)
{
    uint32_t reg_value;
    const struct ethernet_mac_device *const ethernet_mac_p =
        ethernet_phy_p->ethernet_mac_p;
    ENET_Type *const enet_regs_p = ethernet_mac_p->mmio_registers_p;

    /**
     * Initialize the Serial Management Interface (SMI) between the Ethernet MAC
     * and the Ethernet PHY chip. The SMI is also known as the MII Management
     * interface (MIIM) and consists of two pins: MDC and MDIO.
     *
     * Settings:
     * - HOLDTIME:0 (Hold time on MDIO output: one internal module clock cycle)
     * - DIS_PRE: 0 (Preamble enabled)
     * - MII_SPEED: 24 (MDC clock freq: (1 / ((MII_SPEED + 1) * 2) = 1/50 of the
     *                  internal module clock frequency, which is 50MHz)
     */
    reg_value = 0;
    SET_BIT_FIELD(reg_value, ENET_MSCR_MII_SPEED_MASK,
                  ENET_MSCR_MII_SPEED_SHIFT,
                  MCU_CPU_CLOCK_FREQ_IN_MHZ / 5);
    WRITE_MMIO_REGISTER(&enet_regs_p->MSCR, reg_value);

    /*
     * Set GPIO pins for Ethernet PHY MII management (MDIO) functions:
     */

    set_pin_function(&ethernet_phy_p->rmii_mdc_pin, 0);

    /*
     * Set "open drain enabled", "pull-up resistor enabled" and
     * "internal pull resistor enabled" for rmii_mdio pin
     *
     * NOTE: No external pullup is available on MDIO signal when the K64F SoC
     * requests status of the Ethernet link connection. Internal pullup
     * is required when port configuration for MDIO signal is enabled.
     */
    set_pin_function(&ethernet_phy_p->rmii_mdio_pin,
                     PORT_PCR_ODE_MASK |
                     PORT_PCR_PE_MASK |
                     PORT_PCR_PS_MASK);
}


/**
 * Initializes an Ethernet PHY chip (Micrel KSZ8081RNA)
 *
 * @param ethernet_phy_p    Pointer to the Ethernet PHY
 */
void ethernet_phy_init(const struct ethernet_phy_device *ethernet_phy_p)
{
    uint32_t reg_value;
    uint16_t polling_count;
    error_t error;

    D_ASSERT(ethernet_phy_p->signature == ETHERNET_PHY_DEVICE_SIGNATURE);

    struct ethernet_phy_device_var *const phy_var_p = ethernet_phy_p->var_p;

    D_ASSERT(!phy_var_p->initialized);
    rtos_mutex_init(&phy_var_p->mutex, "Ethernet PHY mutex");

    ether_phy_mdio_init(ethernet_phy_p);

    /*
     * Set GPIO pins for Ethernet PHY RMII functions:
     */
    set_pin_function(&ethernet_phy_p->rmii_rxd0_pin, 0);
    set_pin_function(&ethernet_phy_p->rmii_rxd1_pin, 0);
    set_pin_function(&ethernet_phy_p->rmii_crs_dv_pin, 0);
    set_pin_function(&ethernet_phy_p->rmii_rxer_pin, 0);
    set_pin_function(&ethernet_phy_p->rmii_txd0_pin, 0);
    set_pin_function(&ethernet_phy_p->rmii_txd1_pin, 0);
    set_pin_function(&ethernet_phy_p->rmii_txen_pin, 0);
    set_pin_function(&ethernet_phy_p->mii_txer_pin, 0);

    /*
     * Reset Phy
     */
    ethernet_phy_mdio_write_nolock(ethernet_phy_p,
                                   ETHERNET_PHY_CONTROL_REG,
                                   ETHERNET_PHY_RESET_MASK);

    /*
     * Wait for reset to complete:
     */
    polling_count = ETHERNET_PHY_MAX_POLLING_COUNT;
    do {
        reg_value = ethernet_phy_mdio_read_nolock(ethernet_phy_p,
                                                  ETHERNET_PHY_CONTROL_REG);
        polling_count --;
    } while ((reg_value & ETHERNET_PHY_RESET_MASK) != 0 && polling_count != 0);

    if ((reg_value & ETHERNET_PHY_RESET_MASK) != 0) {
        error = CAPTURE_ERROR("Ethernet PHY reset failed", ethernet_phy_p,
                              reg_value);
        fatal_error_handler(error);
        /*UNREACHABLE*/
    }

    reg_value = ethernet_phy_mdio_read_nolock(ethernet_phy_p,
                                              ETHERNET_PHY_STATUS_REG);
    if ((reg_value & ETHERNET_PHY_AUTO_NEG_CAPABLE_MASK) != 0 &&
        (reg_value & ETHERNET_PHY_AUTO_NEG_COMPLETE_MASK) == 0) {
        /*
         * Set auto-negotiation:
         */
        reg_value = ethernet_phy_mdio_read_nolock(ethernet_phy_p,
                                                  ETHERNET_PHY_CONTROL_REG);
        reg_value |= ETHERNET_PHY_AUTO_NEGOTIATION_MASK;
        ethernet_phy_mdio_write_nolock(ethernet_phy_p, ETHERNET_PHY_CONTROL_REG,
                                       reg_value);

        /*
         * Wait for auto-negotiation completion:
         */
        polling_count = ETHERNET_PHY_MAX_POLLING_COUNT;
        do {
            reg_value = ethernet_phy_mdio_read_nolock(ethernet_phy_p,
                                                      ETHERNET_PHY_STATUS_REG);
            polling_count --;
        } while ((reg_value & ETHERNET_PHY_AUTO_NEG_COMPLETE_MASK) == 0 &&
                 polling_count != 0);

        if ((reg_value & ETHERNET_PHY_RESET_MASK) != 0) {
            error = CAPTURE_ERROR("Ethernet PHY auto-negotiation failed",
                                  ethernet_phy_p, reg_value);

            fatal_error_handler(error);
            /*UNREACHABLE*/
        }
    }

    phy_var_p->initialized = true;
}


/**
 * Tell if the Ethernet link is up for a given Ethernet PHY
 *
 * @param ethernet_phy_p Pointer to Ethernet PHY
 *
 * @return true, link is up
 * @return false, link is down
 */
bool ethernet_phy_link_is_up(const struct ethernet_phy_device *ethernet_phy_p)
{
	D_ASSERT(ethernet_phy_p->signature == ETHERNET_PHY_DEVICE_SIGNATURE);
	struct ethernet_phy_device_var *const phy_var_p = ethernet_phy_p->var_p;

	D_ASSERT(phy_var_p->initialized);
    uint32_t reg_value = ethernet_phy_mdio_read(ethernet_phy_p,
                                                ETHERNET_PHY_STATUS_REG);

    return (reg_value & ETHERNET_PHY_LINK_UP_MASK) != 0;
}


/**
 * Turns on/off loopback mode for the Ethernet PHY
 *
 * @param ethernet_phy_p	Pointer to Ethernet PHY
 * @param on				Boolean flag: true (on), false (off)
 */
void ethernet_phy_set_loopback(const struct ethernet_phy_device *ethernet_phy_p,
		                       bool on)
{
	D_ASSERT(ethernet_phy_p->signature == ETHERNET_PHY_DEVICE_SIGNATURE);
	struct ethernet_phy_device_var *const phy_var_p = ethernet_phy_p->var_p;

	D_ASSERT(phy_var_p->initialized);
	uint32_t reg_value = ethernet_phy_mdio_read(ethernet_phy_p,
	                                            ETHERNET_PHY_CONTROL_REG);

	if (on) {
		reg_value |= ETHERNET_PHY_LOOP_MASK;
	} else {
		reg_value &= ~ETHERNET_PHY_LOOP_MASK;
	}

    ethernet_phy_mdio_write(ethernet_phy_p,
                            ETHERNET_PHY_CONTROL_REG,
                            reg_value);
}
