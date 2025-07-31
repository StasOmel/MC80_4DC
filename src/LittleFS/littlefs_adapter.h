#ifndef LITTLEFS_ADAPTER_H
#define LITTLEFS_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mx25um25645g.h"

// LittleFS configuration based on global MC80 NOR Flash macros
// Uses MC80_NOR_FLASH_* macros defined in src\MC80.h for hardware abstraction
#define LITTLEFS_BLOCK_SIZE     MC80_NOR_FLASH_SECTOR_SIZE_BYTES    // Use global sector size (4KB)
#define LITTLEFS_BLOCK_COUNT    MC80_NOR_FLASH_TOTAL_SECTORS        // Use global sector count
#define LITTLEFS_CACHE_SIZE     1024                                // Cache size (must be <= block_size and multiple of read/prog sizes)
                                                                    // NOTE: Reducing from 2048 to 1024 has minimal performance impact
                                                                    // NOTE: Reducing from 2048 to 64 decreases read performance by ~20%
#define LITTLEFS_LOOKAHEAD_SIZE (MC80_NOR_FLASH_TOTAL_SECTORS / 8)  // Lookahead buffer size (sectors/8)
#define LITTLEFS_BLOCK_CYCLES   1000                                // Maximum erase cycles per block
#define LITTLEFS_READ_SIZE      64                                  // Minimum read size (optimized for OSPI flash)
#define LITTLEFS_PROG_SIZE      64                                  // Minimum program size (optimized for OSPI flash)

// Data integrity and performance settings
#define LITTLEFS_NAME_MAX       LFS_NAME_MAX  // Maximum filename length (default 255)
#define LITTLEFS_FILE_MAX       LFS_FILE_MAX  // Maximum file size (default 2147483647)
#define LITTLEFS_ATTR_MAX       LFS_ATTR_MAX  // Maximum custom attribute size (default 1022)

// Note: LittleFS automatically handles:
// - Block-level CRC32 checksums for metadata integrity
// - Copy-on-write semantics to prevent corruption
// - Wear leveling across flash blocks
// - Bad block management and error recovery

// OSPI protocol configuration for LittleFS operations
#define LITTLEFS_OSPI_PROTOCOL  MC80_OSPI_PROTOCOL_8D_8D_8D  // Can be changed to MC80_OSPI_PROTOCOL_8D_8D_8D

// Debug output control for LittleFS adapter
// Set LITTLEFS_DEBUG_ENABLE to 0 to disable all debug output from LittleFS adapter
// Set LITTLEFS_DEBUG_ENABLE to 1 to enable debug output (default)
#ifndef LITTLEFS_DEBUG_ENABLE
  #define LITTLEFS_DEBUG_ENABLE 0  // Set to 0 to disable debug output
#endif

#if LITTLEFS_DEBUG_ENABLE
  #include "RTT_utils.h"
  #define LITTLEFS_DEBUG_PRINTF(format, ...)     RTT_printf(0, "[FS] %-25s:%4d " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
  #define LITTLEFS_DEBUG_ERR_PRINTF(format, ...) RTT_err_printf(0, "[FS] %-25s:%4d " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
  #define LITTLEFS_DEBUG_PRINTF(format, ...)     ((void)0)
  #define LITTLEFS_DEBUG_ERR_PRINTF(format, ...) ((void)0)
#endif

  // LittleFS instance structure
  typedef struct
  {
    lfs_t             lfs;  // LittleFS filesystem
    struct lfs_config cfg;  // LittleFS configuration
    uint8_t           read_buffer[LITTLEFS_CACHE_SIZE];
    uint8_t           prog_buffer[LITTLEFS_CACHE_SIZE];
    uint8_t           lookahead_buffer[LITTLEFS_LOOKAHEAD_SIZE];
    bool              driver_initialized;  // Flag to track OSPI driver state
    bool              filesystem_mounted;  // Flag to track filesystem mount state
  } T_littlefs_context;

  // Global LittleFS context
  extern T_littlefs_context g_littlefs_context;

  // Function prototypes
  int  Littlefs_initialize(void);
  int  Littlefs_mount(void);
  int  Littlefs_format(void);
  int  Littlefs_unmount(void);
  bool Littlefs_is_initialized(void);
  bool Littlefs_is_mounted(void);

  // LFS driver functions - these are called by LittleFS
  int _lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
  int _lfs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
  int _lfs_erase(const struct lfs_config *c, lfs_block_t block);
  int _lfs_sync(const struct lfs_config *c);

#ifdef __cplusplus
}
#endif

#endif  // LITTLEFS_ADAPTER_H
