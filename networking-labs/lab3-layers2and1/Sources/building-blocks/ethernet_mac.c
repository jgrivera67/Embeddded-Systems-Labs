/**
 * @file ethernet_mac.c
 *
 * K64F Ethernet MAC driver
 *
 * @author German Rivera
 */
#include "ethernet_mac.h"
#include "ethernet_phy.h"
#include "crc_32.h"
#include "networking_layer2.h"
#include "compile_time_checks.h"
#include "runtime_checks.h"
#include "runtime_log.h"
#include "io_utils.h"
#include "interrupt_vector_table.h"
#include "memory_protection_unit.h"

/*
 * Compile-time configuration options:
 */
#define ENET_DATA_PAYLOAD_32_BIT_ALIGNED
#define    ENET_CHECKSUM_OFFLOAD

C_ASSERT(NET_PACKET_DATA_BUFFER_SIZE >= 256 &&
     (NET_PACKET_DATA_BUFFER_SIZE & ~ENET_MRBR_R_BUF_SIZE_MASK) == 0);

/**
 * Maximum number of iterations for a polling loop
 * waiting for reset completion of the Ethernet MAC
 */
#define ETHERNET_MAC_RESET_MAX_POLLING_COUNT   UINT16_MAX

/**
 * Ethernet frame buffer descriptor alignment in bytes
 */
#define ETHERNET_FRAME_BUFFER_DESCRIPTOR_ALIGNMENT UINT32_C(16)

/**
 * Rx buffer descriptor (type of entries of the Ethernet MAC Rx ring)
 */
struct ethernet_rx_buffer_descriptor {
    /**
     * If the ENET_RX_BD_LAST_IN_FRAME bit is set in 'control', this is the
     * frame length in bytes including CRC
     */
    uint16_t data_length;

    /**
     * Rx buffer descriptor control flags
     */
    uint16_t control;
#   define  ENET_RX_BD_EMPTY_MASK                      BIT(15)
#   define  ENET_RX_BD_SOFTWARE_OWNERSHIP1_MASK        BIT(14)
#   define  ENET_RX_BD_WRAP_MASK                       BIT(13)
#   define  ENET_RX_BD_SOFTWARE_OWNERSHIP2_MASK        BIT(12)
#   define  ENET_RX_BD_LAST_IN_FRAME_MASK              BIT(11)
#   define  ENET_RX_BD_MISS_MASK                       BIT(8)
#   define  ENET_RX_BD_BROADCAST_MASK                  BIT(7)
#   define  ENET_RX_BD_MULTICAST_MASK                  BIT(6)
#   define  ENET_RX_BD_LENGTH_VIOLATION_MASK           BIT(5)
#   define  ENET_RX_BD_NON_OCTET_ALIGNED_FRAME_MASK    BIT(4)
#   define  ENET_RX_BD_CRC_ERROR_MASK                  BIT(2)
#   define  ENET_RX_BD_FIFO_OVERRRUN_MASK              BIT(1)
#   define  ENET_RX_BD_FRAME_TRUNCATED_MASK            BIT(0)

    /**
     * Pointer to the Rx buffer descriptor's data buffer
     */
    void *data_buffer;

    /**
     * Rx buffer descriptor's first extended control flags
     */
    uint16_t control_extend0;
#   define  ENET_RX_BD_VLAN_PRIORITY_CODE_POINT_MASK   MULTI_BIT_MASK(15, 13)
#   define  ENET_RX_BD_VLAN_PRIORITY_CODE_POINT_SHIFT  13
#   define  ENET_RX_BD_IP_HEADER_CHECKSUM_ERROR_MASK   BIT(5)
#   define  ENET_RX_BD_PROTOCOL_CHECKSUM_ERROR_MASK    BIT(4)
#   define  ENET_RX_BD_VLAN_FRAME_MASK                 BIT(2)
#   define  ENET_RX_BD_IPv6_FRAME_MASK                 BIT(1)
#   define  ENET_RX_BD_IPv4_FRAGMENT_MASK              BIT(0)

    /**
     * Rx buffer descriptor's second extended control flags
     */
    uint16_t control_extend1;
#   define  ENET_RX_BD_GENERATE_INTERRUPT_MASK        BIT(7)
#   define  ENET_RX_BD_UNICAST_FRAME_MASK             BIT(8)
#   define  ENET_RX_BD_COLLISION_MASK                 BIT(9)
#   define  ENET_RX_BD_PHY_ERROR_MASK                 BIT(10)
#   define  ENET_RX_BD_MAC_ERROR_MASK                 BIT(15)

    /**
     * Data payload checksum
     */
    uint16_t payload_checksum;

    /**
     * Frame header length
     */
    uint8_t header_length;

    /**
     * Protocol type
     */
    uint8_t protocol_type;

    uint16_t reserved0;

    /**
     * Rx buffer descriptor's third extended control flags
     */
    uint16_t control_extend2;
#   define  ENET_RX_BD_LAST_DESCRIPTOR_UPDATE_DONE_MASK    BIT(15)

    /**
     * Rx buffer descriptor timestamp
     */
    uint32_t timestamp;

    uint16_t reserved1;
    uint16_t reserved2;
    uint16_t reserved3;
    uint16_t reserved4;
} __attribute__ ((aligned(ETHERNET_FRAME_BUFFER_DESCRIPTOR_ALIGNMENT)));

C_ASSERT(sizeof(struct ethernet_rx_buffer_descriptor) %
         ETHERNET_FRAME_BUFFER_DESCRIPTOR_ALIGNMENT == 0);

/**
 * Tx buffer descriptor (type of entries of the Ethernet MAC Tx ring)
 */
struct ethernet_tx_buffer_descriptor {
    /**
     * Length of the frame data payload in bytes
     */
    uint16_t data_length;

    /**
     * Tx buffer descriptor's control flags
     */
    uint16_t control;
#   define  ENET_TX_BD_READY_MASK            BIT(15)
#   define  ENET_TX_BD_SOFTWARE_OWNER1_MASK  BIT(14)
#   define  ENET_TX_BD_WRAP_MASK             BIT(13)
#   define  ENET_TX_BD_SOFTWARE_OWNER2_MASK  BIT(12)
#   define  ENET_TX_BD_LAST_IN_FRAME_MASK    BIT(11)
#   define  ENET_TX_BD_CRC_MASK              BIT(10)

    /**
     * Pointer to the Tx buffer descriptor's data buffer
     */
    void *data_buffer;

    /**
     * Tx buffer descriptor's first extended control flags
     */
    uint16_t control_extend0;
#   define  ENET_TX_BD_ERROR_MASK                   BIT(15)
#   define  ENET_TX_BD_UNDERFLOW_MASK               BIT(13)
#   define  ENET_TX_BD_EXCESS_COLLISION_ERROR_MASK  BIT(12)
#   define  ENET_TX_BD_FRAME_ERROR_MASK             BIT(11)
#   define  ENET_TX_BD_LATE_COLLISION_ERROR_MASK    BIT(10)
#   define  ENET_TX_BD_FIFO_OVERFLOW_ERROR_MASK     BIT(9)
#   define  ENET_TX_BD_TMESTAMP_ERROR_MASK          BIT(8)

