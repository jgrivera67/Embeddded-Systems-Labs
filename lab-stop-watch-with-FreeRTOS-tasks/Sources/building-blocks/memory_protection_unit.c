/**
 * @file memory_protection_unit.c
 *
 * Memory protection unit services implementation
 *
 * @author: German Rivera
 */
#include "memory_protection_unit.h"
#include "runtime_checks.h"
#include "io_utils.h"
#include "mem_utils.h"
#include "atomic_utils.h"
#include "interrupt_vector_table.h"
#include "rtos_wrapper.h"
#include "stack_trace.h"
#include "cortex_m_startup.h"

#define MAX_NUM_SUB_REGIONS    8

#define LAST_ADDRESS(_first_addr, _size) \
        ((void *)((uintptr_t)(_first_addr) + (_size) - 1))

enum arm_mpu_read_write_permissions {
     NO_ACCESS = 0x0,
     PRIVILEGED_READ_WRITE_UNPRIVILEGED_NO_ACCESS = 0x1,
     PRIVILEGED_READ_WRITE_UNPRIVILEGED_READ_ONLY = 0x2,
     PRIVILEGED_READ_WRITE_UNPRIVILEGED_READ_WRITE = 0x3,
     RESERVED = 0x4,
     PRIVILEGED_READ_ONLY_UNPRIVILEGED_NO_ACCESS = 0x5,
     PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY = 0x6
};

/**
 * Const fields of the MPU device
 */
struct mpu_device {
#   define MPU_DEVICE_SIGNATURE  GEN_SIGNATURE('M', 'P', 'U', ' ')
    uint32_t signature;
    MPU_Type *mmio_regs_p;
    struct mpu_device_var *var_p;
};

/**
 * Non-const fields of the MPU device
 */
struct mpu_device_var {
    bool initialized;
    bool enabled;
    bool return_from_fault_enabled;
    uint8_t num_regions;
    uint8_t num_defined_global_regions;
};

static struct mpu_device_var g_mpu_var = {
    .initialized = false,
    .enabled = false,
    .return_from_fault_enabled = false,
    .num_regions = 0,
    .num_defined_global_regions = 0,
};

static const struct mpu_device g_mpu = {
    .signature = MPU_DEVICE_SIGNATURE,
    .mmio_regs_p = (MPU_Type *)MPU_BASE,
    .var_p = &g_mpu_var,
};


static void memory_barrier(void)
{
    __DSB();
    __ISB();
}


static bool is_power_of_2(uint32_t value)
{
#   if __CORTEX_M >= 0x03
    uint32_t log_value = 31 - __CLZ(value);

    return (value == BIT(log_value));
#   else
    for (uint32_t log_value = 31; log_value != 0; log_value --) {
        if (value == BIT(log_value)) {
            return true;
        }
    }

    return (value == 1);
#   endif
}


static inline uint8_t int_log_base_2(uint32_t value)
{
    uint32_t log_value;

    D_ASSERT(value != 0);
#   if __CORTEX_M >= 0x03
    log_value = 31 - __CLZ(value);
#   else
    for (log_value = 31; (value & BIT(log_value)) == 0; log_value --)
        ;
#   endif
    return (uint8_t)log_value;
}


