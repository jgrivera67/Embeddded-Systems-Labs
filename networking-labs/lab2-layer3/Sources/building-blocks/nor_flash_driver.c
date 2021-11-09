/**
 * @file nor_flash_driver.c
 *
 * NOR flash driver implementation
 *
 * @author German Rivera
 */
#include "nor_flash_driver.h"
#include "microcontroller.h"
#include "runtime_checks.h"
#include "io_utils.h"
#include "mem_utils.h"
#include "rtos_wrapper.h"
#include "interrupt_vector_table.h"
#include "cortex_m_startup.h"
#include <MK64F12.h>

enum nor_flash_commands {
#if defined(KL25Z_MCU)
    NOR_FLASH_CMD_PROGRAM_LONG_WORD =    0x06,
#elif defined(K64F_MCU)
    NOR_FLASH_CMD_PROGRAM_SECTION =      0x0b,
#else
#error "No Microcontroller defined"
#endif

    NOR_FLASH_CMD_ERASE_SECTOR =         0x09,
};

/**
 * Const fields of the NOR flash memory device (to be placed in flash)
 */
struct nor_flash_device {
    struct nor_flash_device_var *var_p;
    FTFE_Type *mmio_p;
    const char *semaphore_name;
};


/**
 * Non-const fields of the NOR flash memory device (to be placed in SRAM)
 */
struct nor_flash_device_var {
    /**
     * Flag indicating if the NOR flash device has been initialized for writing
     */
    bool initialized;
};

/**
 * Global non-const structure for the NOR flash memory device
 * (allocated in SRAM space)
 */
static struct nor_flash_device_var g_nor_flash_device_var = {
    .initialized = false,
};

/**
 * Global const structure for the NOR flash memory device of the KL25Z SoC
 * (allocated in flash space)
 */
static const struct nor_flash_device g_nor_flash_device =
{
    .var_p = &g_nor_flash_device_var,
    .mmio_p = FTFE_BASE_PTR,
};


/**
 * Initializes NOR flash memory device for writing to it
 */
void nor_flash_init(void)
{
    struct nor_flash_device_var *const nor_flash_var_p = g_nor_flash_device.var_p;

    nor_flash_var_p->initialized = true;
}


/**
 * Execute NOR flash command currently loaded in the FCCOBx registers.
 *
 * It launches the NOR flash command and waits for its completion
 *
 * NOTE: To prevent a read from NOR flash while a NOR flash is being
 * executed, we need to make sure that the polling loop to wait for
 * command completion runs from RAM instead of flash, to avoid
 * instruction fetches from flash. Also, we disable interrupts to
 * ensure that the polling loop is not preempted by any other code
 * as that may cause instruction fetches from flash.
 *
 * @return 0, on success
 * @return error code, on failure
 */
RAM_FUNC static error_t nor_flash_execute_command(void)
{
    uint32_t reg_value;
    error_t error;
    FTFE_Type *const nor_flash_mmio_p = g_nor_flash_device.mmio_p;

	uint32_t int_mask = disable_cpu_interrupts();

    /*
     * Launch the command by clearing FSTAT register's CCIF bit (w1c):
     */
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FSTAT, FTFE_FSTAT_CCIF_MASK);

    do {
        reg_value = READ_MMIO_REGISTER(&nor_flash_mmio_p->FSTAT);
    } while ((reg_value & FTFE_FSTAT_CCIF_MASK) == 0);

    restore_cpu_interrupts(int_mask);

    if (reg_value & (FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK | FTFE_FSTAT_MGSTAT0_MASK)) {
        error = CAPTURE_ERROR("NOR flash command finished with error",
                              reg_value, 0);
        return error;
    }

    return 0;
}