    uint16_t control_extend1;
#   define  ENET_TX_BD_INTERRUPT_MASK                 BIT(14)
#   define  ENET_TX_BD_TIMESTAMP_MASK                 BIT(13)
#   define  ENET_TX_BD_INSERT_PROTOCOL_CHECKSUM_MASK  BIT(12)
#   define  ENET_TX_BD_INSERT_IP_HEADER_CHECKSUM_MASK BIT(11)

    uint16_t reserved0;
    uint16_t reserved1;
    uint16_t reserved2;

    /**
     * Tx buffer descriptor's second extended control flags
     */
    uint16_t control_extend2;
#   define  ENET_TX_BD_LAST_DESCRIPTOR_UPDATE_DONE_MASK    BIT(15)

    /**
     * Tx buffer descriptor timestamp
     */
    uint32_t timestamp;

    uint16_t reserved3;
    uint16_t reserved4;
    uint16_t reserved5;
    uint16_t reserved6;
} __attribute__ ((aligned(ETHERNET_FRAME_BUFFER_DESCRIPTOR_ALIGNMENT)));

C_ASSERT(sizeof(struct ethernet_tx_buffer_descriptor) %
         ETHERNET_FRAME_BUFFER_DESCRIPTOR_ALIGNMENT == 0);

/**
 * Non-const fields of an Ethernet MAC device (to be placed in SRAM)
 */
struct ethernet_mac_device_var {
    /**
     * Flag indicating that ethernet_mac_init() has been called
     * for this MAC
     */
    bool initialized;

    /**
     * Ethernet MAC address for this layer-2 end point (Ethernet port)
     */
    struct ethernet_mac_address mac_address;

    /**
     * Total number of Tx/Rx errors
     */
    uint32_t tx_rx_error_count;

    /**
     * Pointer to the local layer-2 end point associated with this MAC
     */
    struct net_layer2_end_point *layer2_end_point_p;

    /**
     * Number of Tx buffer descriptors currently filled in ths MAC's Tx ring
     */
    uint16_t tx_ring_entries_filled;

    /**
     * Number of Rx buffer descriptors currently filled in this MAC's Rx ring
     */
    uint16_t rx_ring_entries_filled;

    /**
     * This MAC's Tx ring write cursor (pointer to next Tx buffer descriptor
     * that can be filled by ethernet_mac_start_xmit())
     */
    volatile struct ethernet_tx_buffer_descriptor *tx_ring_write_cursor;

    /**
     * This MAC's Tx ring read cursor (pointer to the first Tx buffer descriptor
     * that can be read by ethernet_mac_transmit_interrupt_handler())
     */
    volatile struct ethernet_tx_buffer_descriptor *tx_ring_read_cursor;

    /**
     * This MAC's Rx ring write cursor (pointer to next Rx buffer descriptor
     * that can be filled by ethernet_mac_repost_rx_packet())
     */
    volatile struct ethernet_rx_buffer_descriptor *rx_ring_write_cursor;

    /**
     * This MAC's Rx ring read cursor (pointer to the first Rx buffer descriptor
     * that can be read by ethernet_mac_receive_interrupt_handler())
     */
    volatile struct ethernet_rx_buffer_descriptor *rx_ring_read_cursor;

    /**
     * Array of counters for the multicast hash table buckets. Each entry
     * corresponds to the number of multicast addresses added to the
     * corresponding bucket (bit in the GAUR/GALR bit hash table)
     */
#   define ETHERNET_MAC_MULTICAST_HASH_TABLE_NUM_BUCKETS    64
    uint8_t multicast_hash_table_counts[ETHERNET_MAC_MULTICAST_HASH_TABLE_NUM_BUCKETS];

    /**
     * This MAC's Tx buffer descriptor ring
     */
    volatile struct ethernet_tx_buffer_descriptor tx_buffer_descriptors[NET_MAX_TX_PACKETS];

    /**
     * This MAC's Rx buffer descriptor ring
     */
    volatile struct ethernet_rx_buffer_descriptor rx_buffer_descriptors[NET_MAX_RX_PACKETS];
};

/**
 * Global non-const structure for Ethernet MAC device
 * (allocated in SRAM space)
 */
static struct ethernet_mac_device_var g_ethernet_mac0_var = {
    .initialized = false,
};

/**
 * Global const structure for the Ethernet MAC device
 * (allocated in flash space)
 */
const struct ethernet_mac_device g_ethernet_mac0 = {
    .signature = ETHERNET_MAC_DEVICE_SIGNATURE,
    .name_p = "enet0",
    .var_p = &g_ethernet_mac0_var,
    .mmio_registers_p = (ENET_Type *)ENET_BASE,
    .ethernet_phy_p = &g_ethernet_phy0,
    .ieee_1588_timer_pins = {
        [0] = PIN_INITIALIZER(PIN_PORT_C, 16, PIN_FUNCTION_ALT4),
        [1] = PIN_INITIALIZER(PIN_PORT_C, 17, PIN_FUNCTION_ALT4),
        [2] = PIN_INITIALIZER(PIN_PORT_C, 18, PIN_FUNCTION_ALT4),
        [3] = PIN_INITIALIZER(PIN_PORT_C, 19, PIN_FUNCTION_ALT4),
    },

    .tx_irq_num = ENET_Transmit_IRQn,
    .rx_irq_num = ENET_Receive_IRQn,
    .error_irq_num = ENET_Error_IRQn,
    .clock_gate_mask = SIM_SCGC2_ENET_MASK,
};


static void
ethernet_mac_tx_buffer_descriptor_ring_init(
    const struct ethernet_mac_device *ethernet_mac_p)
{
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;
    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;

    /*
     * Configure Tx buffer descriptor ring:
     * - Set Tx descriptor ring start address
     * - Initialize Tx buffer descriptors
     */
    D_ASSERT(((uintptr_t)mac_var_p->tx_buffer_descriptors &
              ~ENET_TDSR_X_DES_START_MASK) == 0);

    WRITE_MMIO_REGISTER(&mac_regs_p->TDSR,
                        (uintptr_t)mac_var_p->tx_buffer_descriptors);

    for (unsigned int i = 0; i < NET_MAX_TX_PACKETS; i ++) {
        volatile struct ethernet_tx_buffer_descriptor *buffer_desc_p =
            &mac_var_p->tx_buffer_descriptors[i];

        buffer_desc_p->data_buffer = NULL;
        buffer_desc_p->data_length = 0;

        /*
         * Our Tx buffers are large enough to always hold entire frames, so a
         * frame is never fragmented into multiple buffers.
         * Frames smaller than 60 bytes are automatically padded.
         * The minimum Ethernet frame length transmitted on the wire
         * is 64 bytes, including the CRC.
         */
        buffer_desc_p->control = ENET_TX_BD_LAST_IN_FRAME_MASK |
                                 ENET_TX_BD_CRC_MASK;

        /*
         * Set the wrap flag for the last buffer of the ring:
         */
        if (i == NET_MAX_TX_PACKETS - 1) {
            buffer_desc_p->control |= ENET_TX_BD_WRAP_MASK;
        }
    }

    /*
     * The Tx descriptor ring is empty:
     */
    mac_var_p->tx_ring_entries_filled = 0;
    mac_var_p->tx_ring_write_cursor = &mac_var_p->tx_buffer_descriptors[0];
    mac_var_p->tx_ring_read_cursor = &mac_var_p->tx_buffer_descriptors[0];
}


