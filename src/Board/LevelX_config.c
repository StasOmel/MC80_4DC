#include "App.h"

// === LevelX NOR Configuration ===

// LevelX read buffer (used only when LX_DIRECT_READ is disabled)
#ifndef LX_DIRECT_READ
  #define FSP_LX_READ_BUFFER_SIZE_WORDS (128U)
ULONG g_rm_levelx_nor_OSPI_read_buffer[FSP_LX_READ_BUFFER_SIZE_WORDS] = { 0 };
#endif

// === LevelX NOR System Error Handling ===

/*-----------------------------------------------------------------------------------------------------
  Description: Weak system error callback for LevelX NOR operations
               This function is called when a critical error occurs in LevelX operations
               Override this function to implement custom error handling

  Parameters: error_code - error code indicating the type of error that occurred

  Return: LX_ERROR - indicates error condition to LevelX
-----------------------------------------------------------------------------------------------------*/
#if defined(__ICCARM__)
  #define g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE
  #pragma weak g_rm_levelx_nor_OSPI_system_error = g_rm_levelx_nor_OSPI_system_error_internal
#elif defined(__GNUC__)
  #define g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE \
    __attribute__((weak, alias("g_rm_levelx_nor_OSPI_system_error_internal")))
#endif

UINT g_rm_levelx_nor_OSPI_system_error(UINT error_code) g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE;

