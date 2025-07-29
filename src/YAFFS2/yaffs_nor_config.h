#ifndef YAFFS_NOR_CONFIG_H
#define YAFFS_NOR_CONFIG_H

#include "App.h"

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 NOR Flash Configuration for MC80 OSPI Driver

  This configuration adapts YAFFS2 (originally designed for NAND Flash) to work with NOR Flash
  by emulating NAND-like page structure with OOB data stored within each page.
-----------------------------------------------------------------------------------------------------*/

// Flash geometry configuration
#define YAFFS_NOR_PAGE_DATA_SIZE          (2048)     // User data per page (2KB)
#define YAFFS_NOR_PAGE_OOB_SIZE           (64)       // OOB area per page (64 bytes) - stored within page
#define YAFFS_NOR_PAGE_TOTAL_SIZE         (YAFFS_NOR_PAGE_DATA_SIZE + YAFFS_NOR_PAGE_OOB_SIZE)  // 2112 bytes total

#define YAFFS_NOR_PAGES_PER_BLOCK         (64)       // Pages per block (64 pages)
#define YAFFS_NOR_BLOCK_SIZE              (YAFFS_NOR_PAGE_TOTAL_SIZE * YAFFS_NOR_PAGES_PER_BLOCK)  // 135168 bytes per block

// Filesystem layout configuration
#define YAFFS_NOR_TOTAL_BLOCKS            (1900)     // Total blocks available (~256MB filesystem)
#define YAFFS_NOR_RESERVED_BLOCKS         (100)      // Reserved blocks for wear leveling and bad block management
#define YAFFS_NOR_START_BLOCK             (0)        // First block used by filesystem
#define YAFFS_NOR_END_BLOCK               (YAFFS_NOR_TOTAL_BLOCKS - 1)

// Memory management configuration
#define YAFFS_NOR_CACHE_SIZE              (50)       // Number of cached pages for performance
#define YAFFS_NOR_CHECKPOINT_BLOCKS       (10)       // Blocks reserved for checkpoints

// Deterministic timing guarantees (milliseconds)
#define YAFFS_MAX_WRITE_TIME_MS           (5)        // Maximum time for single page write
#define YAFFS_MAX_READ_TIME_MS            (2)        // Maximum time for single page read
#define YAFFS_MAX_ERASE_TIME_MS           (20)       // Maximum time for block erase
#define YAFFS_MAX_GC_TIME_MS              (50)       // Maximum time for garbage collection cycle

// YAFFS2 feature configuration
#define YAFFS_NOR_WIDE_TNODES             (1)        // Use wide tree nodes for large files
#define YAFFS_NOR_ALWAYS_CHECK_CHECKPT    (1)        // Always verify checkpoint integrity
#define YAFFS_NOR_AUTO_CHECKPT            (1)        // Enable automatic checkpointing
#define YAFFS_NOR_DISABLE_LAZY_LOAD       (1)        // Disable lazy loading for determinism
#define YAFFS_NOR_DISABLE_BACKGROUND_GC   (1)        // Disable background garbage collection

// Error handling and reliability
#define YAFFS_NOR_ENABLE_ECC              (1)        // Enable ECC for OOB data
#define YAFFS_NOR_INBAND_TAGS             (0)        // Use separate OOB area (not inband)
#define YAFFS_NOR_NO_TAGS_ECC             (0)        // Enable ECC for tags

// Performance optimization
#define YAFFS_NOR_REFRESH_PERIOD          (0)        // Disable automatic refresh (manual control)
#define YAFFS_NOR_USE_HEADER_FILE_SIZE    (1)        // Use header for file size information
#define YAFFS_NOR_EMPTY_LOST_AND_FOUND    (1)        // Keep lost+found directory empty

// Page structure with embedded OOB
typedef struct
{
  uint8_t data[YAFFS_NOR_PAGE_DATA_SIZE];           // User data area (2048 bytes)
  uint8_t oob[YAFFS_NOR_PAGE_OOB_SIZE];             // OOB area embedded in page (64 bytes)
} T_yaffs_nor_page;

// Block structure
typedef struct
{
  T_yaffs_nor_page pages[YAFFS_NOR_PAGES_PER_BLOCK]; // All pages in block
} T_yaffs_nor_block;

// Address conversion macros
#define YAFFS_NOR_CHUNK_TO_ADDRESS(chunk_id)         ((chunk_id) * YAFFS_NOR_PAGE_TOTAL_SIZE)
#define YAFFS_NOR_BLOCK_TO_ADDRESS(block_no)         ((block_no) * YAFFS_NOR_BLOCK_SIZE)
#define YAFFS_NOR_ADDRESS_TO_CHUNK(address)          ((address) / YAFFS_NOR_PAGE_TOTAL_SIZE)
#define YAFFS_NOR_ADDRESS_TO_BLOCK(address)          ((address) / YAFFS_NOR_BLOCK_SIZE)

// Performance monitoring structure
typedef struct
{
  uint32_t write_count;                             // Number of page writes
  uint32_t read_count;                              // Number of page reads
  uint32_t erase_count;                             // Number of block erases
  uint32_t gc_count;                                // Number of GC cycles
  uint32_t max_write_time_ms;                       // Maximum observed write time
  uint32_t max_read_time_ms;                        // Maximum observed read time
  uint32_t max_erase_time_ms;                       // Maximum observed erase time
  uint32_t max_gc_time_ms;                          // Maximum observed GC time
  uint32_t determinism_violations;                  // Count of timing violations
} T_yaffs_nor_stats;

// External statistics variable
extern T_yaffs_nor_stats g_yaffs_nor_stats;

#endif // YAFFS_NOR_CONFIG_H