static void
ethernet_mac_rx_buffer_descriptor_ring_init(
    const struct ethernet_mac_device *ethernet_mac_p)
{
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;
    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;
    struct net_layer2_end_point *const layer2_end_point_p = mac_var_p->layer2_end_point_p;

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);

    /*
     * Configure Rx buffer descriptor ring:
     * - Set Rx descriptor ring start address
     * - Set max receive buffer size in bytes
     * - Initialize Rx buffer descriptors
     */
    D_ASSERT(((uintptr_t)mac_var_p->rx_buffer_descriptors &
              ~ENET_RDSR_R_DES_START_MASK) == 0);

    WRITE_MMIO_REGISTER(&mac_regs_p->RDSR,
                        (uintptr_t)mac_var_p->rx_buffer_descriptors);
    WRITE_MMIO_REGISTER(&mac_regs_p->MRBR, NET_PACKET_DATA_BUFFER_SIZE);

    for (unsigned int i = 0; i < NET_MAX_RX_PACKETS; i ++) {
        volatile struct ethernet_rx_buffer_descriptor *buffer_desc_p =
            &mac_var_p->rx_buffer_descriptors[i];

        struct network_packet *rx_packet_p = &layer2_end_point_p->rx_packets[i];

        D_ASSERT(rx_packet_p->signature == NET_RX_PACKET_SIGNATURE);
        rx_packet_p->state_flags = NET_PACKET_IN_RX_TRANSIT;
        rx_packet_p->rx_buf_desc_p = buffer_desc_p;
        buffer_desc_p->data_buffer = rx_packet_p->data_buffer;

        D_ASSERT((uintptr_t)buffer_desc_p->data_buffer %
                 NET_PACKET_DATA_BUFFER_ALIGNMENT == 0);

        buffer_desc_p->data_length = NET_PACKET_DATA_BUFFER_SIZE;

        /*
         * Set the wrap flag for the last buffer of the ring:
         */
        if (i != NET_MAX_RX_PACKETS - 1) {
            buffer_desc_p->control = 0;
        } else {
            buffer_desc_p->control = ENET_RX_BD_WRAP_MASK;
        }

        /*
         * Mark buffer descriptor as "available for reception":
         */
        buffer_desc_p->control |= ENET_RX_BD_EMPTY_MASK;

        /*
         * Enable generation of receive interrupts
         */
        buffer_desc_p->control_extend1 = ENET_RX_BD_GENERATE_INTERRUPT_MASK;
    }

    /*
     * The Rx descriptor ring is full:
     */
    mac_var_p->rx_ring_entries_filled = NET_MAX_RX_PACKETS;
    mac_var_p->rx_ring_write_cursor = &mac_var_p->rx_buffer_descriptors[0];
    mac_var_p->rx_ring_read_cursor = &mac_var_p->rx_buffer_descriptors[0];
}


static void ethernet_mac_reset(const struct ethernet_mac_device *ethernet_mac_p)
{
    uint32_t reg_value;
    uint_fast16_t polling_count;
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;

    /*
     * Reset Ethernet MAC module:
     */
    WRITE_MMIO_REGISTER(&mac_regs_p->ECR, ENET_ECR_RESET_MASK);

    /*
     * Wait for reset to complete:
     */
    polling_count = ETHERNET_MAC_RESET_MAX_POLLING_COUNT;
    do {
        reg_value = READ_MMIO_REGISTER(&mac_regs_p->ECR);
        polling_count --;
    } while ((reg_value & ENET_ECR_RESET_MASK) != 0 && polling_count != 0);

    if (reg_value & ENET_ECR_RESET_MASK) {
        error_t error = CAPTURE_ERROR("Enet reset failed", ethernet_mac_p,
                                       reg_value);

        fatal_error_handler(error);
        /*UNREACHABLE*/
    }

    D_ASSERT((reg_value & ENET_ECR_ETHEREN_MASK) == 0);
}


static void set_mac_address(const struct ethernet_mac_device *ethernet_mac_p,
                            const struct ethernet_mac_address *mac_address_p)
{
    uint32_t reg_value;
    ENET_Type *mac_regs_p = ethernet_mac_p->mmio_registers_p;
    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;

    mac_var_p->mac_address = *mac_address_p;

    /*
     * Program the MAC address:
     */
    reg_value = mac_var_p->mac_address.bytes[0] << 24 |
                mac_var_p->mac_address.bytes[1] << 16 |
                mac_var_p->mac_address.bytes[2] << 8 |
                mac_var_p->mac_address.bytes[3];
    WRITE_MMIO_REGISTER(&mac_regs_p->PALR, reg_value);

    reg_value = mac_var_p->mac_address.bytes[4] << 24 |
                mac_var_p->mac_address.bytes[5] << 16;
    WRITE_MMIO_REGISTER(&mac_regs_p->PAUR, reg_value);
}


