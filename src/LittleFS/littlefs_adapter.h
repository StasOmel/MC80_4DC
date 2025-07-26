#ifndef LITTLEFS_ADAPTER_H
#define LITTLEFS_ADAPTER_H

#include "lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum size for LittleFS configuration based on MX25UM25645G datasheet
#define LITTLEFS_BLOCK_SIZE        4096    // 4KB sectors (matches flash erase sector size)
#define LITTLEFS_BLOCK_COUNT       64      // Use 256KB (64 * 4KB) for minimal testing
#define LITTLEFS_CACHE_SIZE        256     // Cache size (matches 256-byte page buffer)
#define LITTLEFS_LOOKAHEAD_SIZE    16      // Lookahead buffer size (64/8 = 8, use 16 for safety, must be multiple of 8)
#define LITTLEFS_BLOCK_CYCLES      100000  // 100,000 erase/program cycles (per datasheet)
#define LITTLEFS_PROG_SIZE         256     // 256-byte page buffer (per datasheet)

// OSPI protocol configuration for LittleFS operations
#define LITTLEFS_OSPI_PROTOCOL     MC80_OSPI_PROTOCOL_8D_8D_8D  // Can be changed to MC80_OSPI_PROTOCOL_8D_8D_8D

// Debug output control for LittleFS adapter
// Set LITTLEFS_DEBUG_ENABLE to 0 to disable all debug output from LittleFS adapter
// Set LITTLEFS_DEBUG_ENABLE to 1 to enable debug output (default)
#ifndef LITTLEFS_DEBUG_ENABLE
#define LITTLEFS_DEBUG_ENABLE 1  // Set to 0 to disable debug output
#endif

#if LITTLEFS_DEBUG_ENABLE
#define LITTLEFS_DEBUG_PRINTF(channel, ...) RTT_printf(channel, __VA_ARGS__)
#define LITTLEFS_DEBUG_ERR_PRINTF(channel, ...) RTT_err_printf(channel, __VA_ARGS__)
#else
#define LITTLEFS_DEBUG_PRINTF(channel, ...) do {} while(0)
#define LITTLEFS_DEBUG_ERR_PRINTF(channel, ...) do {} while(0)
#endif

// LittleFS instance structure
typedef struct
{
  lfs_t lfs;                     // LittleFS filesystem
  struct lfs_config cfg;         // LittleFS configuration
  uint8_t read_buffer[LITTLEFS_CACHE_SIZE];
  uint8_t prog_buffer[LITTLEFS_CACHE_SIZE];
  uint8_t lookahead_buffer[LITTLEFS_LOOKAHEAD_SIZE];
  bool driver_initialized;       // Flag to track OSPI driver state
  bool filesystem_mounted;       // Flag to track filesystem mount state
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
int  _lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int  _lfs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int  _lfs_erase(const struct lfs_config *c, lfs_block_t block);
int  _lfs_sync(const struct lfs_config *c);

#ifdef __cplusplus
}
#endif

#endif // LITTLEFS_ADAPTER_H