static uint8_t build_disabled_subregions_mask(
    const void *rounded_down_first_address,
    const void *rounded_up_last_address,
    const void *first_address,
    const void *last_address)
{
	D_ASSERT(rounded_up_last_address >= rounded_down_first_address);
	D_ASSERT(first_address >= rounded_down_first_address);
	D_ASSERT(last_address <= rounded_up_last_address);

    const size_t rounded_region_size = (uintptr_t)rounded_up_last_address -
                                             (uintptr_t)rounded_down_first_address + 1;
    const size_t subregion_size = rounded_region_size / MAX_NUM_SUB_REGIONS;
    const void *subregion_address;
    uint8_t subregion_bit_index1;
    uint8_t subregion_bit_index2;
    uint8_t disabled_subregions_mask = 0;

    D_ASSERT(subregion_size >= MPU_REGION_ALIGNMENT);

    subregion_address = rounded_down_first_address;
    subregion_bit_index1 = 0;
    if ((uintptr_t)first_address >= subregion_size) {
		while ((uintptr_t)subregion_address <= (uintptr_t)first_address - subregion_size) {
			D_ASSERT(subregion_bit_index1 < MAX_NUM_SUB_REGIONS);
			disabled_subregions_mask |= BIT(subregion_bit_index1);
			subregion_bit_index1++;
			subregion_address =
					(void *)((uintptr_t)subregion_address + subregion_size);
		}
    }

    D_ASSERT(subregion_bit_index1 < MAX_NUM_SUB_REGIONS);
    D_ASSERT(disabled_subregions_mask != UINT8_MAX);
    subregion_address = rounded_up_last_address;
    subregion_bit_index2 = MAX_NUM_SUB_REGIONS - 1;
    if ((uintptr_t)last_address <= UINTPTR_MAX - subregion_size) {
		while ((uintptr_t)subregion_address >= (uintptr_t)last_address + subregion_size) {
			D_ASSERT(subregion_bit_index2 > subregion_bit_index1);
			disabled_subregions_mask |= BIT(subregion_bit_index2);
			subregion_bit_index2--;
			subregion_address =
				(void *)((uintptr_t)subregion_address - subregion_size);
		}
    }

    D_ASSERT(disabled_subregions_mask != UINT8_MAX);
    D_ASSERT(subregion_bit_index1 <= subregion_bit_index2);
      return disabled_subregions_mask;
}


static void define_rounded_mpu_region(
    enum mpu_region_id region_id,
    void *rounded_down_first_address,
    size_t rounded_region_size,
    uint8_t subregions_disabled_mask,
    enum arm_mpu_read_write_permissions read_write_permissions,
    bool execute_permission)
{
	/* region size is encoded as a power of two - 1: region_size = 2 ** (encoded_size + 1): */
	D_ASSERT(rounded_region_size >= MPU_REGION_ALIGNMENT);
    const uint8_t encoded_region_size = int_log_base_2(rounded_region_size) - 1;
    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;
    uint32_t old_primask;
    uint32_t reg_value;

    old_primask = disable_cpu_interrupts();

#if 0
    emergency_printf(
    	"%s:%d: rounded_down_first_address 0x%x, rounded_up_region_size 0x%x, subregions_disabled 0x%x\r\n",
        __func__, __LINE__, rounded_down_first_address, rounded_region_size, subregions_disabled_mask);
#endif

    /*
     * Disable region before configuring it:
     */
    mpu_regs_p->RNR = region_id;
    reg_value = mpu_regs_p->RASR;
    reg_value &= ~MPU_RASR_ENABLE_Msk;
    mpu_regs_p->RASR = reg_value;

    /*
     * Configure region:
     */
    mpu_regs_p->RBAR = (uintptr_t)rounded_down_first_address;
    reg_value = 0;
    SET_BIT_FIELD(reg_value, MPU_RASR_SIZE_Msk, MPU_RASR_SIZE_Pos,
                  encoded_region_size);
    SET_BIT_FIELD(reg_value, MPU_RASR_SRD_Msk, MPU_RASR_SRD_Pos,
                  subregions_disabled_mask);
    SET_BIT_FIELD(reg_value, MPU_RASR_AP_Msk, MPU_RASR_AP_Pos,
                  read_write_permissions);
    if (!execute_permission) {
        reg_value |= MPU_RASR_XN_Msk;
    }

    if (VALID_FLASH_ADDRESS(rounded_down_first_address)) {
        reg_value |= MPU_RASR_C_Msk;
    } else if (VALID_MMIO_ADDRESS(rounded_down_first_address)) {
        reg_value |= MPU_RASR_S_Msk | MPU_RASR_B_Msk;
    } else {
        reg_value |= MPU_RASR_C_Msk | MPU_RASR_S_Msk;
    }

    reg_value |= MPU_RASR_ENABLE_Msk;
    mpu_regs_p->RASR = reg_value;

    memory_barrier();
    restore_cpu_interrupts(old_primask);
}