static void ethernet_mac_tx_init(const struct ethernet_mac_device *ethernet_mac_p)
{
    uint32_t reg_value;
    ENET_Type *mac_regs_p = ethernet_mac_p->mmio_registers_p;

    /*
     * Configure transmit control register:
     * - Automatically write the source MAC address (SA) to Ethernet frame
     *   header in the Tx buffer, using the address programmed in the PALR/PAUR
     *   registers
     * - Enable full duplex mode
     */
    reg_value = READ_MMIO_REGISTER(&mac_regs_p->TCR);
    reg_value |= ENET_TCR_ADDINS_MASK | ENET_TCR_FDEN_MASK;
    WRITE_MMIO_REGISTER(&mac_regs_p->TCR, reg_value);

    /*
     * Set the transmit inter-packet gap
     */
    reg_value = READ_MMIO_REGISTER(&mac_regs_p->TIPG);
    SET_BIT_FIELD(reg_value, ENET_TIPG_IPG_MASK, ENET_TIPG_IPG_SHIFT, 12);
    WRITE_MMIO_REGISTER(&mac_regs_p->TIPG, reg_value);

    /*
     * Set pause duration to 0:
     */
    reg_value = READ_MMIO_REGISTER(&mac_regs_p->OPD);
    SET_BIT_FIELD(reg_value, ENET_OPD_PAUSE_DUR_MASK, ENET_OPD_PAUSE_DUR_SHIFT,
                  0);
    WRITE_MMIO_REGISTER(&mac_regs_p->OPD, reg_value);

    /*
     * Set Tx accelerators:
     * - Enable IP header checksum offload
     *   (automatically insert IP header checksum)
     * - Enable layer-4 checksum offload (for TCP, UDP, ICMP)
     *   (automatically insert layer-4 checksum)
     * - Enable Tx FIFO shift 16, so that the data payload of
     *   an outgoing Ethernet frame can be 32-bit aligned in memory.
     *   (Like if 2 dummy bytes were added at the beginning of the
     *   14-byte long Ethernet header. However, with the SHIFT16 flag,
     *   the 2 dummy bytes are not transmitted on the wire)
     */

    reg_value = 0;

#   ifdef ENET_CHECKSUM_OFFLOAD
    reg_value = ENET_TACC_PROCHK_MASK | ENET_TACC_IPCHK_MASK;
#   endif

#   ifdef ENET_DATA_PAYLOAD_32_BIT_ALIGNED
    reg_value |= ENET_TACC_SHIFT16_MASK;
#   endif

    WRITE_MMIO_REGISTER(&mac_regs_p->TACC, reg_value);

    /*
     * Configure Tx FIFO:
     * - Set store and forward mode
     * - Set Tx FIFO section empty threshold to 0 (reset default)
     * - Set Tx FIFO almost empty threshold to 4 (reset default)
     * - Set Tx FIFO almost full threshold to 8 (reset default)
     */
    WRITE_MMIO_REGISTER(&mac_regs_p->TFWR, ENET_TFWR_STRFWD_MASK);
    WRITE_MMIO_REGISTER(&mac_regs_p->TSEM, 0);
    WRITE_MMIO_REGISTER(&mac_regs_p->TAEM, 4);
    WRITE_MMIO_REGISTER(&mac_regs_p->TAFL, 8);

    /*
     * Enable Tx interrupts in the interrupt controller (NVIC):
     */
    nvic_setup_irq(ethernet_mac_p->tx_irq_num, ETHERNET_MAC_TX_INTERRUPT_PRIORITY);
}


static void ethernet_mac_rx_init(const struct ethernet_mac_device *ethernet_mac_p)
{
    uint32_t reg_value;
    ENET_Type *mac_regs_p = ethernet_mac_p->mmio_registers_p;

    /*
     * Configure receive control register:
     * - Enable stripping of CRC field for incoming frames
     * - ???Enable frame padding remove for incoming frames
     * - Enable flow control
     * - Configure RMII interface to the Ethernet PHY
     * - Enable 100Mbps operation
     * - Disable internal loopback
     * - Set max incoming frame length (including CRC)
     */
    reg_value = READ_MMIO_REGISTER(&mac_regs_p->RCR);
    reg_value |= ENET_RCR_CRCFWD_MASK;
    //???reg_value |= ENET_RCR_PADEN_MASK;
    reg_value |= ENET_RCR_FCE_MASK;
    reg_value |= ENET_RCR_MII_MODE_MASK | ENET_RCR_RMII_MODE_MASK;

#if 0  /* Promiscuous mode is useful for debugging */
    reg_value |= ENET_RCR_PROM_MASK;
#endif

    reg_value &= ~ENET_RCR_RMII_10T_MASK;
    reg_value &= ~ENET_RCR_LOOP_MASK;
    SET_BIT_FIELD(reg_value, ENET_RCR_MAX_FL_MASK, ENET_RCR_MAX_FL_SHIFT,
                  ETHERNET_MAX_FRAME_SIZE);
    WRITE_MMIO_REGISTER(&mac_regs_p->RCR, reg_value);

    /*
     * Set receive frame truncate length (use reset value 0x7ff):
     *
     * GERMAN: Should this be changed to ENET_MAX_FRAME_SIZE?
     */
    reg_value = read_32bit_mmio_register(&mac_regs_p->FTRL);
    SET_BIT_FIELD(reg_value, ENET_FTRL_TRUNC_FL_MASK, ENET_FTRL_TRUNC_FL_SHIFT,
                  2047);
    WRITE_MMIO_REGISTER(&mac_regs_p->FTRL, reg_value);

    /*
     * Set Rx accelerators:
     * - Enable padding removal for short IP frames
     * - Enable discard of frames with MAC layer errors
     * - Enable IP header checksum offload
     *   (automatically discard frames with wrong IP header checksum)
     * - Enable layer-4 checksum offload for TCP, UDP, ICMP
     *   (automatically discard frames with wrong layer-4 checksum)
     * - Enable Rx FIFO shift 16, so that the data payload of
     *   an incoming Ethernet frame can be 32-bit aligned in memory.
     *   (Like if 2 dummy bytes were added at the beginning of the
     *    14-byte long Ethernet header, right after the frame is
     *    received.)
     */

    reg_value = ENET_RACC_PADREM_MASK | ENET_RACC_LINEDIS_MASK;

#   ifdef ENET_CHECKSUM_OFFLOAD
    reg_value |= ENET_RACC_IPDIS_MASK | ENET_RACC_PRODIS_MASK;
#   endif

#   ifdef ENET_DATA_PAYLOAD_32_BIT_ALIGNED
    reg_value |= ENET_RACC_SHIFT16_MASK;
#   endif

    WRITE_MMIO_REGISTER(&mac_regs_p->RACC, reg_value);

    /*
     * Configure Rx FIFO:
     * - Set Rx FIFO section full threshold to 0 (reset default)
     * - Set Rx FIFO section empty threshold to 0 (reset default)
     * - Set Rx FIFO almost empty threshold to 4 (reset default)
     * - Set Rx FIFO almost full threshold to 4 (reset default)
     */
    WRITE_MMIO_REGISTER(&mac_regs_p->RSFL, 0);
    WRITE_MMIO_REGISTER(&mac_regs_p->RSEM, 0);
    WRITE_MMIO_REGISTER(&mac_regs_p->RAEM, 4);
    WRITE_MMIO_REGISTER(&mac_regs_p->RAFL, 4);

    /*
     * Enable Rx interrupts in the interrupt controller (NVIC):
     */
    nvic_setup_irq(ethernet_mac_p->rx_irq_num, ETHERNET_MAC_RX_INTERRUPT_PRIORITY);
}


/**
 * Initializes an Ethernet MAC module
 *
 * @param ethernet_mac_p        Pointer to the Ethernet MAC device
 * @param layer2_end_point_p     Pointer to the local layer-2 end point
 *                                 to be associated with this MAC
 */