static error_t nor_flash_erase_sector(uintptr_t addr)
{
    uint32_t reg_value;
    FTFE_Type *const nor_flash_mmio_p = g_nor_flash_device.mmio_p;

    /*
     * Clear error flags (w1c) from previous command, if any:
     */
    reg_value = READ_MMIO_REGISTER(&nor_flash_mmio_p->FSTAT);
    D_ASSERT(reg_value & FTFE_FSTAT_CCIF_MASK);
    if (reg_value & (FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK)) {
        WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FSTAT,
                            FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK);
    }

    /*
     * Populate NOR flash command registers:
     */
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB0, NOR_FLASH_CMD_ERASE_SECTOR);
    reg_value = GET_BIT_FIELD(addr, MULTI_BIT_MASK(23, 16), 16);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB1, reg_value);
    reg_value = GET_BIT_FIELD(addr, MULTI_BIT_MASK(15, 8), 8);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB2, reg_value);
    reg_value = GET_BIT_FIELD(addr, MULTI_BIT_MASK(7, 0), 0);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB3, reg_value);

    return nor_flash_execute_command();
}


#if defined(KL25Z_MCU)
static error_t nor_flash_write_word(uint32_t *dest_word_p, uint32_t word_value)
{
	uint32_t reg_value;
	uintptr_t addr = (uintptr_t)dest_word_p;
    FTFE_Type *const nor_flash_mmio_p = g_nor_flash_device.mmio_p;

    /*
     * Clear error flags (w1c) from previous command, if any:
     */
    reg_value = READ_MMIO_REGISTER(&nor_flash_mmio_p->FSTAT);
    D_ASSERT(reg_value & FTFE_FSTAT_CCIF_MASK);
    if (reg_value & (FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK)) {
        WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FSTAT,
                            FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK);
    }

    /*
     * Populate NOR flash command registers:
     */
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB0, NOR_FLASH_CMD_PROGRAM_LONG_WORD);
    reg_value = GET_BIT_FIELD(addr, MULTI_BIT_MASK(23, 16), 16);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB1, reg_value);
    reg_value = GET_BIT_FIELD(addr, MULTI_BIT_MASK(15, 8), 8);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB2, reg_value);
    reg_value = GET_BIT_FIELD(addr, MULTI_BIT_MASK(7, 0), 0);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB3, reg_value);
    reg_value = GET_BIT_FIELD(word_value, MULTI_BIT_MASK(31, 24), 24);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB4, reg_value);
    reg_value = GET_BIT_FIELD(word_value, MULTI_BIT_MASK(23, 16), 16);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB5, reg_value);
    reg_value = GET_BIT_FIELD(word_value, MULTI_BIT_MASK(15, 8), 8);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB6, reg_value);
    reg_value = GET_BIT_FIELD(word_value, MULTI_BIT_MASK(7, 0), 0);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB7, reg_value);

    return nor_flash_execute_command();
}

#elif defined(K64F_MCU)

/**
 * NOR Flash programming acceleration 4KB buffer address
 */
#define NOR_FLASH_PROG_ACCEL_BUFFER_ADDR    UINT32_C(0x14000000)

/**
 * Programs a section of NOR flash
 */
static error_t nor_flash_program_section(uintptr_t dest_addr, const void *src_addr,
		                                 uint32_t section_size)
{
	uint32_t reg_value;
	uint32_t num_128bit_chunks;
    FTFE_Type *const nor_flash_mmio_p = g_nor_flash_device.mmio_p;

    C_ASSERT(NOR_FLASH_PROG_ACCEL_BUFFER_ADDR % sizeof(uint32_t) == 0);
    D_ASSERT(VALID_RAM_POINTER(src_addr, sizeof(uint32_t)));
    D_ASSERT(section_size % sizeof(uint32_t) == 0);

    /*
     * Copy source section to the NOR flash programming acceleration buffer:
     */
    memcpy32((uint32_t *)NOR_FLASH_PROG_ACCEL_BUFFER_ADDR, (uint32_t *)src_addr, section_size);

    /*
     * Clear error flags (w1c) from previous command, if any:
     */
    reg_value = READ_MMIO_REGISTER(&nor_flash_mmio_p->FSTAT);
    D_ASSERT(reg_value & FTFE_FSTAT_CCIF_MASK);
    if (reg_value & (FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK)) {
        WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FSTAT,
                            FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK);
    }

    /*
     * Populate NOR flash command registers:
     */

    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB0, NOR_FLASH_CMD_PROGRAM_SECTION);

    reg_value = GET_BIT_FIELD(dest_addr, MULTI_BIT_MASK(23, 16), 16);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB1, reg_value);
    reg_value = GET_BIT_FIELD(dest_addr, MULTI_BIT_MASK(15, 8), 8);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB2, reg_value);
    reg_value = GET_BIT_FIELD(dest_addr, MULTI_BIT_MASK(7, 0), 0);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB3, reg_value);

    num_128bit_chunks = HOW_MANY(section_size, 16);
    D_ASSERT(num_128bit_chunks <= UINT16_MAX);
    reg_value = GET_BIT_FIELD(num_128bit_chunks, MULTI_BIT_MASK(15, 8), 8);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB4, reg_value);
    reg_value = GET_BIT_FIELD(num_128bit_chunks, MULTI_BIT_MASK(7, 0), 0);
    WRITE_MMIO_REGISTER(&nor_flash_mmio_p->FCCOB5, reg_value);

    return nor_flash_execute_command();
}

