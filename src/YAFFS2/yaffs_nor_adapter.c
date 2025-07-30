#include "App.h"
#include "yaffs_nor_adapter.h"
#include "yaffs_nor_config.h"
#include "MC80_OSPI_drv.h"
#include "MC80_OSPI_config.h"

// Compile-time check for inband tags mode (tags are stored within data area)
_Static_assert(YAFFS_NOR_PAGE_OOB_SIZE == 0,
               "OOB size must be 0 for inband tags mode");


/*-----------------------------------------------------------------------------------------------------
  External reference to OSPI driver control structure
  This should be initialized by the main application before using YAFFS2
-----------------------------------------------------------------------------------------------------*/
extern T_mc80_ospi_instance_ctrl g_OSPI_ctrl;

/*-----------------------------------------------------------------------------------------------------
  Erase block in NOR Flash

  Parameters:
    dev      - YAFFS device structure
    block_no - Block number to erase (0-based)

  Return:
    YAFFS_OK on success, YAFFS_FAIL on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_erase_block(struct yaffs_dev *dev, int block_no)
{
  fsp_err_t err;
  uint32_t block_address;

  // Parameter validation
  if (NULL == dev || block_no < 0 || block_no >= YAFFS_NOR_TOTAL_BLOCKS)
  {
    return YAFFS_FAIL;
  }

  // Calculate physical address of block in NOR Flash
  block_address = YAFFS_NOR_BLOCK_TO_ADDRESS(block_no);

  // Erase block using OSPI driver
  err = Mc80_ospi_erase(&g_OSPI_ctrl, block_address, YAFFS_NOR_BLOCK_SIZE);

  if (FSP_SUCCESS == err)
  {
    return YAFFS_OK;
  }
  else
  {
    return YAFFS_FAIL;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Bad block checking function for NOR Flash

  NOR Flash typically doesn't have factory bad blocks like NAND Flash.
  This function always returns good status but could be extended to track
  blocks that fail during runtime.

  Parameters:
    dev      - YAFFS device structure
    block_no - Block number to check

  Return:
    YAFFS_OK if block is good, YAFFS_FAIL if block is bad
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_check_bad_block(struct yaffs_dev *dev, int block_no)
{
  // NOR Flash typically doesn't have bad blocks
  // Could be extended to track runtime failures
  return YAFFS_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Bad block marking function for NOR Flash

  Parameters:
    dev      - YAFFS device structure
    block_no - Block number to mark as bad

  Return:
    YAFFS_OK on success
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_mark_bad_block(struct yaffs_dev *dev, int block_no)
{
  return YAFFS_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Initialize NOR Flash driver

  Parameters:
    dev - YAFFS device structure

  Return:
    YAFFS_OK on success, YAFFS_FAIL on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_initialise(struct yaffs_dev *dev)
{
  fsp_err_t err;

  // Initialize OSPI driver first (similar to LittleFS and LevelX initialization)
  err = Mc80_ospi_open(g_mc80_ospi.p_ctrl, g_mc80_ospi.p_cfg);
  if (err != FSP_SUCCESS)
  {
    // Check if driver is already opened
    if (err == FSP_ERR_ALREADY_OPEN)
    {
      // Driver already opened, this is acceptable
    }
    else
    {
      // Initialization failed
      return YAFFS_FAIL;
    }
  }

  // Set OSPI protocol as configured for YAFFS2 (configurable via YAFFS_NOR_OSPI_PROTOCOL)
  err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, YAFFS_NOR_OSPI_PROTOCOL);
  if (err != FSP_SUCCESS)
  {
    return YAFFS_FAIL;
  }

  return YAFFS_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Deinitialize NOR Flash driver

  Parameters:
    dev - YAFFS device structure

  Return:
    YAFFS_OK on success
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_deinitialise(struct yaffs_dev *dev)
{
  // Note: In this implementation, we don't close the OSPI driver because
  // it might be shared with other filesystems (LittleFS, LevelX, etc.)
  // The driver will be closed when the application shuts down

  return YAFFS_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Write chunk to NOR Flash

  Parameters:
    dev      - YAFFS device structure
    chunk_id - Page identifier (0-based sequential)
    data     - User data buffer (includes inband tags at end if present)
    data_len - Length of data buffer
    oob      - Out-of-band data (not used in inband tags mode)
    oob_len  - Length of OOB buffer

  Return:
    YAFFS_OK on success, YAFFS_FAIL on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_write_chunk(struct yaffs_dev *dev, int chunk_id,
                          const u8 *data, int data_len,
                          const u8 *oob, int oob_len)
{
  fsp_err_t err;
  uint32_t page_address;

  // Parameter validation
  if (NULL == dev || chunk_id < 0)
  {
    return YAFFS_FAIL;
  }

  FSP_PARAMETER_NOT_USED(data_len);  // Length should match page size
  FSP_PARAMETER_NOT_USED(oob_len);   // Not used in inband tags mode
  FSP_PARAMETER_NOT_USED(oob);       // Not used in inband tags mode

  // Calculate physical address in NOR Flash
  page_address = YAFFS_NOR_CHUNK_TO_ADDRESS(chunk_id);

  if (NULL != data)
  {
    // Direct write from user data buffer (includes inband tags at end if present)
    err = Mc80_ospi_memory_mapped_write(&g_OSPI_ctrl,
                                       data,
                                       page_address,
                                       YAFFS_NOR_PAGE_DATA_SIZE);
  }
  else
  {
    // Metadata-only write: NOR Flash erased state (0xFF) is the desired state
    err = FSP_SUCCESS;
  }

  if (FSP_SUCCESS == err)
  {
    return YAFFS_OK;
  }
  else
  {
    return YAFFS_FAIL;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Read chunk from NOR Flash

  Parameters:
    dev        - YAFFS device structure
    chunk_id   - Page identifier (0-based sequential)
    data       - Buffer for user data (includes inband tags at end after read)
    data_len   - Length of data buffer
    oob        - Buffer for out-of-band data (not used in inband tags mode)
    oob_len    - Length of OOB buffer
    ecc_result - ECC correction result (output)

  Return:
    YAFFS_OK on success, YAFFS_FAIL on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_read_chunk(struct yaffs_dev *dev, int chunk_id,
                         u8 *data, int data_len,
                         u8 *oob, int oob_len,
                         enum yaffs_ecc_result *ecc_result)
{
  fsp_err_t err;
  uint32_t page_address;

  // Parameter validation
  if (NULL == dev || chunk_id < 0)
  {
    return YAFFS_FAIL;
  }

  FSP_PARAMETER_NOT_USED(data_len);  // Length should match page size
  FSP_PARAMETER_NOT_USED(oob_len);   // Not used in inband tags mode
  FSP_PARAMETER_NOT_USED(oob);       // Not used in inband tags mode

  // Calculate physical address in NOR Flash
  page_address = YAFFS_NOR_CHUNK_TO_ADDRESS(chunk_id);

  if (NULL != data)
  {
    // Direct read into user data buffer (includes inband tags at end)
    err = Mc80_ospi_memory_mapped_read(&g_OSPI_ctrl,
                                      data,
                                      page_address,
                                      YAFFS_NOR_PAGE_DATA_SIZE);
  }
  else
  {
    // If no data buffer provided, just return success (tags-only read not supported in inband mode)
    err = FSP_SUCCESS;
  }

  // Set ECC result to indicate no ECC errors (NOR Flash is reliable)
  if (ecc_result)
  {
    *ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
  }

  if (FSP_SUCCESS == err)
  {
    return YAFFS_OK;
  }
  else
  {
    return YAFFS_FAIL;
  }
}