void ethernet_mac_init(const struct ethernet_mac_device *ethernet_mac_p,
                       struct net_layer2_end_point *layer2_end_point_p)
{
    uint32_t reg_value;
    ENET_Type *mac_regs_p = ethernet_mac_p->mmio_registers_p;
    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;

    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);
    D_ASSERT(!mac_var_p->initialized);
    D_ASSERT(mac_var_p->layer2_end_point_p == NULL);

    mac_var_p->layer2_end_point_p = layer2_end_point_p;

    /*
     * Enable the Clock to the ENET Module
     */
    reg_value = READ_MMIO_REGISTER(&SIM_SCGC2);
    reg_value |= ethernet_mac_p->clock_gate_mask;
    WRITE_MMIO_REGISTER(&SIM_SCGC2, reg_value);

    /*
     * Configure GPIO pins for Ethernet IEEE 1588 timer functions:
     */
    for (uint_fast8_t i = 0;
         i < ARRAY_SIZE(ethernet_mac_p->ieee_1588_timer_pins);
         ++ i) {
        set_pin_function(&ethernet_mac_p->ieee_1588_timer_pins[i], 0);
    }

    ethernet_mac_reset(ethernet_mac_p);

    /*
     * Disable generation of interrupts:
     */
    WRITE_MMIO_REGISTER(&mac_regs_p->EIMR, 0x0);

    /*
     * Clear pending interrupts:
     */
    WRITE_MMIO_REGISTER(&mac_regs_p->EIR, MULTI_BIT_MASK(30, 0));

    /*
     * Clear multicast group and individual hash registers
     */
    WRITE_MMIO_REGISTER(&mac_regs_p->GALR, 0x0);
    WRITE_MMIO_REGISTER(&mac_regs_p->GAUR, 0x0);
    WRITE_MMIO_REGISTER(&mac_regs_p->IALR, 0x0);
    WRITE_MMIO_REGISTER(&mac_regs_p->IAUR, 0x0);

    set_mac_address(ethernet_mac_p, &layer2_end_point_p->mac_address);

    /*
     * - Enable normal operating mode (Disable sleep mode)
     * - Enable buffer descriptor byte swapping
     *   (since ARM Cortex-M is little-endian):
     * - Enable enhanced frame time-stamping functions
     */
    reg_value = READ_MMIO_REGISTER(&mac_regs_p->ECR);
    reg_value &= ~ENET_ECR_SLEEP_MASK;
    reg_value |= ENET_ECR_DBSWP_MASK | ENET_ECR_EN1588_MASK;
    WRITE_MMIO_REGISTER(&mac_regs_p->ECR, reg_value);

    ethernet_mac_tx_init(ethernet_mac_p);
    ethernet_mac_rx_init(ethernet_mac_p);

    /*
     * Disable MIB counters:
     */
    WRITE_MMIO_REGISTER(&mac_regs_p->MIBC,
                        ENET_MIBC_MIB_DIS_MASK | ENET_MIBC_MIB_CLEAR_MASK);
    WRITE_MMIO_REGISTER(&mac_regs_p->MIBC, 0x0);

    /*
     * Enable error interrupts in the interrupt controller (NVIC):
     */
    nvic_setup_irq(ethernet_mac_p->error_irq_num, ETHERNET_MAC_ERROR_INTERRUPT_PRIORITY);

    ethernet_phy_init(ethernet_mac_p->ethernet_phy_p);

    crc_32_accelerator_init();

    mac_var_p->initialized = true;
    DEBUG_PRINTF("Ethernet MAC: Initialized MAC %s\n",
                 ethernet_mac_p->name_p);
}


/**
 * Activates an Ethernet MAC module
 *
 * @param ethernet_mac_p        Pointer to the Ethernet MAC device
 */
void ethernet_mac_start(const struct ethernet_mac_device *ethernet_mac_p)
{
    uint32_t reg_value;
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;
    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;

#ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(enet_var_p,
                                sizeof *enet_var_p,
                                0,
                                &old_comp_region);
#endif

    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);
    D_ASSERT(mac_var_p->initialized);
    D_ASSERT(mac_var_p->layer2_end_point_p != NULL);

#ifdef USE_MPU
    bool caller_was_privileged = rtos_enter_privileged_mode();

    /*
     * Enable access to Rx/Tx rings memory for the ENET DMA engine:
     */
    mpu_register_dma_region(MPU_BUS_MASTER_ENET, mac_var_p, sizeof *mac_var_p);
#else
    mpu_disable();
#endif

    /*
     * Initialize Tx buffer descriptor ring:
     */
    ethernet_mac_tx_buffer_descriptor_ring_init(ethernet_mac_p);

    /*
     * Initialize Rx buffer descriptor ring:
     */
    ethernet_mac_rx_buffer_descriptor_ring_init(ethernet_mac_p);

    uint32_t int_mask = disable_cpu_interrupts();

    /*
     * Enable generation of Tx/Rx interrupts:
     * - Generate Tx interrupt when a frame has been transmitted (the
     *   last and only Tx buffer descriptor of the frame has been updated)
     * - Generate Rx interrupt when a frame has been received (the
     *   (last and only Rx buffer descriptor of the frame has been updated)
     */
    WRITE_MMIO_REGISTER(&mac_regs_p->EIMR,
                        ENET_EIMR_TXF_MASK |
                        ENET_EIMR_RXF_MASK |
                        ENET_EIMR_BABR_MASK |
                        ENET_EIMR_BABT_MASK |
                        ENET_EIMR_EBERR_MASK |
                        ENET_EIMR_UN_MASK |
                        ENET_EIMR_PLR_MASK);

    /*
     * Enable ENET module:
     */
    reg_value = READ_MMIO_REGISTER(&mac_regs_p->ECR);
    reg_value |= ENET_ECR_ETHEREN_MASK;
    WRITE_MMIO_REGISTER(&mac_regs_p->ECR, reg_value);

    /*
     * Activate Rx buffer descriptor ring:
     * (the Rx descriptor ring must have at least one descriptor with the "empty"
     *  bit set in its control field)
     *
     *  NOTE: This must be done after enabling the ENET module.
     */
    __DSB();
    WRITE_MMIO_REGISTER(&mac_regs_p->RDAR, ENET_RDAR_RDAR_MASK);

    restore_cpu_interrupts(int_mask);

    DEBUG_PRINTF("Ethernet MAC: Started MAC %s\n", ethernet_mac_p->name_p);

#   ifdef USE_MPU
    if (!caller_was_privileged) {
        rtos_exit_privileged_mode();
    }

    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Add a multicast MAC address to the given Ethernet device
 */
void ethernet_mac_add_multicast_addr(const struct ethernet_mac_device *ethernet_mac_p,
                                     struct ethernet_mac_address *mac_addr_p)
{
    uint32_t reg_value;
    uint32_t hash_bit_index;
    volatile uint32_t *reg_p;

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(enet_var_p,
                                sizeof *enet_var_p,
                                0,
                                &old_comp_region);
#    endif

    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);
    D_ASSERT(mac_addr_p->bytes[0] & MAC_MULTICAST_ADDRESS_MASK);

    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;
    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;

    D_ASSERT(mac_var_p->initialized);

#   ifdef USE_MPU
    bool caller_was_privileged = rtos_enter_privileged_mode();