static size_t
mpu_get_enclosing_region_boundaries(const void *first_addr, const void *last_addr,
                                    void **region_first_addr_p, void **region_last_addr_p)
{
    D_ASSERT(first_addr <= last_addr);
    D_ASSERT((uintptr_t)last_addr != UINTPTR_MAX);
    size_t region_size;
    size_t actual_size;
    void *region_first_addr;
    void *region_last_addr;
    size_t range_size = (uintptr_t)last_addr - (uintptr_t)first_addr + 1;

    D_ASSERT(range_size != 0); /* check for overflow */
    if (range_size <= MPU_REGION_ALIGNMENT) {
		region_size = MPU_REGION_ALIGNMENT;
    } else {
		region_size = UINT32_C(1) << int_log_base_2(range_size);

		if (region_size < range_size) {
			region_size *= 2;
		}

		D_ASSERT(region_size != 0); /* check for overflow */
    }

    for ( ; ; ) {
		region_first_addr = (void *)ROUND_DOWN((uintptr_t)first_addr, region_size);
		region_last_addr = (void *)(ROUND_UP((uintptr_t)last_addr + 1, region_size) - 1);
		actual_size = (uintptr_t)region_last_addr - (uintptr_t)region_first_addr  + 1;
		D_ASSERT(actual_size != 0); /* check for overflow */
		if (actual_size == region_size) {
			break;
		}

		D_ASSERT(actual_size > region_size);
    	region_size *= 2;
    }

    *region_first_addr_p = region_first_addr;
    *region_last_addr_p = region_last_addr;
    return region_size;
}


static void define_mpu_region(
    enum mpu_region_id region_id,
    const void *first_address,
    const void *last_address,
    enum arm_mpu_read_write_permissions read_write_permissions,
    bool execute_permission)
{
    void *rounded_down_first_address;
    void *rounded_up_last_address;
    uint8_t subregions_disabled_mask = 0;
	size_t rounded_up_region_size =
		mpu_get_enclosing_region_boundaries(first_address, last_address,
                                    		&rounded_down_first_address,
											&rounded_up_last_address);

    D_ASSERT(rounded_down_first_address < rounded_up_last_address);

#if 0
    emergency_printF(
    	"%s:%d: last_address 0x%x, first_address 0x%x, region_size 0x%x, "
        "rounded_up_last_address 0x%x, rounded_down_first_address 0x%x, rounded_up_region_size 0x%x\r\n",
        __func__, __LINE__, last_address, first_address, region_size,
        rounded_up_last_address, rounded_down_first_address, rounded_up_region_size);
#endif

    if (rounded_up_region_size / MAX_NUM_SUB_REGIONS >= MPU_REGION_ALIGNMENT) {
		subregions_disabled_mask = build_disabled_subregions_mask(
										rounded_down_first_address,
										rounded_up_last_address,
										first_address,
										last_address);
    }

    define_rounded_mpu_region(region_id,
                              rounded_down_first_address,
                              rounded_up_region_size,
                              subregions_disabled_mask,
                              read_write_permissions,
                              execute_permission);
}


