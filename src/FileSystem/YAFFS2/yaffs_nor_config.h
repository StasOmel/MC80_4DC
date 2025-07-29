/*-----------------------------------------------------------------------------------------------------
  YAFFS2 NOR Flash Configuration Header

  This file defines the page structure and layout for YAFFS2 on NOR Flash
  with embedded OOB (Out-Of-Band) data within each page
-----------------------------------------------------------------------------------------------------*/

#ifndef YAFFS_NOR_CONFIG_H
#define YAFFS_NOR_CONFIG_H

#include "App.h"

/*-----------------------------------------------------------------------------------------------------
  NOR Flash Memory Layout Configuration
-----------------------------------------------------------------------------------------------------*/

// YAFFS2 page structure for NOR Flash (2048 + 64 bytes)
#define YAFFS_NOR_PAGE_DATA_SIZE    2048  // User data area
#define YAFFS_NOR_PAGE_OOB_SIZE     64    // Out-of-band metadata area
#define YAFFS_NOR_PAGE_TOTAL_SIZE   (YAFFS_NOR_PAGE_DATA_SIZE + YAFFS_NOR_PAGE_OOB_SIZE)

// Block organization (64 pages per block)
#define YAFFS_NOR_PAGES_PER_BLOCK   64
#define YAFFS_NOR_BLOCK_SIZE        (YAFFS_NOR_PAGES_PER_BLOCK * YAFFS_NOR_PAGE_TOTAL_SIZE)

// Filesystem layout on 32MB NOR Flash
#define YAFFS_NOR_FLASH_SIZE        (32 * 1024 * 1024)  // 32MB total
#define YAFFS_NOR_TOTAL_BLOCKS      (YAFFS_NOR_FLASH_SIZE / YAFFS_NOR_BLOCK_SIZE)
#define YAFFS_NOR_TOTAL_PAGES       (YAFFS_NOR_TOTAL_BLOCKS * YAFFS_NOR_PAGES_PER_BLOCK)

// Reserved blocks for wear leveling and bad block replacement
#define YAFFS_NOR_RESERVED_BLOCKS   8
#define YAFFS_NOR_USABLE_BLOCKS     (YAFFS_NOR_TOTAL_BLOCKS - YAFFS_NOR_RESERVED_BLOCKS)

/*-----------------------------------------------------------------------------------------------------
  Address Translation Macros
-----------------------------------------------------------------------------------------------------*/

// Convert chunk ID to physical flash address
#define YAFFS_NOR_CHUNK_TO_ADDRESS(chunk_id) \
  ((uint32_t)(chunk_id) * YAFFS_NOR_PAGE_TOTAL_SIZE)

// Convert block number to physical flash address
#define YAFFS_NOR_BLOCK_TO_ADDRESS(block_no) \
  ((uint32_t)(block_no) * YAFFS_NOR_BLOCK_SIZE)

// Extract block number from chunk ID
#define YAFFS_NOR_CHUNK_TO_BLOCK(chunk_id) \
  ((chunk_id) / YAFFS_NOR_PAGES_PER_BLOCK)

// Extract page number within block from chunk ID
#define YAFFS_NOR_CHUNK_TO_PAGE(chunk_id) \
  ((chunk_id) % YAFFS_NOR_PAGES_PER_BLOCK)

/*-----------------------------------------------------------------------------------------------------
  Page Structure Definition
-----------------------------------------------------------------------------------------------------*/

// Complete page structure with embedded OOB
typedef struct
{
  uint8_t data[YAFFS_NOR_PAGE_DATA_SIZE];  // User data (2048 bytes)
  uint8_t oob[YAFFS_NOR_PAGE_OOB_SIZE];    // Metadata/tags (64 bytes)
} T_yaffs_nor_page;

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 Return Codes (compatibility with YAFFS2 core)
-----------------------------------------------------------------------------------------------------*/

#define YAFFS_OK    0
#define YAFFS_FAIL  1

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 Device Configuration Constants
-----------------------------------------------------------------------------------------------------*/

// Block range for YAFFS2 filesystem
#define YAFFS_NOR_START_BLOCK                   0
#define YAFFS_NOR_END_BLOCK                     (YAFFS_NOR_TOTAL_BLOCKS - 1)

// YAFFS2 feature configuration
#define YAFFS_NOR_USE_HEADER_FILE_SIZE          1
#define YAFFS_NOR_DISABLE_LAZY_LOAD             0
#define YAFFS_NOR_REFRESH_PERIOD                10
#define YAFFS_NOR_CACHE_SIZE                    10
#define YAFFS_NOR_DISABLE_BACKGROUND_GC         1
#define YAFFS_NOR_EMPTY_LOST_AND_FOUND          1
#define YAFFS_NOR_NO_TAGS_ECC                   0
#define YAFFS_NOR_INBAND_TAGS                   0
#define YAFFS_NOR_ALWAYS_CHECK_CHECKPT          1
#define YAFFS_NOR_AUTO_CHECKPT                  1
#define YAFFS_NOR_WIDE_TNODES                   1

// Garbage collection control
#define YAFFS_GC_CONTROL_DISABLE_BG             1

#endif // YAFFS_NOR_CONFIG_H