#   endif

    uint32_t crc = crc_32_accelerator_run(mac_addr_p, sizeof *mac_addr_p);
    uint32_t hash_value = crc >> 26; /* top 6 bits */

    D_ASSERT(hash_value < ETHERNET_MAC_MULTICAST_HASH_TABLE_NUM_BUCKETS);
    mac_var_p->multicast_hash_table_counts[hash_value] ++;

    D_ASSERT(mac_var_p->multicast_hash_table_counts[hash_value] != 0);

    /*
     * Select either GAUR or GARL from the top bit of the hash value:
     */
    if (hash_value & BIT(5)) {
        reg_p = &mac_regs_p->GAUR;
        hash_bit_index = hash_value & ~BIT(5);
    } else {
        reg_p = &mac_regs_p->GALR;
        hash_bit_index = hash_value;
    }

    D_ASSERT(hash_bit_index < 32);

    /*
     * Set hash bit in GAUR or GALR, if not set already:
     */
    reg_value = READ_MMIO_REGISTER(reg_p);
    if ((reg_value & BIT(hash_bit_index)) == 0) {
        reg_value |= BIT(hash_bit_index);
        WRITE_MMIO_REGISTER(reg_p, reg_value);
    }

#   ifdef USE_MPU
    if (!caller_was_privileged) {
        rtos_exit_privileged_mode();
    }

    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Remove a multicast MAC address from the given Ethernet device
 */
void ethernet_mac_remove_multicast_addr(const struct ethernet_mac_device *ethernet_mac_p,
                                        struct ethernet_mac_address *mac_addr_p)
{
    uint32_t reg_value;
    volatile uint32_t *reg_p;
    uint32_t hash_bit_index;

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(enet_var_p,
                                sizeof *enet_var_p,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);

    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;
    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;

    D_ASSERT(mac_var_p->initialized);

    uint32_t crc = crc_32_accelerator_run(mac_addr_p, sizeof *mac_addr_p);
    uint32_t hash_value = crc >> 26; /* top 6 bits */

    D_ASSERT(hash_value < ETHERNET_MAC_MULTICAST_HASH_TABLE_NUM_BUCKETS);
    D_ASSERT(mac_var_p->multicast_hash_table_counts[hash_value] != 0);

    mac_var_p->multicast_hash_table_counts[hash_value] --;

    /*
     * Select either GAUR or GARL from the top bit of the hash value:
     */
    if (hash_value & BIT(5)) {
        reg_p = &mac_regs_p->GAUR;
        hash_bit_index = hash_value & ~BIT(5);
    } else {
        reg_p = &mac_regs_p->GALR;
        hash_bit_index = hash_value;
    }

    D_ASSERT(hash_bit_index < 32);

    /*
     * Clear hash bit in GAUR or GALR, if hash bucket became empty:
     */
    reg_value = READ_MMIO_REGISTER(reg_p);
    if (mac_var_p->multicast_hash_table_counts[hash_value] == 0) {
        reg_value &= ~BIT(hash_bit_index);
        WRITE_MMIO_REGISTER(reg_p, reg_value);
    }

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Remove Tx buffer descriptors from the Tx ring, for those network packets
 * that have already been transmitted, and return those packets to the pool
 * of free Tx packets.
 */
static void ethernet_mac_drain_tx_ring(struct ethernet_mac_device_var *mac_var_p)
{
    volatile struct ethernet_tx_buffer_descriptor *buffer_desc_p =
        mac_var_p->tx_ring_read_cursor;

    do {
        D_ASSERT(buffer_desc_p >= &mac_var_p->tx_buffer_descriptors[0] &&
                 buffer_desc_p <=
                   &mac_var_p->tx_buffer_descriptors[NET_MAX_TX_PACKETS - 1]);

        D_ASSERT(buffer_desc_p != mac_var_p->tx_ring_write_cursor ||
                 mac_var_p->tx_ring_entries_filled == NET_MAX_TX_PACKETS);

        D_ASSERT((buffer_desc_p->control &
                  (ENET_TX_BD_LAST_IN_FRAME_MASK | ENET_TX_BD_CRC_MASK)) ==
                 (ENET_TX_BD_LAST_IN_FRAME_MASK | ENET_TX_BD_CRC_MASK));

        if (buffer_desc_p->control & ENET_TX_BD_READY_MASK) {
            break;
        }

        struct network_packet *tx_packet_p =
            BUFFER_TO_NETWORK_PACKET(buffer_desc_p->data_buffer);

        D_ASSERT(tx_packet_p->signature == NET_TX_PACKET_SIGNATURE);
        D_ASSERT(tx_packet_p->tx_buf_desc_p == buffer_desc_p);
        D_ASSERT(tx_packet_p->state_flags & NET_PACKET_IN_TX_TRANSIT);
        D_ASSERT(tx_packet_p->state_flags & NET_PACKET_IN_TX_USE_BY_APP);

#       if 0
        DEBUG_PRINTF("Ethernet MAC: Transmitted packet %#x\n", tx_packet_p);
#       endif

        tx_packet_p->state_flags &= ~NET_PACKET_IN_TX_TRANSIT;
        tx_packet_p->tx_buf_desc_p = NULL;
        buffer_desc_p->data_buffer = NULL;
        buffer_desc_p->control_extend1 &= ~ENET_TX_BD_INTERRUPT_MASK;
        if (buffer_desc_p->control_extend0 &
            (ENET_TX_BD_ERROR_MASK |
             ENET_TX_BD_FIFO_OVERFLOW_ERROR_MASK |
             ENET_TX_BD_TMESTAMP_ERROR_MASK |
             ENET_TX_BD_FRAME_ERROR_MASK)) {
             ERROR_PRINTF("Ethernet transmission failed (Tx packet dropped): "
                          "control_extended: %#x, buffer_desc: %#x\n",
                          buffer_desc_p->control_extend0, buffer_desc_p);
        }

        if (tx_packet_p->state_flags & NET_PACKET_FREE_AFTER_TX_COMPLETE) {
            /*
             * Free transmitted packet:
             */
            tx_packet_p->state_flags &= ~NET_PACKET_FREE_AFTER_TX_COMPLETE;
            net_layer2_free_tx_packet(tx_packet_p);
        }

        if (buffer_desc_p->control & ENET_TX_BD_WRAP_MASK) {
            buffer_desc_p = &mac_var_p->tx_buffer_descriptors[0];
        } else {
            buffer_desc_p ++;
        }

        mac_var_p->tx_ring_entries_filled --;
    } while (mac_var_p->tx_ring_entries_filled != 0);

    mac_var_p->tx_ring_read_cursor = buffer_desc_p;
}


/**
 * Transmit completion interrupt handler
 */
static void ethernet_mac_tx_irq_handler(const struct ethernet_mac_device *ethernet_mac_p)
{
    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);

    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;