#else
#error "No Microcontroller defined"
#endif

/**
 * Writes a data block to NOR flash at the given address. The write is done
 * in whole flash sectors. The corresponding flash sectors are erased before
 * being written.
 *
 * @param dest_addr Destination address in NOR flash. It must be NOR flash
 *                  sector aligned.
 * @param src_addr  Source address of the data block in RAM. It must be word
 *                     (4 byte) aligned.
 * @param src_size  Size of the data block in bytes. It must be a multiple of
 *                  4 bytes (32-bit word size).
 *
 * @return 0, on success
 * @return error code, on failure
 */
error_t nor_flash_write(uintptr_t dest_addr,
                        const void *src_addr,
                        size_t src_size)
{
    uintptr_t sector_addr;
    error_t error;
    struct nor_flash_device_var *const nor_flash_var_p = g_nor_flash_device.var_p;
    uint32_t num_sectors = HOW_MANY(src_size, NOR_FLASH_SECTOR_SIZE);
    uintptr_t highest_code_address = MCU_FLASH_BASE_ADDR + get_flash_used();

    D_ASSERT(VALID_FLASH_ADDRESS(dest_addr));
    D_ASSERT((dest_addr & (NOR_FLASH_SECTOR_SIZE - 1)) == 0);
    D_ASSERT(VALID_RAM_POINTER(src_addr, sizeof(uint32_t)));
    D_ASSERT(src_size != 0);
    D_ASSERT(nor_flash_var_p->initialized);

    if (dest_addr <= highest_code_address) {
        /*
         * Destination address overlaps with code
         */
        error = CAPTURE_ERROR("NOR flash cannot be written at given address",
                              dest_addr, highest_code_address);
        return error;
    }

    /*
     * Erase sectors to be written:
     */
    sector_addr = dest_addr;
    for (uint32_t i = 0; i < num_sectors; i ++) {
        error = nor_flash_erase_sector(sector_addr);
        if (error != 0) {
            return error;
        }

        sector_addr += NOR_FLASH_SECTOR_SIZE;
    }

    /*
     * Write data to NOR flash:
     */

#if defined(KL25Z_MCU)
    uint32_t num_words = src_size / sizeof(uint32_t);
    const uint32_t *src_word_p = src_addr;
    uint32_t *dest_word_p = (void *)dest_addr;

    D_ASSERT(src_size % sizeof(uint32_t) == 0);
    for (uint32_t i = 0; i < num_words; i ++) {
    	error = nor_flash_write_word(dest_word_p, *src_word_p);
        if (error != 0) {
            return error;
        }

    	dest_word_p ++;
    	src_word_p ++;
    }

#elif defined(K64F_MCU)
    error = nor_flash_program_section(dest_addr, src_addr, src_size);
    if (error != 0) {
        return error;
    }

#else
#error "No Microcontroller defined"
#endif

    return 0;
}
