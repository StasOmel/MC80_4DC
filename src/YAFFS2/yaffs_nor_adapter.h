#ifndef YAFFS_NOR_ADAPTER_H
#define YAFFS_NOR_ADAPTER_H

#include "App.h"
#include "yaffs_nor_config.h"

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 NOR Flash Adapter Interface

  This header provides the interface between YAFFS2 filesystem and the MC80 OSPI NOR Flash driver.
  It implements the required callback functions for YAFFS2 to operate on NOR Flash memory.
-----------------------------------------------------------------------------------------------------*/

// Forward declarations for YAFFS2 structures
struct yaffs_dev;
struct yaffs_ext_tags;

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 Callback Functions for NOR Flash Operations

  These functions implement the required interface between YAFFS2 and the underlying flash hardware.
  All functions are designed to be deterministic with guaranteed maximum execution times.
-----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------------
  Write a page with tags to NOR Flash

  Parameters:
    dev      - YAFFS device structure
    chunk_id - Page identifier (sequential numbering)
    data     - User data buffer (2048 bytes, NULL for metadata-only)
    tags     - YAFFS metadata tags (stored in OOB area)

  Return:
    YAFFS_OK on success, YAFFS_FAIL on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_write_chunk_tags(struct yaffs_dev *dev, int chunk_id,
                               const unsigned char *data,
                               const struct yaffs_ext_tags *tags);

/*-----------------------------------------------------------------------------------------------------
  Read a page with tags from NOR Flash

  Parameters:
    dev      - YAFFS device structure
    chunk_id - Page identifier (sequential numbering)
    data     - Buffer for user data (2048 bytes, NULL to skip)
    tags     - Buffer for YAFFS metadata tags (NULL to skip)

  Return:
    YAFFS_OK on success, YAFFS_FAIL on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_read_chunk_tags(struct yaffs_dev *dev, int chunk_id,
                              unsigned char *data,
                              struct yaffs_ext_tags *tags);

/*-----------------------------------------------------------------------------------------------------
  Erase a block in NOR Flash

  Parameters:
    dev      - YAFFS device structure
    block_no - Block number to erase

  Return:
    YAFFS_OK on success, YAFFS_FAIL on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_erase_block(struct yaffs_dev *dev, int block_no);

/*-----------------------------------------------------------------------------------------------------
  Check if a block is bad

  Parameters:
    dev      - YAFFS device structure
    block_no - Block number to check

  Return:
    YAFFS_OK if good, YAFFS_FAIL if bad
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_check_bad_block(struct yaffs_dev *dev, int block_no);

/*-----------------------------------------------------------------------------------------------------
  Mark a block as bad

  Parameters:
    dev      - YAFFS device structure
    block_no - Block number to mark as bad

  Return:
    YAFFS_OK on success
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_mark_bad_block(struct yaffs_dev *dev, int block_no);

/*-----------------------------------------------------------------------------------------------------
  Performance Statistics Management
-----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------------
  Initialize performance statistics

  Reset all counters and timing measurements. Should be called before mounting filesystem.

  Return: None
-----------------------------------------------------------------------------------------------------*/
void Yaffs_nor_stats_init(void);

/*-----------------------------------------------------------------------------------------------------
  Print current performance statistics

  Output comprehensive performance metrics and determinism analysis.

  Return: None
-----------------------------------------------------------------------------------------------------*/
void Yaffs_nor_stats_print(void);

/*-----------------------------------------------------------------------------------------------------
  Inline helper functions for address calculations
-----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------------
  Convert YAFFS chunk ID to physical NOR Flash address

  Parameters:
    chunk_id - YAFFS chunk (page) identifier

  Return:
    Physical address in NOR Flash
-----------------------------------------------------------------------------------------------------*/
static inline uint32_t Yaffs_nor_chunk_to_address(int chunk_id)
{
  return YAFFS_NOR_CHUNK_TO_ADDRESS(chunk_id);
}

/*-----------------------------------------------------------------------------------------------------
  Convert YAFFS block number to physical NOR Flash address

  Parameters:
    block_no - YAFFS block number

  Return:
    Physical address in NOR Flash
-----------------------------------------------------------------------------------------------------*/
static inline uint32_t Yaffs_nor_block_to_address(int block_no)
{
  return YAFFS_NOR_BLOCK_TO_ADDRESS(block_no);
}

/*-----------------------------------------------------------------------------------------------------
  Get current performance statistics

  Return:
    Pointer to current statistics structure
-----------------------------------------------------------------------------------------------------*/
static inline const T_yaffs_nor_stats* Yaffs_nor_get_stats(void)
{
  return &g_yaffs_nor_stats;
}

#endif // YAFFS_NOR_ADAPTER_H