    for ( ; ; ) {
        uint32_t reg_value = READ_MMIO_REGISTER(&mac_regs_p->EIR);

        if ((reg_value & ENET_EIR_TXF_MASK) == 0) {
            break;
        }

        /*
         * Clear interrupt source (w1c):
         */
        WRITE_MMIO_REGISTER(&mac_regs_p->EIR, ENET_EIR_TXF_MASK);

        uint32_t int_mask = disable_cpu_interrupts();

        D_ASSERT(mac_var_p->tx_ring_entries_filled <= NET_MAX_TX_PACKETS);
        D_ASSERT(mac_var_p->tx_ring_read_cursor != NULL);

        if (mac_var_p->tx_ring_entries_filled == 0) {
            restore_cpu_interrupts(int_mask);
            break;
        }

        ethernet_mac_drain_tx_ring(mac_var_p);
        restore_cpu_interrupts(int_mask);
    }
}


/**
 * ISR for the Ethernet MAC0's Tx interrupt
 */
void ethernet_mac0_tx_irq_handler(void)
{
    D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

    rtos_enter_isr();
    ethernet_mac_tx_irq_handler(&g_ethernet_mac0);
    rtos_exit_isr();
}


/**
 * Remove Rx buffer descriptors from the Rx ring, for those network packets
 * that have already been received, and enqueue those packets at the corresponding
 * layer2 end point's Rx packet queue.
 */
static void ethernet_mac_drain_rx_ring(struct ethernet_mac_device_var *mac_var_p)
{
    volatile struct ethernet_rx_buffer_descriptor *buffer_desc_p =
        mac_var_p->rx_ring_read_cursor;

    do {
        D_ASSERT(buffer_desc_p >= &mac_var_p->rx_buffer_descriptors[0] &&
                 buffer_desc_p <=
                   &mac_var_p->rx_buffer_descriptors[NET_MAX_RX_PACKETS - 1]);

        D_ASSERT(buffer_desc_p != mac_var_p->rx_ring_write_cursor ||
                 mac_var_p->rx_ring_entries_filled == NET_MAX_RX_PACKETS);

        if (buffer_desc_p->control & ENET_RX_BD_EMPTY_MASK) {
            break;
        }

        D_ASSERT(buffer_desc_p->control & ENET_RX_BD_LAST_IN_FRAME_MASK);

        struct network_packet *rx_packet_p =
            BUFFER_TO_NETWORK_PACKET(buffer_desc_p->data_buffer);

        D_ASSERT(rx_packet_p->signature == NET_RX_PACKET_SIGNATURE);
        D_ASSERT(rx_packet_p->state_flags & NET_PACKET_IN_RX_TRANSIT);
        D_ASSERT(!(rx_packet_p->state_flags & NET_PACKET_IN_RX_USE_BY_APP));

        rx_packet_p->state_flags &= ~NET_PACKET_IN_RX_TRANSIT;
        rx_packet_p->rx_buf_desc_p = NULL;
        buffer_desc_p->data_buffer = NULL;
        buffer_desc_p->control_extend1 &= ~ENET_RX_BD_GENERATE_INTERRUPT_MASK;
        if (buffer_desc_p->control &
            (ENET_RX_BD_LENGTH_VIOLATION_MASK |
             ENET_RX_BD_NON_OCTET_ALIGNED_FRAME_MASK |
             ENET_RX_BD_CRC_ERROR_MASK |
             ENET_RX_BD_FIFO_OVERRRUN_MASK |
             ENET_RX_BD_FRAME_TRUNCATED_MASK)) {
            ERROR_PRINTF("Received bad frame (Rx packet dropped): "
                          "control: %#x, buffer_desc: %#x\n",
                          buffer_desc_p->control, buffer_desc_p);

            rx_packet_p->state_flags = NET_PACKET_RX_FAILED;
        } else {
            D_ASSERT(buffer_desc_p->data_length <= ETHERNET_MAX_FRAME_DATA_SIZE);
            rx_packet_p->total_length = buffer_desc_p->data_length;
        }

#       if 0
        DEBUG_PRINTF("Ethernet MAC: Received packet %#x (type %#x, state_flags %#x)\n",
                     rx_packet_p, buffer_desc_p->protocol_type, rx_packet_p->state_flags);
#       endif

        /*
         * Enqueue received packet at the corresponding layer-2 end point:
         */
        net_layer2_enqueue_rx_packet(mac_var_p->layer2_end_point_p, rx_packet_p);

        if (buffer_desc_p->control & ENET_RX_BD_WRAP_MASK) {
            buffer_desc_p = &mac_var_p->rx_buffer_descriptors[0];
        } else {
            buffer_desc_p ++;
        }

        mac_var_p->rx_ring_entries_filled --;
    } while (mac_var_p->rx_ring_entries_filled != 0);

    mac_var_p->rx_ring_read_cursor = buffer_desc_p;
}


/**
 * Receive completion interrupt handler
 */
static void ethernet_mac_rx_irq_handler(const struct ethernet_mac_device *ethernet_mac_p)
{
    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);

    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;

    for ( ; ; ) {
        uint32_t reg_value = READ_MMIO_REGISTER(&mac_regs_p->EIR);

        if ((reg_value & ENET_EIR_RXF_MASK) == 0) {
            break;
        }

        /*
         * Clear interrupt source (w1c):
         */
        WRITE_MMIO_REGISTER(&mac_regs_p->EIR, ENET_EIR_RXF_MASK);

        uint32_t int_mask = disable_cpu_interrupts();

        D_ASSERT(mac_var_p->rx_ring_entries_filled <= NET_MAX_RX_PACKETS);
        D_ASSERT(mac_var_p->rx_ring_read_cursor != NULL);
        if (mac_var_p->rx_ring_entries_filled == 0) {
            restore_cpu_interrupts(int_mask);
            break;
        }

        ethernet_mac_drain_rx_ring(mac_var_p);
        restore_cpu_interrupts(int_mask);
    }
}


/**
 * ISR for the Ethernet MAC0's Rx interrupt
 */
void ethernet_mac0_rx_irq_handler(void)
{
    D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

    rtos_enter_isr();
    ethernet_mac_rx_irq_handler(&g_ethernet_mac0);
    rtos_exit_isr();
}


/**
 * Transmission error interrupt handler
 */
static void ethernet_mac_error_irq_handler(const struct ethernet_mac_device *ethernet_mac_p)
{
    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);

    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;
    uint32_t reg_value = READ_MMIO_REGISTER(&mac_regs_p->EIR);

    uint32_t error_interrupt_mask = (reg_value &
                                     (ENET_EIMR_BABR_MASK |
                                      ENET_EIMR_BABT_MASK |
                                      ENET_EIMR_EBERR_MASK |
                                      ENET_EIMR_UN_MASK |
                                      ENET_EIMR_PLR_MASK));

    if (error_interrupt_mask != 0) {
        ERROR_PRINTF("Ethernet MAC %s error (error interrupt mask: %#x)\n",
                     ethernet_mac_p->name_p, error_interrupt_mask);

        /*
         * Clear interrupt source (w1c):
         */
        WRITE_MMIO_REGISTER(&mac_regs_p->EIR, error_interrupt_mask);
        mac_var_p->tx_rx_error_count ++;
    }
}


