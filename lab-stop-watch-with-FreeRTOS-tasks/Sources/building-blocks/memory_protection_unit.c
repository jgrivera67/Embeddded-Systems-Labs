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
#include "microcontroller.h"
#include <stdbool.h>

/**
 * Privileged and unprivileged read/write permissions
 */
enum read_write_permissions {
     NO_ACCESS = 0,
     PRIVILEGED_READ_WRITE_UNPRIVILEGED_NO_ACCESS,
     PRIVILEGED_READ_WRITE_UNPRIVILEGED_READ_ONLY,
     PRIVILEGED_READ_WRITE_UNPRIVILEGED_READ_WRITE,
     RESERVED,
     PRIVILEGED_READ_ONLY_UNPRIVILEGED_NO_ACCESS,
     PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY
};

/**
 * Linker script symbol for the start address of the global flash
 * text region
 */
extern uint32_t __flash_text_start[];

/**
 * Linker script symbol for the end address of the global flash
 * text region
 */
extern uint32_t __rom_end[];

/**
 * Linker script symbol for the start address of the global RAM
 * text region
 */
extern uint32_t __ram_text_start[];

/**
 * Linker script symbol for the end address of the global RAM text
 * region
 */
extern uint32_t __ram_text_end[];

/**
 *  Linker script symbol for the start address of the global background
 *  data region. The global background data area spans to the end of the
 *  physical address space
 */
extern uint32_t __background_data_region_start[];

/**
 * Linker script symbol for the start address of the stack for ISRs
 */
extern uint32_t __interrupt_stack_start[];

/**
 * Linker script symbol for the end address of the stack for ISRs
 */
extern uint32_t __interrupt_stack_end[];

/**
 * Linker script symbol for the start address of the sectet flash
 * text region
 */
extern uint32_t __secret_flash_text_start[];

/**
 * Linker script symbol for the end address of the secret flash text
 * region
 */
extern uint32_t __secret_flash_text_end[];

/**
 * Linker script symbol for the start address of the sectet RAM
 * text region
 */
extern uint32_t __secret_ram_text_start[];

/**
 * Linker script symbol for the end address of the secret RAM text
 * region
 */
extern uint32_t __secret_ram_text_end[];

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
    uint8_t num_regions;
    uint8_t num_defined_global_regions;
};

static struct mpu_device_var g_mpu_var = {
    .initialized = false,
    .enabled = false,
    .num_regions = 0,
    .num_defined_global_regions = 0,
};

static const struct mpu_device g_mpu = {
    .signature = MPU_DEVICE_SIGNATURE,
    .mmio_regs_p = (MPU_Type *)MPU_BASE,
    .var_p = &g_mpu_var,
};

static    function Encode_Region_Size (Region_Size_Bytes : Integer_Address)
return Encoded_Region_Size_Type
is
begin
for Log_Base2_Value in reverse Encoded_Region_Size_Type'Range loop
   if (Unsigned_32 (Region_Size_Bytes) and
       Shift_Left (Unsigned_32 (1), Natural (Log_Base2_Value))) /= 0
   then
      pragma Assert ((Region_Size_Bytes and
                      ((2 ** Natural (Log_Base2_Value)) - 1)) = 0);

      return Log_Base2_Value - 1;
   end if;
end loop;

pragma Assert (Region_Size_Bytes = 0); --  4GiB
return Encoded_Region_Size_Type'Last;
end Encode_Region_Size;

static void procedure Define_Rounded_MPU_Region (
   Region_Id : MPU_Region_Id_Type;
   Rounded_First_Address : System.Address;
   Rounded_Region_Size : Integer_Address;
   Subregions_Disabled_Mask : Subregions_Disabled_Mask_Type;
   Read_Write_Permissions : Read_Write_Permissions_Type;
   Execute_Permission : Boolean := False)
