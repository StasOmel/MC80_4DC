#ifndef FS_TEST_CONFIG_H
#define FS_TEST_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// Common filesystem test configuration constants
#define FS_TEST_FILES_COUNT_DEFAULT 10           // Default number of files
#define FS_TEST_FILE_SIZE_DEFAULT   (10 * 1024)  // Default file size (10 KB)
#define FS_TEST_BLOCK_SIZE_DEFAULT  (64 * 1024)  // Default block size (64 KB)
#define FS_TEST_FILE_PREFIX         "test_"      // File name prefix
#define FS_MAX_FILENAME_LENGTH      64           // Maximum filename length
#define FS_CRC32_SIZE               4            // CRC32 size in bytes

// Test configuration structure
typedef struct
{
  uint32_t files_count;        // Number of test files
  uint32_t file_size;          // Size of each test file in bytes
  uint32_t block_size;         // Block size for I/O operations
  uint32_t data_pattern;       // Data pattern type (DATA_PATTERN_*)
  uint32_t fill_constant;      // Constant value for pattern fill
  bool     data_verification;  // Enable/disable data verification
} T_fs_test_config;

// Global test configuration instance
extern T_fs_test_config g_fs_test_config;

// Configuration management functions
void Fs_test_config_init(void);
void Fs_test_config_reset_to_defaults(void);

#endif  // FS_TEST_CONFIG_H