/**
 * ISR for the Ethernet MAC0's error interrupt
 */
void ethernet_mac0_error_irq_handler(void)
{
    D_ASSERT(CPU_INTERRUPTS_ARE_ENABLED());

    rtos_enter_isr();
    ethernet_mac_error_irq_handler(&g_ethernet_mac0);
    rtos_exit_isr();
}


/**
 * Initiates the transmission of a Tx packet, by assigning it to the next
 * available Tx descriptor in the Tx descriptor ring, marking that descriptor
 * as "ready" and re-activating the Tx descriptor ring.
 */
void ethernet_mac_start_xmit(const struct ethernet_mac_device *ethernet_mac_p,
                             struct network_packet *tx_packet_p)
{
    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;

    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);
    D_ASSERT(tx_packet_p->signature == NET_TX_PACKET_SIGNATURE);

#   ifdef USE_MPU
    bool caller_was_privileged = rtos_enter_privileged_mode();
#    endif

    uint32_t int_mask = disable_cpu_interrupts();

    D_ASSERT(mac_var_p->tx_ring_entries_filled < NET_MAX_TX_PACKETS);
    D_ASSERT(mac_var_p->tx_ring_write_cursor != mac_var_p->tx_ring_read_cursor ||
             mac_var_p->tx_ring_entries_filled == 0);

    volatile struct ethernet_tx_buffer_descriptor *tx_buf_desc_p =
        mac_var_p->tx_ring_write_cursor;

    D_ASSERT((tx_buf_desc_p->control & ENET_TX_BD_READY_MASK) == 0);
    D_ASSERT(tx_buf_desc_p->data_buffer == NULL);
    D_ASSERT(tx_packet_p->tx_buf_desc_p == NULL);

    tx_buf_desc_p->data_buffer = tx_packet_p->data_buffer;
    tx_packet_p->tx_buf_desc_p = tx_buf_desc_p;

    D_ASSERT(tx_packet_p->state_flags & NET_PACKET_IN_TX_USE_BY_APP);
    D_ASSERT(tx_packet_p->total_length != 0);

    tx_buf_desc_p->data_length = tx_packet_p->total_length;
    tx_buf_desc_p->control_extend1 |= ENET_TX_BD_INTERRUPT_MASK;

    D_ASSERT(!(tx_packet_p->state_flags & NET_PACKET_IN_TX_TRANSIT));

    tx_packet_p->state_flags |= NET_PACKET_IN_TX_TRANSIT;

    /*
     * Mark buffer descriptor as "ready for transmission":
     */
    tx_buf_desc_p->control |= ENET_TX_BD_READY_MASK;

    /*
     * Advance Tx ring write cursor:
     */
    if (tx_buf_desc_p->control & ENET_TX_BD_WRAP_MASK) {
        mac_var_p->tx_ring_write_cursor = mac_var_p->tx_buffer_descriptors;
    } else {
        mac_var_p->tx_ring_write_cursor ++;
    }

    mac_var_p->tx_ring_entries_filled ++;
    restore_cpu_interrupts(int_mask);

    /*
     * Re-activate Tx buffer descriptor ring, to start transmitting the frame:
     * (the Tx descriptor ring has at least one descriptor with the "ready"
     *  bit set in its control field)
     */
    __DSB();
    WRITE_MMIO_REGISTER(&mac_regs_p->TDAR, ENET_TDAR_TDAR_MASK);

#   if 0
    if (!ethernet_phy_link_is_up(ethernet_mac_p->ethernet_phy_p)) {
        DEBUG_PRINTF("Ethernet MAC: link down for MAC %s\n", ethernet_mac_p->name_p);
    }
#   endif

#   ifdef USE_MPU
    if (!caller_was_privileged) {
        rtos_exit_privileged_mode();
    }
#   endif
}


/**
 * Re-post the given Rx packet to the Ethernet MAC's Rx ring, by assigning it to
 * the next available Rx descriptor in the Rx descriptor ring, marking that
 * descriptor as "empty" and re-activating the Rx descriptor ring.
 */
void ethernet_mac_repost_rx_packet(const struct ethernet_mac_device *ethernet_mac_p,
                                   struct network_packet *rx_packet_p)
{
    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);

    struct ethernet_mac_device_var *const mac_var_p = ethernet_mac_p->var_p;
    ENET_Type *const mac_regs_p = ethernet_mac_p->mmio_registers_p;

    D_ASSERT(rx_packet_p->signature == NET_RX_PACKET_SIGNATURE);
    D_ASSERT(rx_packet_p->state_flags == NET_PACKET_IN_RX_USE_BY_APP ||
             rx_packet_p->state_flags == NET_PACKET_RX_FAILED);

#   ifdef USE_MPU
    bool caller_was_privileged = rtos_enter_privileged_mode();
#   endif

    uint32_t int_mask = disable_cpu_interrupts();

    D_ASSERT(mac_var_p->rx_ring_entries_filled < NET_MAX_RX_PACKETS);

    volatile struct ethernet_rx_buffer_descriptor *rx_buf_desc_p =
        mac_var_p->rx_ring_write_cursor;

    D_ASSERT((rx_buf_desc_p->control & ENET_RX_BD_EMPTY_MASK) == 0);
    D_ASSERT(rx_buf_desc_p->data_buffer == NULL);
    D_ASSERT(rx_packet_p->rx_buf_desc_p == NULL);

    rx_buf_desc_p->data_buffer = rx_packet_p->data_buffer;
    rx_packet_p->rx_buf_desc_p = rx_buf_desc_p;
    rx_buf_desc_p->control_extend1 |= ENET_RX_BD_GENERATE_INTERRUPT_MASK;

    D_ASSERT(!(rx_packet_p->state_flags & NET_PACKET_IN_RX_TRANSIT));

    rx_packet_p->state_flags = NET_PACKET_IN_RX_TRANSIT;

    /*
     * Mark buffer descriptor as "ready for reception":
     */
    rx_buf_desc_p->control |= ENET_RX_BD_EMPTY_MASK;

    /*
     * Advance Rx ring write cursor:
     */
    if (rx_buf_desc_p->control & ENET_RX_BD_WRAP_MASK) {
        mac_var_p->rx_ring_write_cursor = mac_var_p->rx_buffer_descriptors;
    } else {
        mac_var_p->rx_ring_write_cursor ++;
    }

    mac_var_p->rx_ring_entries_filled ++;
    restore_cpu_interrupts(int_mask);

    /*
     * Re-activate Rx buffer descriptor ring:
     * (the Rx descriptor ring has at least one descriptor with the "empty"
     *  bit set in its control field)
     */
    __DSB();
    WRITE_MMIO_REGISTER(&mac_regs_p->RDAR, ENET_RDAR_RDAR_MASK);

#   ifdef USE_MPU
    if (!caller_was_privileged) {
        rtos_exit_privileged_mode();
    }
#   endif
}