is
   Region_Index : constant Byte := Region_Id'Enum_Rep;
   Encoded_Region_Size : constant Encoded_Region_Size_Type :=
      Encode_Region_Size (Rounded_Region_Size);
   Old_Intr_Mask : Word;
   RNR_Value : MPU_RNR_Register_Type;
   RBAR_Value : MPU_RBAR_Register_Type;
   RASR_Value : MPU_RASR_Register_Type;
   ADDR_Value : constant UInt27 :=
      UInt27 (Shift_Right (Unsigned_32 (To_Integer (Rounded_First_Address)),
                           5));
begin
   Old_Intr_Mask := Disable_Cpu_Interrupts;

   --
   --  Configure region:
   --
   RNR_Value.REGION := Region_Index;
   MPU_Registers.MPU_RNR := RNR_Value;
   Memory_Barrier;
   RBAR_Value := (ADDR => ADDR_Value,
                  others => <>);
   MPU_Registers.MPU_RBAR := RBAR_Value;
   RASR_Value := (ENABLE => 1,
                  SIZE => Encoded_Region_Size,
                  SRD => Subregions_Disabled_Mask,
                  ATTRS => (AP => Read_Write_Permissions,
                            XN => (if Execute_Permission then 0 else 1),
                            others => <>));
   MPU_Registers.MPU_RASR := RASR_Value;

   Memory_Barrier;
   Restore_Cpu_Interrupts (Old_Intr_Mask);
end Define_Rounded_MPU_Region;

procedure Build_Disabled_Subregions_Mask (
   Rounded_Down_First_Address : Address;
   Rounded_Up_Last_Address : Address;
   First_Address : Address;
   Last_Address : Address;
   Disabled_Subregions_Mask : out Subregions_Disabled_Mask_Type)
is
   Rounded_Region_Size : constant Integer_Address :=
      To_Integer (Rounded_Up_Last_Address) -
      To_Integer (Rounded_Down_First_Address) + 1;
   Subregion_Size : constant Integer_Address :=
      Rounded_Region_Size / Disabled_Subregions_Mask'Length;
   Subregion_Index : Subregion_Index_Type;
   Subregion_Address : Address;