void mpu_init(void)
{
    uint32_t reg_value;

    /*
     * This function is to be invoked from the reset handler:
     *
     * NOTE: It is required that the caller is running in handler mode
     * as the only area of RAM that is writable after enabling the MPU
     * is the ISR stack.
     */
    D_ASSERT(CALLER_IS_RESET_HANDLER());
    D_ASSERT(g_mpu.signature == MPU_DEVICE_SIGNATURE);

    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;

    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(!mpu_var_p->initialized);

    /*
     * Verify that the MPU has enough regions:
     */
    reg_value = mpu_regs_p->TYPE;
    mpu_var_p->num_regions =
        GET_BIT_FIELD(reg_value, MPU_TYPE_DREGION_Msk, MPU_TYPE_DREGION_Pos);

    D_ASSERT(mpu_var_p->num_regions >= MAX_NUM_MPU_REGIONS);

    /*
     * Disable MPU to configure it:
     */
    reg_value = mpu_regs_p->CTRL;
    reg_value &= ~MPU_CTRL_ENABLE_Msk;
    mpu_regs_p->CTRL = reg_value;

    /*
     * Disable the default background region:
     */
    reg_value &= ~MPU_CTRL_PRIVDEFENA_Msk;
    mpu_regs_p->CTRL = reg_value;

    /*
     * Disable all regions:
     */
    reg_value = 0;
    for (int i = 0; i < mpu_var_p->num_regions; i++) {
        SET_BIT_FIELD(reg_value, MPU_RNR_REGION_Msk, MPU_RNR_REGION_Pos, i);
        mpu_regs_p->RNR = reg_value;
        mpu_regs_p->RASR = 0;
    }

    /*
     * Set MPU region that represents the global background region
     * to have read-only permissions:
     */
    define_mpu_region(GLOBAL_BACKGROUND_DATA_REGION,
                      __background_data_region_start,
                      (void *)((uint32_t)__sram_end - 1),
                      PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY,
                      false /*no execute*/);

    /*
     * Set global region for executable code and constants in flash:
     */
    define_mpu_region(GLOBAL_FLASH_CODE_REGION,
                      __flash_text_start,
                      (void *)((uintptr_t)__etext - 1),
                      PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY,
                      true /*execute*/);

    /*
     * Set global region for executable code in RAM:
     */
    if (__ram_text_start != __ram_text_end) {
        define_mpu_region(GLOBAL_RAM_CODE_REGION,
                          __ram_text_start,
                          (void *)((uintptr_t)__ram_text_end - 1),
                          PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY,
                          true /*execute*/);
    }

    /*
     * Set global region for ISR stack:
     */
    define_mpu_region(GLOBAL_INTERRUPT_STACK_REGION,
                      __StackLimit,
                      (void *)((uintptr_t)__StackTop - 1),
                      PRIVILEGED_READ_WRITE_UNPRIVILEGED_NO_ACCESS,
                      false /*no execute*/);

#if 0
     /*
      *  Set global region for accessing the MPU I/O registers:
      *
      *  NOTE: Once the default background region is disabled,
      *  we won't be able to modify any MPU region descriptor unless
      *  we reserve a region for the MPU itself.
      */
     define_mpu_region(GLOBAL_MPU_IO_REGION,
    		 	 	   mpu_regs_p,
                       LAST_ADDRESS(mpu_regs_p, sizeof(*mpu_regs_p)),
                       PRIVILEGED_READ_WRITE_UNPRIVILEGED_NO_ACCESS,
                       false /*no execute*/);
#endif

    /*
     * NOTE: We do not need to set a region for the ARM core
     * memory-mapped control registers (private peripheral area:
     * 16#E000_0000# .. 16#E00F_FFFF#), as they are always accessible
     * in privileged mode, regardless of the MPU settings.
     */

    /*
     * NOTE: Leave the MPU disabled, so that the Ada runtime startup code
     * and global package elaboration can execute normally.
     * The application's main program is expected to call Enable_MPU
     */

    mpu_var_p->return_from_fault_enabled = false;
    mpu_var_p->initialized = true;
}


bool mpu_disable(void)
{
    uint32_t reg_value;
    bool mpu_was_enabled;

    D_ASSERT(g_mpu.signature == MPU_DEVICE_SIGNATURE);

    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;

    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(mpu_var_p->initialized);
    mpu_was_enabled = mpu_var_p->enabled;
    if (!mpu_was_enabled) {
    	return false;
    }

    memory_barrier();
    reg_value = mpu_regs_p->CTRL;
    reg_value &= ~MPU_CTRL_ENABLE_Msk;
    mpu_regs_p->CTRL = reg_value;
    memory_barrier();
    mpu_var_p->enabled = false;
    return mpu_was_enabled;
}

