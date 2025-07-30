#ifndef YAFFS_NOR_CONFIG_H
#define YAFFS_NOR_CONFIG_H

#include "App.h"

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 Platform Configuration Defines

  These defines tell YAFFS2 what type of environment it's running in
-----------------------------------------------------------------------------------------------------*/
#define CONFIG_YAFFS_DIRECT             (1)  // Direct interface mode
#define CONFIG_YAFFS_YAFFS2             (1)  // Enable YAFFS2 support
#define CONFIG_YAFFS_PROVIDE_DEFS       (1)  // Provide standard definitions
#define CONFIG_YAFFSFS_PROVIDE_VALUES   (1)  // Provide filesystem values
#define CONFIG_YAFFS_DEFINES_TYPES      (0)  // Don't redefine types (use our own)
#define CONFIG_YAFFS_SHORT_NAMES_IN_RAM (1)  // Use short names in RAM
#define CONFIG_YAFFS_USE_32_BIT_TIME_T  (1)  // Use 32-bit time

// Define types locally to avoid conflicts
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed int         s32;

typedef long long          off_t;   //
typedef long long          loff_t;  //

/* POSIX-style types for filesystem operations */
typedef unsigned int       mode_t;  // File mode/permissions type
typedef unsigned int       dev_t;   // Device ID type

/* Linux compatibility defines for embedded environment */
#ifndef LINUX_VERSION_CODE
#define LINUX_VERSION_CODE 0
#endif

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif


/* IATTR macros for embedded environment (bypass Linux version check) */
#ifndef IATTR_UID
#define IATTR_UID ia_uid
#define IATTR_GID ia_gid
#endif

/* IAR compiler pragma to suppress enum mixing warnings */
#ifdef __ICCARM__
#pragma diag_suppress=Pe188
#pragma diag_suppress=Pe175  // Suppress subscript out of range warning
#endif

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 NOR Flash Configuration for MC80 OSPI Driver

  This configuration adapts YAFFS2 (originally designed for NAND Flash) to work with NOR Flash
  by emulating NAND-like page structure with OOB data stored within each page.
-----------------------------------------------------------------------------------------------------*/

// Flash geometry configuration
#define YAFFS_NOR_PAGE_TOTAL_SIZE       (4096)                                                   // Total page size (exactly 4KB) - matches NOR Flash sector size
#define YAFFS_NOR_PAGE_OOB_SIZE         (0)                                                      // No separate OOB area (inband tags mode)
#define YAFFS_NOR_PAGE_DATA_SIZE        (YAFFS_NOR_PAGE_TOTAL_SIZE - YAFFS_NOR_PAGE_OOB_SIZE)    // Data area per page (4096 bytes, includes 16-byte inband tags)

#define YAFFS_NOR_PAGES_PER_BLOCK       (1)                                                      // Pages per block (1 page = 1 erasable sector)
#define YAFFS_NOR_BLOCK_SIZE            (YAFFS_NOR_PAGE_TOTAL_SIZE * YAFFS_NOR_PAGES_PER_BLOCK)  // 4096 bytes per block (4KB)

// Filesystem layout configuration
#define YAFFS_NOR_TOTAL_BLOCKS          (8000)  // Total blocks available (~32MB filesystem, 8000 × 4KB = 32MB)
#define YAFFS_NOR_RESERVED_BLOCKS       (200)   // Reserved blocks for wear leveling and bad block management
#define YAFFS_NOR_START_BLOCK           (0)     // First block used by filesystem
#define YAFFS_NOR_END_BLOCK             (YAFFS_NOR_TOTAL_BLOCKS - 1)

// Memory management configuration
#define YAFFS_NOR_CACHE_SIZE            (50)  // Number of cached pages for performance
#define YAFFS_NOR_CHECKPOINT_BLOCKS     (10)  // Blocks reserved for checkpoints

// YAFFS2 feature configuration
#define YAFFS_NOR_WIDE_TNODES           (1)  // Use wide tree nodes for large files
#define YAFFS_NOR_ALWAYS_CHECK_CHECKPT  (1)  // Always verify checkpoint integrity
#define YAFFS_NOR_AUTO_CHECKPT          (1)  // Enable automatic checkpointing
#define YAFFS_NOR_DISABLE_LAZY_LOAD     (1)  // Disable lazy loading for determinism
#define YAFFS_NOR_DISABLE_BACKGROUND_GC (1)  // Disable background garbage collection

// Error handling and reliability
#define YAFFS_NOR_ENABLE_ECC            (0)  // Disable ECC for OOB data
#define YAFFS_NOR_INBAND_TAGS           (1)  // Store tags inside data area (inband mode)
#define YAFFS_NOR_NO_TAGS_ECC           (1)  // Disable ECC for tags

// OSPI driver configuration
// Available protocols:
// - MC80_OSPI_PROTOCOL_1S_1S_1S: Standard SPI mode (reliable, supports all operations including erase)
// - MC80_OSPI_PROTOCOL_8D_8D_8D: Octal DDR mode (high performance, but erase operations must be done in SPI mode)
#define YAFFS_NOR_OSPI_PROTOCOL         MC80_OSPI_PROTOCOL_8D_8D_8D  // Standard SPI mode for YAFFS2

// Performance optimization
#define YAFFS_NOR_REFRESH_PERIOD        (0)  // Disable automatic refresh (manual control)
#define YAFFS_NOR_USE_HEADER_FILE_SIZE  (1)  // Use header for file size information
#define YAFFS_NOR_EMPTY_LOST_AND_FOUND  (1)  // Keep lost+found directory empty

// Page structure with inband tags (no separate OOB area)
typedef struct
{
  uint8_t data[YAFFS_NOR_PAGE_DATA_SIZE];  // Data area (4096 bytes) - user data + 16-byte inband tags at end
  uint8_t oob[YAFFS_NOR_PAGE_OOB_SIZE];    // No separate OOB area (0 bytes)
} T_yaffs_nor_page;

// Block structure (now contains only one page since 1 block = 1 erasable sector)
typedef struct
{
  T_yaffs_nor_page pages[YAFFS_NOR_PAGES_PER_BLOCK];  // Single page per block (4KB)
} T_yaffs_nor_block;

// Address conversion macros
#define YAFFS_NOR_CHUNK_TO_ADDRESS(chunk_id) ((chunk_id) * YAFFS_NOR_PAGE_TOTAL_SIZE)
#define YAFFS_NOR_BLOCK_TO_ADDRESS(block_no) ((block_no) * YAFFS_NOR_BLOCK_SIZE)
#define YAFFS_NOR_ADDRESS_TO_CHUNK(address)  ((address) / YAFFS_NOR_PAGE_TOTAL_SIZE)
#define YAFFS_NOR_ADDRESS_TO_BLOCK(address)  ((address) / YAFFS_NOR_BLOCK_SIZE)

#endif  // YAFFS_NOR_CONFIG_H