begin
   if Subregion_Size < MPU_Region_Alignment then
      Disabled_Subregions_Mask := (others => 0);
   else
      Subregion_Index := Disabled_Subregions_Mask'First;
      Subregion_Address := Rounded_Down_First_Address;
      loop
         exit when Subregion_Address >= First_Address;
         Disabled_Subregions_Mask (Subregion_Index) := 1;
         Subregion_Index := Subregion_Index + 1;
         Subregion_Address := To_Address (To_Integer (Subregion_Address) +
                                          Subregion_Size);
      end loop;

      pragma Assert (Subregion_Index < Disabled_Subregions_Mask'Last);
      Subregion_Address := Rounded_Up_Last_Address;
      loop
         exit when Subregion_Address <= Last_Address;
         Disabled_Subregions_Mask (Subregion_Index) := 1;
         Subregion_Index := Subregion_Index + 1;
         Subregion_Address := To_Address (To_Integer (Subregion_Address) -
                                          Subregion_Size);
      end loop;
   end if;
end Build_Disabled_Subregions_Mask;

static function Round_Up_Region_Size_To_Power_Of_2 (
   Region_Size_Bytes : Integer_Address)
   return Integer_Address
is
   Power_Of_2 : Integer_Address;
begin
   for Log_Base2_Value in reverse Encoded_Region_Size_Type'Range loop
      Power_Of_2 := 2 ** Natural (Log_Base2_Value);
      if (Unsigned_32 (Region_Size_Bytes) and
          Shift_Left (Unsigned_32 (1), Natural (Log_Base2_Value))) /= 0
      then
         if (Region_Size_Bytes and (Power_Of_2 - 1)) = 0 then
            return Power_Of_2;
         else
            if Log_Base2_Value = Encoded_Region_Size_Type'Last then
               return 0; -- 4GiB encoded in 32-bits
            else
               return Power_Of_2 * 2;
            end if;
         end if;
      end if;
   end loop;

   return 0; -- 4GiB encoded in 32-bits
end Round_Up_Region_Size_To_Power_Of_2;

static void    procedure Define_MPU_Region (
	      Region_Id : MPU_Region_Id_Type;
	      First_Address : System.Address;
	      Last_Address : System.Address;
	      Read_Write_Permissions : Read_Write_Permissions_Type;
	      Execute_Permission : Boolean := False)
	   is
	      Rounded_Up_Region_Size : Integer_Address;
	      Rounded_Down_First_Address : Address;
	      Rounded_Up_Last_Address : Address;
	      Subregions_Disabled_Mask : Subregions_Disabled_Mask_Type;
	      Rounded_Region_Size_Bytes : Integer_Address;
	   begin
	      Rounded_Up_Region_Size :=
	         Round_Up_Region_Size_To_Power_Of_2 (
	            To_Integer (Last_Address) - To_Integer (First_Address) + 1);
	      Rounded_Down_First_Address :=
	         Round_Down_Address (First_Address, Rounded_Up_Region_Size);

	      if To_Integer (Last_Address) = Integer_Address (Unsigned_32'Last) then
	         Rounded_Up_Last_Address := Last_Address;
	      else
	         Rounded_Up_Last_Address :=
	            To_Address (To_Integer (
	               Round_Up_Address (To_Address (To_Integer (Last_Address) + 1),
	                                 Rounded_Up_Region_Size)) - 1);
	      end if;

	      Build_Disabled_Subregions_Mask (
	         Rounded_Down_First_Address,
	         Rounded_Up_Last_Address,
	         First_Address,
	         Last_Address,
	         Subregions_Disabled_Mask);

	      Rounded_Region_Size_Bytes :=
	         To_Integer (Rounded_Up_Last_Address) -
	         To_Integer (Rounded_Down_First_Address) + 1;

	      Define_Rounded_MPU_Region (
	            Region_Id,
	            Rounded_Down_First_Address,
	            Rounded_Region_Size_Bytes,
	            Subregions_Disabled_Mask,
	            Read_Write_Permissions,
	            Execute_Permission);
	   end Define_MPU_Region;

static void    procedure Save_MPU_Region_Descriptor (
	      Region_Id : MPU_Region_Id_Type;
	      Saved_Region : out MPU_Region_Descriptor_Type)
	   is
	      RNR_Value : MPU_RNR_Register_Type;
	   begin
	      RNR_Value := (REGION => Region_Id'Enum_Rep, others => <>);
	      MPU_Registers.MPU_RNR := RNR_Value;
	      Memory_Barrier;
	      Saved_Region.RBAR_Value := MPU_Registers.MPU_RBAR;
	      Saved_Region.RASR_Value := MPU_Registers.MPU_RASR;
	   end Save_MPU_Region_Descriptor;

static void memory_barrier(void)
{
	__DSB();
	__ISB();
}

void mpu_init(void)
{
	uint32_t reg_value;

    D_ASSERT(g_mpu.signature == MPU_DEVICE_SIGNATURE);

    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;

    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(!mpu_var_p->initialized);

	/*
     * Verify that the MPU has enough regions:
     */
    reg_value = mpu_regs_p->TYPE;
    mpu_var_p->num_regions = (reg_value & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
    D_ASSERT(mpu_var_p->num_regions >= NUM_MPU_REGIONS);

    /*
     *  Disable MPU to configure it:
     */
    reg_value = mpu_regs_p->CTRL;
    reg_value &= ~MPU_CTRL_ENABLE_Msk;
    mpu_regs_p->CTRL = reg_value;

    /*
     *  Disable the default background region:
     */
    reg_value &= ~MPU_CTRL_PRIVDEFENA_Msk;
    mpu_regs_p->CTRL = reg_value;

    /*
     * Disable access to all regions:
     */
    for (int i = 0; i < mpu_var_p->num_regions; i++) {
		mpu_regs_p->RNR = (i << MPU_RNR_REGION_Pos) & MPU_RNR_REGION_Msk;
        memory_barrier();
        mpu_regs_p->RASR = 0;
    }

    /*
     * Set global region for executable code and constants in flash:
     */
    define_mpu_region(GLOBAL_FLASH_CODE_REGION,
            		  __flash_text_start,
					  (void *)((uintptr_t)__rom_end - 1),
                      PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY,
					  true /* execute_permission */);

    /*
     * Set global region for executable code in RAM:
     */
    define_mpu_region(GLOBAL_RAM_CODE_REGION,
            		  __ram_text_start,
					  (void *)((uintptr_t)__ram_text_end - 1),
                      PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY,
					  true /* execute_permission */);

   /*
    * Set global region for ISR stack:
    */
    define_mpu_region(GLOBAL_INTERRUPT_STACK_REGION,
            		  __interrupt_stack_start,
					  (void *)((uintptr_t)__interrupt_stack_end - 1),
					  PRIVILEGED_READ_WRITE_UNPRIVILEGED_NO_ACCESS,
					  false /* execute_permission */);

   /*
    * NOTE: For the ARMV7-M MPU, we don't need to set a global region
    * for accessing the MPU I/O registers, as they are always
    * accessible:
    */

   /*
    * NOTE: We do not need to set a region for the ARM core
    * memory-mapped control registers (private peripheral area:
    * E000_0000 .. E00F_FFFF), as they are always accessible
    * regardless of the MPU settings.
    */

   /*
    * Set MPU region that represents the global background region
    * to have read-only permissions:
    */
    define_mpu_region(GLOBAL_BACKGROUND_DATA_REGION,
            		  __background_data_region_start,
					  UINT32_MAX,
					  PRIVILEGED_READ_ONLY_UNPRIVILEGED_READ_ONLY,
					  false /* execute_permission */);

    mpu_var_p->initialized = true;
    mpu_enable();
}

/*
 * Disable MPU
 */
void mpu_disable(void)
{
	uint32_t reg_value;

    D_ASSERT(g_mpu.signature == MPU_DEVICE_SIGNATURE);

    MPU_Type *const mpu_regs_p = g_mpu.mmio_regs_p;

    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

    D_ASSERT(mpu_var_p->initialized);
    D_ASSERT(mpu_var_p->enabled);

    memory_barrier();
    reg_value = mpu_regs_p->CTRL;
    reg_value &= ~MPU_CTRL_ENABLE_Msk;
    mpu_regs_p->CTRL = reg_value;
    memory_barrier();

    mpu_var_p->enabled = false;
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

/**
 * Restore thread-private MPU regions
 *
 * NOTE: This function is to be invoked only from the RTOS
 * context switch code and with the background region enabled
 */
void restore_thread_mpu_regions(const struct thread_regions *thread_regions_p)
{

}

/**
 * Save thread-private MPU regions
 *
 * NOTE: This function is to be invoked only from the RTOS
 * context switch code and with the background region enabled
 */
void save_thread_mpu_regions(struct thread_regions *thread_regions_p)
{

}

void set_cpu_writable_background_region(bool enabled)
{

}

bool set_cpu_writable_background_region(bool enabled)
{

}

bool is_mpu_enabled(void)
{
    struct mpu_device_var *const mpu_var_p = g_mpu.var_p;

	return  mpu_var_p->enabled;
}

void restore_private_code_region(struct mpu_region_descriptor saved_region)
{

}

void restore_private_data_region(struct mpu_region_descriptor saved_region)
{

}

void set_private_code_region_no_save(void *first_addr, void *last_addr)
{

}

void set_private_code_region(void *first_addr, void *last_addr,
		                     struct mpu_region_descriptor *old_region_p)
{

}

void set_private_data_region_no_save(void *start_addr, size_t size,
		                             enum data_region_permissions permissions)
{

}

void set_private_data_region(void *start_addr, size_t size,
		                     enum data_region_permissions permissions,
		                     struct mpu_region_descriptor *old_region_p)
{

}