void mpu_enable(void)
{
    uint32_t reg_value;

    D_ASSERT(g_mpu.signature == MPU_DEVICE_SIGNATURE);

    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;

    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(mpu_var_p->initialized);
    D_ASSERT(!mpu_var_p->enabled);

    mpu_var_p->enabled = true;
    memory_barrier();

    reg_value = mpu_regs_p->CTRL;
    reg_value |= MPU_CTRL_ENABLE_Msk;
    mpu_regs_p->CTRL = reg_value;
}


static void restore_mpu_region_descriptor(
      enum mpu_region_id region_id,
      const struct mpu_region_descriptor *saved_region_p)
{
    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;

    mpu_regs_p->RNR = region_id;
    mpu_regs_p->RBAR = saved_region_p->rbar_value;
    mpu_regs_p->RASR = saved_region_p->rasr_value;
    memory_barrier();
}


void restore_thread_mpu_regions(const struct thread_regions *thread_regions_p)
{
    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(mpu_var_p->initialized);

    restore_mpu_region_descriptor(THREAD_STACK_DATA_REGION,
                                  &thread_regions_p->stack_region);

    restore_mpu_region_descriptor(PRIVATE_DATA_REGION,
                                  &thread_regions_p->private_data_region);

    restore_mpu_region_descriptor(PRIVATE_CODE_REGION,
                                  &thread_regions_p->private_code_region);

    (void)set_writable_background_region(
            thread_regions_p->writable_background_region_enabled);
}


static void save_mpu_region_descriptor(
      enum mpu_region_id region_id,
      struct mpu_region_descriptor *saved_region_p)
{
    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;

    mpu_regs_p->RNR = region_id;
    saved_region_p->rbar_value = mpu_regs_p->RBAR;
    saved_region_p->rasr_value = mpu_regs_p->RASR;
}


void save_thread_mpu_regions(struct thread_regions *thread_regions_p)
{
    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(mpu_var_p->initialized);

    bool old_enabled = set_writable_background_region(true);

    /*
     * Note: Although it may seem unnecessary to have the old_enabled local
     * variable, instead of passing using
     * 'thread_regions_p->writable_background_region_enable' directly
     * in the call above, we cannot do that as we need to ensure
     * we can write to it, by enabling writes on the background
     * region first.
     */
    thread_regions_p->writable_background_region_enabled = old_enabled;

    save_mpu_region_descriptor(THREAD_STACK_DATA_REGION,
                                   &thread_regions_p->stack_region);

    save_mpu_region_descriptor(PRIVATE_DATA_REGION,
                                   &thread_regions_p->private_data_region);

    save_mpu_region_descriptor(PRIVATE_CODE_REGION,
                                   &thread_regions_p->private_code_region);

    /*
     * NOTE: We return leaving the background region enabled for writing
     * so that the rest of the context switch logic in the RTOS code
     * can update its data structures.
     */
}


void initialize_thread_mpu_regions(void *stack_base_addr,
        						   size_t stack_size,
                                   struct thread_regions *thread_regions_p)
{
    uint32_t rasr_value;
    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;
    D_ASSERT(mpu_var_p->initialized);
    D_ASSERT(is_power_of_2(stack_size));
    D_ASSERT((uintptr_t)stack_base_addr % stack_size == 0);

