#ifndef LEVELX_CONFIG_H
#define LEVELX_CONFIG_H

#define G_FX_MEDIA_OSPI_NOR_MEDIA_MEMORY_SIZE    (2048)  // Increased buffer size for better performance
#define G_FX_MEDIA_OSPI_NOR_VOLUME_NAME          ("Volume 1")
#define G_FX_MEDIA_OSPI_NOR_NUMBER_OF_FATS       (1)
#define G_FX_MEDIA_OSPI_NOR_DIRECTORY_ENTRIES    (2048)   // Increased for more files
#define G_FX_MEDIA_OSPI_NOR_HIDDEN_SECTORS       (0)
#define G_FX_MEDIA_OSPI_NOR_TOTAL_SECTORS        (65536) // Corrected: 32MB / 512 bytes per sector
#define G_FX_MEDIA_OSPI_NOR_BYTES_PER_SECTOR     (512)
#define G_FX_MEDIA_OSPI_NOR_SECTORS_PER_CLUSTER  (1)
#define G_FX_MEDIA_OSPI_NOR_VOLUME_SERIAL_NUMBER (12345)
#define G_FX_MEDIA_OSPI_NOR_BOUNDARY_UNIT        (128)

// === LevelX NOR Flash Memory Configuration ===
// Hardware configuration for MX25UM25645G (32MB OSPI Flash)
#define LEVELX_FLASH_TOTAL_SIZE_BYTES    (33554432)  // 32MB = 33,554,432 bytes
#define LEVELX_BLOCK_SIZE_BYTES          (65536)     // 64KB per block (LevelX block size)
#define LEVELX_BYTES_PER_WORD            (4)         // 4 bytes per ULONG word
#define LEVELX_TOTAL_BLOCKS              (LEVELX_FLASH_TOTAL_SIZE_BYTES / LEVELX_BLOCK_SIZE_BYTES)  // 512 blocks
#define LEVELX_WORDS_PER_BLOCK           (LEVELX_BLOCK_SIZE_BYTES / LEVELX_BYTES_PER_WORD)          // 16384 words per block

// OSPI Protocol Selection
// Choose one of the following protocols for OSPI communication:
// - MC80_OSPI_PROTOCOL_1S_1S_1S: Standard SPI mode (most compatible, slower performance)
// - MC80_OSPI_PROTOCOL_8D_8D_8D: Octal DTR mode (highest performance, requires stable setup)
#define LEVELX_OSPI_PROTOCOL    MC80_OSPI_PROTOCOL_8D_8D_8D


// Forward declarations for callback functions
void g_rm_filex_levelx_NOR_callback(rm_filex_levelx_nor_callback_args_t *p_args);

// External declarations for LevelX NOR OSPI configuration structures

// LevelX NOR flash instance
extern LX_NOR_FLASH g_lx_NOR;

// FileX media instance for LevelX NOR
extern FX_MEDIA g_fx_spi_nor_media;

// FileX LevelX NOR instance control structure
extern rm_filex_levelx_nor_instance_ctrl_t g_rm_filex_levelx_NOR_ctrl;

// FileX LevelX NOR configuration
extern const rm_filex_levelx_nor_cfg_t g_rm_filex_levelx_NOR_cfg;

// FileX LevelX NOR instance
extern const rm_filex_levelx_nor_instance_t g_rm_filex_levelx_NOR_instance;

// LevelX NOR read buffer (if LX_DIRECT_READ is disabled)
#ifndef LX_DIRECT_READ
  #define FSP_LX_READ_BUFFER_SIZE_WORDS (128U)
extern ULONG g_rm_levelx_nor_OSPI_read_buffer[FSP_LX_READ_BUFFER_SIZE_WORDS];
#endif

// LevelX NOR driver functions
UINT      g_rm_levelx_nor_OSPI_initialize(LX_NOR_FLASH *p_nor_flash);
fsp_err_t g_rm_levelx_nor_OSPI_close(void);
UINT      g_rm_levelx_nor_OSPI_system_error(UINT error_code);

// FileX LevelX NOR device driver function
void MC80_FileX_LevelX_DeviceDriver(FX_MEDIA *p_fx_media);

// FileX LevelX NOR callback function
void g_rm_filex_levelx_NOR_callback(rm_filex_levelx_nor_callback_args_t *p_args);

#endif  // LEVELX_CONFIG_H