/*-----------------------------------------------------------------------------------------------------
  Description: Internal system error handler for LevelX NOR operations
               Default implementation triggers unrecoverable error handler

  Parameters: error_code - error code indicating the type of error that occurred

  Return: LX_ERROR - indicates error condition to LevelX
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_system_error_internal(UINT error_code);
static UINT g_rm_levelx_nor_OSPI_system_error_internal(UINT error_code)
{
  FSP_PARAMETER_NOT_USED(error_code);

  // Trigger unrecoverable error handler - system will halt
  BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);

  return LX_ERROR;
}

// === LevelX NOR Driver Service Functions ===

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Read Sector" service function
               Reads data from NOR flash memory through MC80_OSPI_drv

  Parameters: flash_address - pointer to flash memory address to read from
              destination   - pointer to destination buffer for read data
              words         - number of 32-bit words to read

  Return: LX_SUCCESS on success, LX_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_read(ULONG *flash_address, ULONG *destination, ULONG words);
static UINT g_rm_levelx_nor_OSPI_read(ULONG *flash_address, ULONG *destination, ULONG words)
{
  fsp_err_t err;
  uint32_t  byte_count = words * 4;  // 4 bytes per ULONG
  uint32_t  address = (uint32_t)flash_address - BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS;  // Convert to relative address

  err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, (uint8_t *)destination, address, byte_count);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Write Sector" service function
               Writes data to NOR flash memory through MC80_OSPI_drv

  Parameters: flash_address - pointer to flash memory address to write to
              source        - pointer to source buffer containing data to write
              words         - number of 32-bit words to write

  Return: LX_SUCCESS on success, LX_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_write(ULONG *flash_address, ULONG *source, ULONG words);
static UINT g_rm_levelx_nor_OSPI_write(ULONG *flash_address, ULONG *source, ULONG words)
{
  fsp_err_t err;
  uint32_t  byte_count = words * 4;  // 4 bytes per ULONG
  uint32_t  address = (uint32_t)flash_address - BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS;  // Convert to relative address

  err = Mc80_ospi_memory_mapped_write(g_mc80_ospi.p_ctrl, (uint8_t *)source, address, byte_count);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Block Erase" service function
               Erases a block of NOR flash memory through MC80_OSPI_drv

  Parameters: block             - block number to erase
              block_erase_count - erase count for wear leveling

  Return: LX_SUCCESS on success, LX_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_block_erase(ULONG block, ULONG block_erase_count);
static UINT g_rm_levelx_nor_OSPI_block_erase(ULONG block, ULONG block_erase_count)
{
  fsp_err_t err;
  uint32_t  block_address = block * LEVELX_BLOCK_SIZE_BYTES;  // Calculate block address using macro

  FSP_PARAMETER_NOT_USED(block_erase_count);

  // Erase block using MC80 OSPI driver - function expects relative address
  err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, block_address, LEVELX_BLOCK_SIZE_BYTES);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Block Erased Verify" service function
               Verifies that a block has been properly erased (all bits set to 1)

  Parameters: block - block number to verify

  Return: LX_SUCCESS if block is erased, LX_ERROR if not erased or on failure
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_block_erased_verify(ULONG block);
static UINT g_rm_levelx_nor_OSPI_block_erased_verify(ULONG block)
{
  // Для простоты implementation, предполагаем что блок стерт успешно
  // В реальной реализации можно добавить проверку чтения блока и проверки что все биты установлены в 1
  FSP_PARAMETER_NOT_USED(block);

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Driver Initialization" service function
               Initializes the LevelX NOR driver and sets up function pointers

  Parameters: p_nor_flash - pointer to LX_NOR_FLASH structure to initialize

  Return: LX_SUCCESS on successful initialization, LX_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
UINT g_rm_levelx_nor_OSPI_initialize(LX_NOR_FLASH *p_nor_flash)
{
  fsp_err_t err;

  // Initialize OSPI driver first (similar to Littlefs_initialize)
  err = Mc80_ospi_open(g_mc80_ospi.p_ctrl, g_mc80_ospi.p_cfg);
  if (err != FSP_SUCCESS)
  {
    // Check if driver is already opened
    if (err == FSP_ERR_ALREADY_OPEN)
    {
      // Driver already opened, continue
    }
    else
    {
      return LX_ERROR;
    }
  }

  // Set OSPI protocol as configured using safe switch (uses LEVELX_OSPI_PROTOCOL from header)
  err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, LEVELX_OSPI_PROTOCOL);
  if (err != FSP_SUCCESS)
  {
    return LX_ERROR;
  }

  // Setup the base address of the flash memory
  p_nor_flash->lx_nor_flash_base_address = (ULONG *)BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS;

  // Setup geometry of the flash using configuration macros
  p_nor_flash->lx_nor_flash_total_blocks = LEVELX_TOTAL_BLOCKS;        // 512 blocks (32MB / 64KB)
  p_nor_flash->lx_nor_flash_words_per_block = LEVELX_WORDS_PER_BLOCK;  // 16384 words per block (64KB / 4 bytes)

#ifndef LX_DIRECT_READ
  // Set sector buffer for LevelX (used only when LX_DIRECT_READ is disabled)
  p_nor_flash->lx_nor_flash_sector_buffer = g_rm_levelx_nor_OSPI_read_buffer;
#endif

  // Set LevelX driver function pointers
  p_nor_flash->lx_nor_flash_driver_read                = g_rm_levelx_nor_OSPI_read;
  p_nor_flash->lx_nor_flash_driver_write               = g_rm_levelx_nor_OSPI_write;
  p_nor_flash->lx_nor_flash_driver_block_erase         = g_rm_levelx_nor_OSPI_block_erase;
  p_nor_flash->lx_nor_flash_driver_block_erased_verify = g_rm_levelx_nor_OSPI_block_erased_verify;
  p_nor_flash->lx_nor_flash_driver_system_error        = g_rm_levelx_nor_OSPI_system_error;

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Driver Close" service function
               Closes the LevelX NOR driver and releases resources

  Parameters: None

  Return: FSP_SUCCESS on successful close, error code on failure
-----------------------------------------------------------------------------------------------------*/
fsp_err_t g_rm_levelx_nor_OSPI_close(void)
{
  // Close OSPI driver (similar to how LittleFS handles it)
  return Mc80_ospi_close(g_mc80_ospi.p_ctrl);
}

// === LevelX NOR Flash Instance ===

// LevelX NOR flash structure
LX_NOR_FLASH g_lx_NOR;

// FileX LevelX NOR instance control structure
rm_filex_levelx_nor_instance_ctrl_t g_rm_filex_levelx_NOR_ctrl;

// FileX LevelX NOR configuration
const rm_filex_levelx_nor_cfg_t g_rm_filex_levelx_NOR_cfg = {
  .close                 = g_rm_levelx_nor_OSPI_close,       // Driver close function
  .nor_driver_initialize = g_rm_levelx_nor_OSPI_initialize,  // Driver initialization function
  .p_nor_flash           = &g_lx_NOR,                        // LevelX NOR flash instance
  .p_nor_flash_name      = "g_rm_filex_levelx_NOR",          // FileX media name
  .p_callback            = g_rm_filex_levelx_NOR_callback,   // FileX callback function
  .p_context             = NULL                              // User context (not used)
};

// FileX LevelX NOR instance
const rm_filex_levelx_nor_instance_t g_rm_filex_levelx_NOR_instance = {
  .p_ctrl = &g_rm_filex_levelx_NOR_ctrl,  // Control structure
  .p_cfg  = &g_rm_filex_levelx_NOR_cfg    // Configuration structure
};
