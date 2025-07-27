#ifndef LITTLEFS_ADAPTER_H
#define LITTLEFS_ADAPTER_H


// LittleFS memory allocation macros using project memory management
#define LFS_MALLOC(size) App_malloc(size)
#define LFS_FREE(ptr)    App_free(ptr)

#ifdef __cplusplus
extern "C"
{
#endif

// Maximum size for LittleFS configuration based on MX25UM25645G datasheet
// MX25UM25645G: 256Mbit (32MB) OSPI NOR Flash Memory
#define LITTLEFS_BLOCK_SIZE     4096    // 4KB sectors (matches flash erase sector size)
#define LITTLEFS_BLOCK_COUNT    8192    // 32MB total = 8192 blocks of 4KB each (32MB / 4KB = 8192)
#define LITTLEFS_CACHE_SIZE     256     // Cache size (matches 256-byte page buffer)
#define LITTLEFS_LOOKAHEAD_SIZE 128     // Lookahead buffer size (8192/64 = 128, must be multiple of 8)
#define LITTLEFS_BLOCK_CYCLES   100000  // 100,000 erase/program cycles (per datasheet)
#define LITTLEFS_PROG_SIZE      256     // 256-byte page buffer (per datasheet)

// OSPI protocol configuration for LittleFS operations
#define LITTLEFS_OSPI_PROTOCOL  MC80_OSPI_PROTOCOL_1S_1S_1S  // Can be changed to MC80_OSPI_PROTOCOL_8D_8D_8D

// Debug output control for LittleFS adapter
// Set LITTLEFS_DEBUG_ENABLE to 0 to disable all debug output from LittleFS adapter
// Set LITTLEFS_DEBUG_ENABLE to 1 to enable debug output (default)
#ifndef LITTLEFS_DEBUG_ENABLE
  #define LITTLEFS_DEBUG_ENABLE 1  // Set to 0 to disable debug output
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