    thread_regions_p->private_data_region.rbar_value = 0;
    thread_regions_p->private_data_region.rasr_value = 0;
    thread_regions_p->private_code_region.rbar_value = 0;
    thread_regions_p->private_code_region.rasr_value = 0;
    thread_regions_p->stack_region.rbar_value = (uintptr_t)stack_base_addr;
    rasr_value = 0;
    SET_BIT_FIELD(rasr_value, MPU_RASR_SIZE_Msk, MPU_RASR_SIZE_Pos,
                  int_log_base_2(stack_size));
    SET_BIT_FIELD(rasr_value, MPU_RASR_AP_Msk, MPU_RASR_AP_Pos,
                  PRIVILEGED_READ_WRITE_UNPRIVILEGED_READ_WRITE);
    rasr_value |= MPU_RASR_XN_Msk;
    rasr_value |= MPU_RASR_C_Msk | MPU_RASR_S_Msk;
    rasr_value |= MPU_RASR_ENABLE_Msk;
    thread_regions_p->stack_region.rasr_value = rasr_value;
}


bool set_writable_background_region(bool enabled)
{
    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;
    uint32_t old_primask;
    uint32_t reg_value;
    bool old_enabled;
    enum arm_mpu_read_write_permissions read_write_permissions;

    old_primask = disable_cpu_interrupts();

    /*
     * Disable region before configuring it:
     */
    mpu_regs_p->RNR = GLOBAL_BACKGROUND_DATA_REGION;
    reg_value = mpu_regs_p->RASR;
    old_enabled = (GET_BIT_FIELD(reg_value, MPU_RASR_AP_Msk, MPU_RASR_AP_Pos) ==
                   PRIVILEGED_READ_WRITE_UNPRIVILEGED_READ_ONLY);

    if (enabled) {
        read_write_permissions = PRIVILEGED_READ_WRITE_UNPRIVILEGED_READ_ONLY;
    } else {
        read_write_permissions = PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY;
    }

    SET_BIT_FIELD(reg_value, MPU_RASR_AP_Msk, MPU_RASR_AP_Pos,
                  read_write_permissions);
    mpu_regs_p->RASR = reg_value;

    memory_barrier();
    restore_cpu_interrupts(old_primask);
    return old_enabled;
}


bool is_mpu_enabled(void)
{
    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(mpu_var_p->initialized);
    return mpu_var_p->enabled;
}


void restore_private_code_region(const struct mpu_region_descriptor *saved_region_p)
{
    if (!g_mpu.var_p->initialized) {
    	return;
    }

    restore_mpu_region_descriptor(PRIVATE_CODE_REGION, saved_region_p);
}


void restore_private_data_region(const struct mpu_region_descriptor *saved_region_p)
{
    if (!g_mpu.var_p->initialized) {
    	return;
    }

    restore_mpu_region_descriptor(PRIVATE_DATA_REGION, saved_region_p);
}


void set_private_code_region_no_save(void *first_addr, void *last_addr)
{
    if (!g_mpu.var_p->initialized) {
    	return;
    }

    define_mpu_region(PRIVATE_CODE_REGION,
                      first_addr,
                      last_addr,
                      PRIVILEGED_READ_ONLY_UNPRIVILEGED_NO_ACCESS,
                      true);
}


void set_private_code_region(void *first_addr, void *last_addr,
                             struct mpu_region_descriptor *old_region_p)
{
    if (!g_mpu.var_p->initialized) {
    	return;
    }

    save_mpu_region_descriptor(PRIVATE_CODE_REGION, old_region_p);
    define_mpu_region(PRIVATE_CODE_REGION,
                      first_addr,
                      last_addr,
                      PRIVILEGED_READ_ONLY_UNPRIVILEGED_NO_ACCESS,
                      true);
}


void set_private_data_region_no_save(void *start_addr, size_t size,
                                     enum data_region_permissions permissions)
{
    if (!g_mpu.var_p->initialized) {
    	return;
    }

    enum arm_mpu_read_write_permissions read_write_permissions =
        (permissions == READ_WRITE ? PRIVILEGED_READ_WRITE_UNPRIVILEGED_NO_ACCESS
                                   : PRIVILEGED_READ_ONLY_UNPRIVILEGED_NO_ACCESS);

    define_mpu_region(PRIVATE_DATA_REGION,
                      start_addr,
                      LAST_ADDRESS(start_addr, size),
                      read_write_permissions,
                      false);
}


