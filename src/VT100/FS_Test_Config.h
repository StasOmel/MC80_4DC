#ifndef FS_TEST_CONFIG_H
#define FS_TEST_CONFIG_H

// Common filesystem test configuration
#define FS_TEST_FILES_COUNT_DEFAULT    10        // Default number of files
#define FS_TEST_FILE_SIZE_DEFAULT      (10*1024) // Default file size (10 KB)
#define FS_TEST_BLOCK_SIZE_DEFAULT     (64*1024) // Default block size (64 KB)
#define FS_TEST_FILE_PREFIX            "test_"   // File name prefix
#define FS_MAX_FILENAME_LENGTH         64        // Maximum filename length
#define FS_CRC32_SIZE                  4         // CRC32 size in bytes

#endif // FS_TEST_CONFIG_H