void set_private_data_region(void *start_addr, size_t size,
                             enum data_region_permissions permissions,
                             struct mpu_region_descriptor *old_region_p)
{
    if (!g_mpu.var_p->initialized) {
    	return;
    }

    enum arm_mpu_read_write_permissions read_write_permissions =
        (permissions == READ_WRITE ? PRIVILEGED_READ_WRITE_UNPRIVILEGED_NO_ACCESS
                                   : PRIVILEGED_READ_ONLY_UNPRIVILEGED_NO_ACCESS);

    save_mpu_region_descriptor(PRIVATE_DATA_REGION, old_region_p);
    define_mpu_region(PRIVATE_DATA_REGION,
                      start_addr,
                      LAST_ADDRESS(start_addr, size),
                      read_write_permissions,
                      false);
}


static void dump_mpu_region_descriptors(void)
{
	struct mpu_region_descriptor region_desc;

	emergency_printf("MPU region descriptors:\n");
	for (uint32_t i = 0; i < MAX_NUM_MPU_REGIONS; i++) {
		save_mpu_region_descriptor(i, &region_desc);
		emergency_printf("\tregion %u: RBAR: %#x, RASR: %#x\n",
				         i, region_desc.rbar_value, region_desc.rasr_value);
	}

}

void HardFault_Handler(void)
{
    const void *pc_at_exception;
    const void *lr_at_exception;
	const uint32_t *stack_pointer;
    const uint32_t *frame_pointer;
    uintptr_t return_address;
    uintptr_t in_stack_return_address;
    const struct rtos_task *task_p;

    CAPTURE_ARM_LR_REGISTER(return_address);
    CAPTURE_ARM_FRAME_POINTER_REGISTER(frame_pointer);

    mpu_disable();
    emergency_printf("\n*** Hard fault exception ***\n");
    dump_mpu_region_descriptors();

    if (return_address == CPU_EXC_RETURN_TO_THREAD_MODE_USING_PSP ||
        return_address == CPU_EXC_RETURN_TO_THREAD_MODE_USING_PSP_FPU) {
        /*
         * The code where the exception was triggered was using the PSP stack
         * pointer, so the offending code was a task
         */
		stack_pointer = (uint32_t *)(uintptr_t)__get_PSP();

        lr_at_exception = (void *)stack_pointer[5];
        pc_at_exception = (void *)stack_pointer[6];
        task_p = rtos_task_get_current();
    } else {
       /*
        * The code where the exception was triggered was using the MSP stack
        * pointer. So, the offending code was another interrupt/exception handler.
        * The frame pointer must be pointing to the top of the stack at
        * the time when registers saving was done before branching to
        * the interrupt/exception handler.
        */
        lr_at_exception = (void *)frame_pointer[12+5];
        pc_at_exception = (void *)frame_pointer[12+6];
        task_p = NULL;
    }

    bool found_prev_stack_frame = find_previous_stack_frame(NULL,
															__StackTop,
															&frame_pointer,
															&in_stack_return_address);

	if (!found_prev_stack_frame) {
		emergency_printf("*** ERROR: Cannot unwind the stack\n");
		goto hang;
	}

	if (in_stack_return_address != return_address) {
		emergency_printf("*** ERROR: Unexpected return address found in the stack: 0x%p\n",
						 in_stack_return_address);
		goto hang;
	}

    emergency_printf("\nCPU registers:\n"
    		         "\tPC 0x%p\n"
    			     "\tLR 0x%p\n"
    				 "\tFP 0x%p\n",
    			     pc_at_exception, lr_at_exception, frame_pointer);

	emergency_printf("\nStack trace:\n");
	print_stack_trace(0, (void *)pc_at_exception, frame_pointer, task_p);

hang:
    /*
     * Hold the processor in a dummy infinite loop:
     */
    (void)disable_cpu_interrupts();
    for ( ; ; )
        ;
}
