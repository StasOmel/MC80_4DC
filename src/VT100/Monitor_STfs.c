#include "App.h"
#include "Performance_Stats.h"
#include "Test_Patterns.h"
#include "FS_Test_Config.h"
#include "STfs_api.h"

#define STFS_MEMORY_BUFFER_SIZE (32 * 1024)  // 32KB
#define STFS_TEST_BUFFER_SIZE   (16 * 1024)  // 16KB for test operations

// FileX media instance
extern FX_MEDIA g_fx_spi_nor_media;

// Global pointer to FileX memory buffer
static uint8_t *g_STfs_memory_buffer      = NULL;

// Global pointer to test buffer for file operations
static uint8_t *g_test_buffer              = NULL;

// Menu definition
const T_VT100_Menu_item MENU_STfs_items[] = {
  { '1', Do_STfs_init, NULL },
  { '2', Do_STfs_list_files, NULL },
  { '3', Do_STfs_performance_test, NULL },
  { '4', Do_STfs_defrag, NULL },
  { 'R', NULL, NULL },
  { 0 }  // End of menu
};

const T_VT100_Menu MENU_STfs = {
  "STfs File System Manager",
  "\033[5C STfs file system management menu\r\n"
  "\033[5C <1> - Initialize STfs (auto-format if needed)\r\n"
  "\033[5C <2> - List files\r\n"
  "\033[5C <3> - Performance test\r\n"
  "\033[5C <4> - Defragment STfs\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_STfs_items
};

/*-----------------------------------------------------------------------------------------------------
  Description: Static function declarations

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void        _Do_write_test(void);
static void        _Do_read_test(void);
static void        _Do_delete_test(void);
static void        _Do_format_test(void);
static void        _Do_full_test(void);
static void        _Print_STfs_info(void);
static void        _Print_test_config(void);
static const char *_Get_STfs_error_description(UINT status);
static bool        _Allocate_test_buffer(void);
static void        _Free_test_buffer(void);

/*-----------------------------------------------------------------------------------------------------
  Description: Get STfs error description string

  Parameters: status - STfs error status code

  Return: pointer to error description string
-----------------------------------------------------------------------------------------------------*/
static const char *_Get_STfs_error_description(UINT status)
{
  switch (status)
  {
    case STFS_OK:
      return "STFS_OK";
    case STFS_ERROR:
      return "STFS_ERROR";
    case STFS_NO_FREE_FCBL:
      return "STFS_NO_FREE_FCBL";
    case STFS_BAD_DRIVE:
      return "STFS_BAD_DRIVE";
    case STFS_NOT_CLOSED_FILE:
      return "STFS_NOT_CLOSED_FILE";
    case STFS_FILE_COPY_ERROR:
      return "STFS_FILE_COPY_ERROR";
    case STFS_FILE_NOT_FOUND:
      return "STFS_FILE_NOT_FOUND";
    case STFS_PROHIBITED_OPERATION:
      return "STFS_PROHIBITED_OPERATION";
    case STFS_ERRONEOUS_ARGUMENT:
      return "STFS_ERRONEOUS_ARGUMENT";
    case STFS_FILE_CREATE_ERROR1:
      return "STFS_FILE_CREATE_ERROR1";
    case STFS_FILE_CREATE_ERROR2:
      return "STFS_FILE_CREATE_ERROR2";
    case STFS_FILE_LOCATION_ERROR:
      return "STFS_FILE_LOCATION_ERROR";
    case STFS_INCOMPLETE_READING:
      return "STFS_INCOMPLETE_READING";
    case STFS_SECTOR_ALLOC_ERROR:
      return "STFS_SECTOR_ALLOC_ERROR";
    case STFS_ACCESS_ERROR:
      return "STFS_ACCESS_ERROR";
    case STFS_FILE_ALREADY_EXIST:
      return "STFS_FILE_ALREADY_EXIST";
    case STFS_FATAL_ERROR:
      return "STFS_FATAL_ERROR";
    case STFS_BAD_FILE_NAME:
      return "STFS_BAD_FILE_NAME";
    case STFS_CHUNK_DELETE_ERROR:
      return "STFS_CHUNK_DELETE_ERROR";
    case STFS_CHUNK_SIZE_ERR1:
      return "STFS_CHUNK_SIZE_ERR1";
    case STFS_CHUNK_SIZE_ERR2:
      return "STFS_CHUNK_SIZE_ERR2";
    case STFS_CHUNK_SIZE_ERR3:
      return "STFS_CHUNK_SIZE_ERR3";
    case STFS_CHUNK_SIZE_ERR4:
      return "STFS_CHUNK_SIZE_ERR4";
    case STFS_CHUNK_SIZE_ERR5:
      return "STFS_CHUNK_SIZE_ERR5";
    case STFS_CHUNK_SIZE_ERR6:
      return "STFS_CHUNK_SIZE_ERR6";
    case STFS_INCORRECT_DEL_TAG:
      return "STFS_INCORRECT_DEL_TAG";
    case STFS_DIRTY_SECTOR:
      return "STFS_DIRTY_SECTOR";
    case STFS_SECTOR_ERASE_ERROR1:
      return "STFS_SECTOR_ERASE_ERROR1";
    case STFS_SECTOR_ERASE_ERROR2:
      return "STFS_SECTOR_ERASE_ERROR2";
    case STFS_SECTOR_ERASE_ERROR3:
      return "STFS_SECTOR_ERASE_ERROR3";
    case STFS_FLASH_PROGR_ERROR1:
      return "STFS_FLASH_PROGR_ERROR1";
    // Note: STFS_FLASH_PROGR_ERROR2 and STFS_FLASH_PROGR_ERROR3 have same value as ERROR1 in STfs_api.h
    default:
      return "STFS_UNKNOWN_ERROR";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Allocate test buffer for file operations

  Parameters: none

  Return: true if allocation successful, false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Allocate_test_buffer(void)
{
  if (g_test_buffer == NULL)
  {
    g_test_buffer = App_malloc(STFS_TEST_BUFFER_SIZE);
    if (g_test_buffer == NULL)
    {
      return false;
    }
  }
  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Free test buffer

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Free_test_buffer(void)
{
  if (g_test_buffer != NULL)
  {
    App_free(g_test_buffer);
    g_test_buffer = NULL;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print STfs file system information

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_STfs_info(void)
{
  GET_MCBL;
  T_stfs_info stfs_info;

  MPRINTF("\n=== STfs File System Information ===\n\r");

  // Get STfs information using STfs_check function
  int32_t status = STfs_check(0, &stfs_info); // Using drive_id = 0
  if (status == STFS_OK)
  {
    uint32_t used_space_bytes = stfs_info.valid_aria_size;
    uint32_t free_space_bytes = stfs_info.empty_aria_size;
    uint32_t total_space_bytes = stfs_info.media_size;

    MPRINTF("Total space          : %lu KB (%lu MB)\n\r", total_space_bytes / 1024, total_space_bytes / (1024 * 1024));
    MPRINTF("Used space           : %lu KB (%lu MB)\n\r", used_space_bytes / 1024, used_space_bytes / (1024 * 1024));
    MPRINTF("Free space           : %lu KB (%lu MB)\n\r", free_space_bytes / 1024, free_space_bytes / (1024 * 1024));
    MPRINTF("Invalid space        : %lu KB (%lu MB)\n\r", stfs_info.invalid_aria_size / 1024, stfs_info.invalid_aria_size / (1024 * 1024));

    MPRINTF("Active files         : %lu\n\r", stfs_info.file_count);
    MPRINTF("Invalid files        : %lu\n\r", stfs_info.invalid_file_count);
    MPRINTF("Valid chunks         : %lu\n\r", stfs_info.valid_chunks_count);
    MPRINTF("Invalid chunks       : %lu\n\r", stfs_info.invalid_chunks_count);

    MPRINTF("Sectors count        : %lu\n\r", stfs_info.sectors_num);
    MPRINTF("Physical sector size : %lu bytes\n\r", stfs_info.phiz_sector_size);
    MPRINTF("Descriptor size      : %lu bytes\n\r", stfs_info.descriptor_size);

    // Calculate and display usage percentage based on total disk space
    if (total_space_bytes > 0)
    {
      uint32_t usage_percent = (used_space_bytes * 100) / total_space_bytes;
      uint32_t free_percent = (free_space_bytes * 100) / total_space_bytes;
      MPRINTF("Usage                : %lu%% used, %lu%% free\n\r", usage_percent, free_percent);
    }
  }
  else
  {
    MPRINTF("Error getting STfs information: %s\n\r", _Get_STfs_error_description(status));
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print current test configuration

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_test_config(void)
{
  GET_MCBL;

  MPRINTF("\n=== Test Configuration ===\n\r");
  MPRINTF("Files count      : %lu\n\r", g_fs_test_config.files_count);
  MPRINTF("File size        : %lu bytes (%.1f KB)\n\r", g_fs_test_config.file_size, (float)g_fs_test_config.file_size / 1024.0f);
  MPRINTF("Block size       : %lu bytes (%.1f KB)\n\r", g_fs_test_config.block_size, (float)g_fs_test_config.block_size / 1024.0f);
  MPRINTF("Data pattern     : %s", Test_patterns_get_name(g_fs_test_config.data_pattern));
  if (g_fs_test_config.data_pattern == DATA_PATTERN_CONSTANT)
  {
    MPRINTF(" (0x%02X)", (uint8_t)g_fs_test_config.fill_constant);
  }
  MPRINTF("\n\r");
  MPRINTF("Data verification: %s\n\r", g_fs_test_config.data_verification ? "Enabled" : "Disabled");
}

/*-----------------------------------------------------------------------------------------------------
  Description: Perform write test

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_write_test(void)
{
  GET_MCBL;
  T_performance_stats stats;
  T_sys_timestump     start_ts, end_ts;
  FX_FILE             file;
  CHAR                filename[FS_MAX_FILENAME_LENGTH];
  UINT                status;

  MPRINTF("\n=== FileX Write Test ===\n\r");
  _Print_test_config();

  // Initialize statistics (matches LittleFS format)
  Performance_stats_init(&stats);

  // Allocate test buffer
  if (!_Allocate_test_buffer())
  {
    MPRINTF("Error: Failed to allocate test buffer\n\r");
    return;
  }

  Get_hw_timestump(&start_ts);

  MPRINTF("Writing %lu files of %lu bytes each...\n\r", g_fs_test_config.files_count, g_fs_test_config.file_size);

  for (uint32_t i = 0; i < g_fs_test_config.files_count; i++)
  {
    snprintf(filename, sizeof(filename), "%s%03lu.bin", FS_TEST_FILE_PREFIX, i + 1);

    T_sys_timestump file_start_ts, file_end_ts;
    T_sys_timestump open_start_ts, open_end_ts, close_start_ts, close_end_ts;
    uint32_t        open_time = 0, close_time = 0, io_time = 0, operation_time = 0;
    uint32_t        speed_kbps = 0;

    Get_hw_timestump(&file_start_ts);

    MPRINTF("File %s: ", filename);

    // Create file
    status = fx_file_create(&g_fx_spi_nor_media, filename);
    if (status != FX_SUCCESS && status != FX_ALREADY_CREATED)
    {
      Get_hw_timestump(&file_end_ts);
      operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
      MPRINTF("FAILED (create): %s (total: %6u us)\n\r", _Get_STfs_error_description(status), operation_time);
      Performance_stats_update_error(&stats);
      continue;
    }

    // Open file with timing
    Get_hw_timestump(&open_start_ts);
    status = fx_file_open(&g_fx_spi_nor_media, &file, filename, FX_OPEN_FOR_WRITE);
    Get_hw_timestump(&open_end_ts);
    open_time = Timestump_diff_to_usec(&open_start_ts, &open_end_ts);

    if (status != FX_SUCCESS)
    {
      Get_hw_timestump(&file_end_ts);
      operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
      MPRINTF("FAILED (open): %s (open: %5u us, total: %6u us)\n\r", _Get_STfs_error_description(status), open_time, operation_time);
      Performance_stats_update_error(&stats);
      continue;
    }

    MPRINTF("opened: %5u us, ", open_time);

    // Initialize CRC calculation
    uint32_t crc                   = 0xFFFFFFFF;

    // Write file data with I/O timing
    uint32_t        bytes_to_write = g_fs_test_config.file_size > FS_CRC32_SIZE ? g_fs_test_config.file_size - FS_CRC32_SIZE : 0;
    uint32_t        total_written  = 0;
    bool            write_error    = false;
    T_sys_timestump io_start_ts, io_end_ts;

    while (total_written < bytes_to_write && !write_error)
    {
      uint32_t chunk_size = (bytes_to_write - total_written > STFS_TEST_BUFFER_SIZE) ? STFS_TEST_BUFFER_SIZE : (bytes_to_write - total_written);

      Test_patterns_fill_buffer(g_test_buffer, chunk_size, g_fs_test_config.data_pattern, g_fs_test_config.fill_constant, total_written);

      // Update CRC with this chunk
      if (g_fs_test_config.data_verification && g_fs_test_config.file_size >= FS_CRC32_SIZE)
      {
        crc = CRC32_IEEE802_3(crc, g_test_buffer, chunk_size);
      }

      Get_hw_timestump(&io_start_ts);
      status = fx_file_write(&file, g_test_buffer, chunk_size);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

      if (status == FX_SUCCESS)
      {
        total_written += chunk_size;
      }
      else
      {
        Get_hw_timestump(&file_end_ts);
        operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
        MPRINTF("FAILED (write at offset %lu): %s (I/O: %6u us, total: %6u us)\n\r", total_written, _Get_STfs_error_description(status), io_time, operation_time);
        write_error = true;
        Performance_stats_update_error(&stats);
      }
    }

    if (!write_error)
    {
      // Write CRC32 at the end of file if verification enabled
      if (g_fs_test_config.data_verification && g_fs_test_config.file_size >= FS_CRC32_SIZE)
      {
        uint32_t crc32_value = ~crc;
        Get_hw_timestump(&io_start_ts);
        status = fx_file_write(&file, &crc32_value, FS_CRC32_SIZE);
        Get_hw_timestump(&io_end_ts);
        io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

        if (status == FX_SUCCESS)
        {
          total_written += FS_CRC32_SIZE;
        }
        else
        {
          Get_hw_timestump(&file_end_ts);
          operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
          MPRINTF("FAILED (write CRC): %s (I/O: %6u us, total: %6u us)\n\r", _Get_STfs_error_description(status), io_time, operation_time);
          write_error = true;
          Performance_stats_update_error(&stats);
        }
      }
    }

    if (!write_error)
    {
      // Close file with timing
      Get_hw_timestump(&close_start_ts);
      status = fx_file_close(&file);
      Get_hw_timestump(&close_end_ts);
      close_time = Timestump_diff_to_usec(&close_start_ts, &close_end_ts);
      Get_hw_timestump(&file_end_ts);
      operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

      if (status != FX_SUCCESS)
      {
        MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r", _Get_STfs_error_description(status), close_time, operation_time);
        Performance_stats_update_error(&stats);
      }
      else
      {
        // Calculate speed in KB/s based on I/O time
        if (io_time > 0)
        {
          speed_kbps = (uint32_t)((float)g_fs_test_config.file_size * 1000000.0f / ((float)io_time * 1024.0f));
        }
        else
        {
          speed_kbps = 0;
        }

        uint32_t final_crc = ~crc;
        Performance_stats_print_write_success(close_time, io_time, operation_time, g_fs_test_config.file_size, final_crc, g_fs_test_config.data_verification);

        // Update all statistics using common function
        Performance_stats_update_success(&stats, operation_time, open_time, close_time, io_time, total_written, speed_kbps);
      }
    }
    else
    {
      // Close file even if write failed
      fx_file_close(&file);
    }
  }

  Get_hw_timestump(&end_ts);

  // Finalize statistics calculations
  Performance_stats_finalize(&stats);

  MPRINTF("\n\r");
  Performance_stats_print("Write Test", &stats, g_fs_test_config.data_verification);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Perform read test

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_read_test(void)
{
  GET_MCBL;
  T_performance_stats stats;
  T_sys_timestump     start_ts, end_ts;
  FX_FILE             file;
  CHAR                filename[FS_MAX_FILENAME_LENGTH];
  UINT                status;
  ULONG               actual_read;

  MPRINTF("\n=== FileX Read Test ===\n\r");
  _Print_test_config();

  // Initialize statistics (matches LittleFS format)
  Performance_stats_init(&stats);

  // Allocate test buffer
  if (!_Allocate_test_buffer())
  {
    MPRINTF("Error: Failed to allocate test buffer\n\r");
    return;
  }

  Get_hw_timestump(&start_ts);

  MPRINTF("Reading %lu files of %lu bytes each...\n\r", g_fs_test_config.files_count, g_fs_test_config.file_size);

  for (uint32_t i = 0; i < g_fs_test_config.files_count; i++)
  {
    snprintf(filename, sizeof(filename), "%s%03lu.bin", FS_TEST_FILE_PREFIX, i + 1);

    T_sys_timestump file_start_ts, file_end_ts;
    T_sys_timestump open_start_ts, open_end_ts, close_start_ts, close_end_ts;
    uint32_t        open_time = 0, close_time = 0, io_time = 0, operation_time = 0;
    uint32_t        speed_kbps = 0;

    Get_hw_timestump(&file_start_ts);

    MPRINTF("File %s: ", filename);

    // Open file for reading with timing
    Get_hw_timestump(&open_start_ts);
    status = fx_file_open(&g_fx_spi_nor_media, &file, filename, FX_OPEN_FOR_READ);
    Get_hw_timestump(&open_end_ts);
    open_time = Timestump_diff_to_usec(&open_start_ts, &open_end_ts);

    if (status != FX_SUCCESS)
    {
      Get_hw_timestump(&file_end_ts);
      operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
      MPRINTF("FAILED (open): %s (open: %5u us, total: %6u us)\n\r",
              _Get_STfs_error_description(status), open_time, operation_time);
      Performance_stats_update_error(&stats);
      continue;
    }

    MPRINTF("opened: %5u us, ", open_time);

    // Initialize CRC calculation and verification flags
    uint32_t crc                  = 0xFFFFFFFF;
    bool     crc_valid            = true;
    bool     pattern_valid        = true;
    bool     size_valid           = true;

    // Read file data with I/O timing
    uint32_t        bytes_to_read = g_fs_test_config.file_size > FS_CRC32_SIZE ? g_fs_test_config.file_size - FS_CRC32_SIZE : g_fs_test_config.file_size;
    uint32_t        total_read    = 0;
    bool            read_error    = false;
    bool            verify_error  = false;
    T_sys_timestump io_start_ts, io_end_ts;

    while (total_read < bytes_to_read && !read_error)
    {
      uint32_t chunk_size = (bytes_to_read - total_read > STFS_TEST_BUFFER_SIZE) ? STFS_TEST_BUFFER_SIZE : (bytes_to_read - total_read);

      Get_hw_timestump(&io_start_ts);
      status = fx_file_read(&file, g_test_buffer, chunk_size, &actual_read);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

      if (status == FX_SUCCESS && actual_read == chunk_size)
      {
        // Verify data if enabled (use offset before incrementing total_read)
        if (g_fs_test_config.data_verification && !Test_patterns_verify_buffer(g_test_buffer, actual_read, g_fs_test_config.data_pattern, g_fs_test_config.fill_constant, total_read))
        {
          Get_hw_timestump(&file_end_ts);
          operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
          MPRINTF("FAILED (verify at offset %lu): Data verification failed (I/O: %6u us, total: %6u us)\n\r", total_read, io_time, operation_time);
          pattern_valid = false;
          verify_error = true;
          Performance_stats_increment_pattern_error(&stats);
          break;
        }

        total_read += actual_read;

        // Update CRC with this chunk if verification enabled
        if (g_fs_test_config.data_verification && g_fs_test_config.file_size >= FS_CRC32_SIZE)
        {
          crc = CRC32_IEEE802_3(crc, g_test_buffer, actual_read);
        }
      }
      else
      {
        Get_hw_timestump(&file_end_ts);
        operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
        MPRINTF("FAILED (read at offset %lu): %s (read %lu, expected %lu, I/O: %6u us, total: %6u us)\n\r", total_read, _Get_STfs_error_description(status), actual_read, chunk_size, io_time, operation_time);
        read_error = true;
        Performance_stats_update_error(&stats);
        break;
      }
    }

    // Read and verify CRC32 if enabled
    if (!read_error && !verify_error && g_fs_test_config.data_verification && g_fs_test_config.file_size >= FS_CRC32_SIZE)
    {
      uint32_t file_crc32, calculated_crc32;
      Get_hw_timestump(&io_start_ts);
      status = fx_file_read(&file, &file_crc32, FS_CRC32_SIZE, &actual_read);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

      if (status == FX_SUCCESS && actual_read == FS_CRC32_SIZE)
      {
        calculated_crc32 = ~crc;
        if (file_crc32 != calculated_crc32)
        {
          crc_valid = false;
          Performance_stats_increment_crc_error(&stats);
        }
        total_read += actual_read;
      }
      else
      {
        crc_valid = false;
        Performance_stats_increment_crc_error(&stats);
      }
    }

    // Close file with timing
    Get_hw_timestump(&close_start_ts);
    status = fx_file_close(&file);
    Get_hw_timestump(&close_end_ts);
    close_time = Timestump_diff_to_usec(&close_start_ts, &close_end_ts);
    Get_hw_timestump(&file_end_ts);
    operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

    if (status != FX_SUCCESS)
    {
      MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r", _Get_STfs_error_description(status), close_time, operation_time);
      Performance_stats_update_error(&stats);
    }
    else if (!read_error && !verify_error)
    {
      // Check file size
      if (total_read != g_fs_test_config.file_size)
      {
        size_valid = false;
        Performance_stats_increment_size_error(&stats);
      }

      // Calculate speed in KB/s based on I/O time
      if (io_time > 0)
      {
        speed_kbps = (uint32_t)((float)total_read * 1000000.0f / ((float)io_time * 1024.0f));
      }
      else
      {
        speed_kbps = 0;
      }

      uint32_t calculated_crc = ~crc;
      Performance_stats_print_read_success(close_time, io_time, operation_time, total_read, g_fs_test_config.file_size,
                                           calculated_crc, crc_valid, pattern_valid, size_valid, g_fs_test_config.data_verification);

      // Update all statistics using common function
      Performance_stats_update_success(&stats, operation_time, open_time, close_time, io_time, total_read, speed_kbps);
    }
  }

  Get_hw_timestump(&end_ts);

  // Finalize statistics calculations
  Performance_stats_finalize(&stats);

  MPRINTF("\n\r");
  Performance_stats_print("Read Test", &stats, g_fs_test_config.data_verification);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Perform delete test

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_delete_test(void)
{
  GET_MCBL;
  T_performance_stats stats;
  T_sys_timestump     start_ts, end_ts;
  CHAR                filename[FS_MAX_FILENAME_LENGTH];
  UINT                status;

  MPRINTF("\n=== FileX Delete Test ===\n\r");

  // Initialize statistics for delete operations (limited fields used)
  Performance_stats_init_delete(&stats);

  Get_hw_timestump(&start_ts);

  MPRINTF("Deleting %lu test files...\n\r", g_fs_test_config.files_count);

  for (uint32_t i = 0; i < g_fs_test_config.files_count; i++)
  {
    snprintf(filename, sizeof(filename), "%s%03lu.bin", FS_TEST_FILE_PREFIX, i + 1);

    T_sys_timestump file_start_ts, file_end_ts;
    uint32_t        operation_time = 0;
    uint32_t        speed_kbps;

    Get_hw_timestump(&file_start_ts);

    MPRINTF("File %s: ", filename);

    status = fx_file_delete(&g_fx_spi_nor_media, filename);
    Get_hw_timestump(&file_end_ts);
    operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

    if (status == FX_SUCCESS)
    {
      // Calculate speed in KB/s based on operation time (avoid division by zero)
      // Using binary KB (1 KB = 1024 bytes) for speed calculation with floating point precision
      if (operation_time > 0)
      {
        speed_kbps = (uint32_t)((float)g_fs_test_config.file_size * 1000000.0f / ((float)operation_time * 1024.0f));
      }
      else
      {
        speed_kbps = 0;
      }

      MPRINTF("deleted: %6u us, speed: %5u KB/s\n\r", operation_time, speed_kbps);
      Performance_stats_update_delete_success(&stats, operation_time, g_fs_test_config.file_size);
    }
    else
    {
      MPRINTF("FAILED: %s (%6u us)\n\r", _Get_STfs_error_description(status), operation_time);
      Performance_stats_update_error(&stats);
    }
  }

  Get_hw_timestump(&end_ts);

  // Finalize statistics calculations
  Performance_stats_finalize(&stats);

  MPRINTF("\n\r");
  Performance_stats_print("Delete Test", &stats, g_fs_test_config.data_verification);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Perform STfs format test

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_format_test(void)
{
  GET_MCBL;
  T_sys_timestump start_ts, end_ts;
  int32_t         status;

  MPRINTF("\n=== STfs Format Test ===\n\r");
  MPRINTF("WARNING: This will erase all data on the STfs!\n\r");
  MPRINTF("Press 'Y' to confirm or any other key to cancel: ");

  uint8_t confirm;
  if (WAIT_CHAR(&confirm, ms_to_ticks(30000)) != RES_OK || (confirm != 'Y' && confirm != 'y'))
  {
    MPRINTF("\nFormat cancelled.\n\r");
    return;
  }

  MPRINTF("\nFormatting STfs...\n\r");

  Get_hw_timestump(&start_ts);

  // Format the STfs file system
  status = STfs_format(0); // Using drive_id = 0

  Get_hw_timestump(&end_ts);
  uint32_t format_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (status == STFS_OK)
  {
    MPRINTF("STfs format completed successfully in %lu us (%.2f ms)\n\r",
            format_time, (float)format_time / 1000.0f);
    _Print_STfs_info();
  }
  else
  {
    MPRINTF("STfs format failed with error: %s\n\r", _Get_STfs_error_description(status));
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Perform full test (write + read + delete)

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_full_test(void)
{
  GET_MCBL;
  T_sys_timestump total_start_ts, total_end_ts;
  Get_hw_timestump(&total_start_ts);

  // Step 1: Write test
  MPRINTF("\n[1/3] Write Test\n\r");
  _Do_write_test();

  MPRINTF("\nPress any key to continue to read test...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));

  // Step 2: Read test
  MPRINTF("\n[2/3] Read Test\n\r");
  _Do_read_test();

  MPRINTF("\nPress any key to continue to delete test...\n\r");
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));

  // Step 3: Delete test
  MPRINTF("\n[3/3] Delete Test\n\r");
  _Do_delete_test();

  Get_hw_timestump(&total_end_ts);
  uint32_t total_time = Timestump_diff_to_usec(&total_start_ts, &total_end_ts);

  MPRINTF("\n=== Full Test Summary ===\n\r");
  MPRINTF("Total test time: %lu us\n\r", total_time);
  MPRINTF("Files processed: %lu\n\r", g_fs_test_config.files_count);
  MPRINTF("Data per file: %lu bytes\n\r", g_fs_test_config.file_size);
  MPRINTF("Total data: %lu KB\n\r", (g_fs_test_config.files_count * g_fs_test_config.file_size) / 1024);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize STfs file system

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_STfs_init(uint8_t keycode)
{
  GET_MCBL;
  int32_t         status;
  T_sys_timestump start_ts, end_ts;
  T_stfs_info     stfs_info;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== STfs Initialization ===\n\r");

  Get_hw_timestump(&start_ts);

  // Initialize STfs file system
  status = STfs_init(0, &stfs_info); // Using drive_id = 0

  Get_hw_timestump(&end_ts);
  uint32_t init_time = Timestump_diff_to_usec(&start_ts, &end_ts) / 1000; // Convert to ms

  if (status == STFS_OK)
  {
    MPRINTF("STfs initialized successfully in %lu ms\n\r", init_time);
    _Print_STfs_info();
  }
  else
  {
    MPRINTF("STfs initialization failed: %s\n\r", _Get_STfs_error_description(status));
    MPRINTF("Attempting to format STfs...\n\r");

    Get_hw_timestump(&start_ts);

    // Try to format the STfs
    int32_t format_status = STfs_format(0); // Using drive_id = 0

    Get_hw_timestump(&end_ts);
    uint32_t format_time = Timestump_diff_to_usec(&start_ts, &end_ts) / 1000; // Convert to ms

    if (format_status == STFS_OK)
    {
      MPRINTF("STfs format completed successfully in %lu ms\n\r", format_time);

      // Try to initialize again after format
      Get_hw_timestump(&start_ts);
      status = STfs_init(0, &stfs_info);
      Get_hw_timestump(&end_ts);
      init_time = Timestump_diff_to_usec(&start_ts, &end_ts) / 1000;

      if (status == STFS_OK)
      {
        MPRINTF("STfs initialized successfully after format in %lu ms\n\r", init_time);
        _Print_STfs_info();
      }
      else
      {
        MPRINTF("STfs initialization failed after format: %s\n\r", _Get_STfs_error_description(status));
      }
    }
    else
    {
      MPRINTF("STfs format failed: %s\n\r", _Get_STfs_error_description(format_status));
    }
  }

  MPRINTF("\nPress any key to continue...\n\r");
  uint8_t key;
  WAIT_CHAR(&key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: List files

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_STfs_list_files(uint8_t keycode)
{
  GET_MCBL;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== FileX File Listing ===\n\r");

  // Check if media is open
  if (g_fx_spi_nor_media.fx_media_id != FX_MEDIA_ID)
  {
    MPRINTF("Error: FileX media not initialized. Please initialize first.\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  _Print_STfs_info();

  MPRINTF("\n=== File List ===\n\r");

  // Interactive menu for file operations
  uint8_t choice;
  while (true)
  {
    // Show menu at the top
    MPRINTF("\n\rOptions:\n\r");
    MPRINTF("<1> - Read file as HEX dump\n\r");
    MPRINTF("<ESC> - Return to menu\n\r");
    MPRINTF("Choice: ");

    if (WAIT_CHAR(&choice, ms_to_ticks(30000)) == RES_OK)
    {
      if (choice == '1')
      {
        // Get filename from user
        char filename[FS_MAX_FILENAME_LENGTH];
        if (VT100_input_filename(filename, FS_MAX_FILENAME_LENGTH, "test_001.bin"))
        {
          // Add leading slash if not present
          char full_filename[FS_MAX_FILENAME_LENGTH + 1];
          if (filename[0] == '/')
          {
            // Already has absolute path
            strncpy(full_filename, filename, sizeof(full_filename) - 1);
            full_filename[sizeof(full_filename) - 1] = '\0';
          }
          else
          {
            // Add leading slash for root directory
            snprintf(full_filename, sizeof(full_filename), "/%s", filename);
          }

          MPRINTF("Opening file: %s\n\r", full_filename);

          // Open file for reading
          FX_FILE file;
          UINT    status = fx_file_open(&g_fx_spi_nor_media, &file, full_filename, FX_OPEN_FOR_READ);
          if (status != FX_SUCCESS)
          {
            MPRINTF("Failed to open file: %s\n\r", _Get_STfs_error_description(status));
          }
          else
          {
            // Get file size
            ULONG file_size;
            status = fx_file_extended_seek(&file, 0);
            if (status == FX_SUCCESS)
            {
              file_size = file.fx_file_current_file_size;
              MPRINTF("File size: %lu bytes\n\r", file_size);

              if (file_size == 0)
              {
                MPRINTF("File is empty\n\r");
              }
              else
              {
                // Allocate test buffer for reading
                if (!_Allocate_test_buffer())
                {
                  MPRINTF("Memory allocation failed\n\r");
                }
                else
                {
                  // Read file in blocks using STFS_TEST_BUFFER_SIZE
                  uint32_t block_size       = STFS_TEST_BUFFER_SIZE;
                  uint32_t total_bytes_read = 0;
                  uint32_t current_offset   = 0;

                  // Read file block by block
                  while (current_offset < file_size)
                  {
                    // Calculate bytes to read for this block
                    uint32_t bytes_to_read = block_size;
                    if (current_offset + bytes_to_read > file_size)
                    {
                      bytes_to_read = file_size - current_offset;
                    }

                    // Seek to current position
                    status = fx_file_extended_seek(&file, current_offset);
                    if (status != FX_SUCCESS)
                    {
                      MPRINTF("Failed to seek to offset %lu: %s\n\r",
                              current_offset, _Get_STfs_error_description(status));
                      break;
                    }

                    // Read one block
                    ULONG actual_bytes_read;
                    status = fx_file_read(&file, g_test_buffer, bytes_to_read, &actual_bytes_read);
                    if (status != FX_SUCCESS)
                    {
                      MPRINTF("Failed to read file at offset %lu: %s\n\r",
                              current_offset, _Get_STfs_error_description(status));
                      break;
                    }

                    if (actual_bytes_read == 0)
                    {
                      // End of file reached
                      break;
                    }

                    // Display this block as HEX dump
                    MPRINTF("Block at offset %lu (%lu bytes):\n\r", current_offset, actual_bytes_read);
                    VT100_print_dump(current_offset, g_test_buffer, actual_bytes_read);

                    current_offset += actual_bytes_read;
                    total_bytes_read += actual_bytes_read;

                    // Show progress for large files
                    if (file_size > block_size)
                    {
                      uint32_t progress_percent = (current_offset * 100) / file_size;
                      MPRINTF("Progress: %lu%% (%lu/%lu bytes)\n\r",
                              progress_percent, current_offset, file_size);
                    }
                  }

                  MPRINTF("Successfully read %lu bytes total\n\r", total_bytes_read);
                }
              }
            }
            else
            {
              MPRINTF("Failed to get file size: %s\n\r", _Get_STfs_error_description(status));
            }

            // Close file
            fx_file_close(&file);
          }
        }
      }
      else if (choice == VT100_ESC)
      {
        // Exit to menu
        break;
      }
      else
      {
        // Invalid choice, continue waiting
        MPRINTF("Invalid choice. Press 1 to read file or ESC to return to menu.\n\r");
        MPRINTF("Choice: ");
      }
    }
    else
    {
      MPRINTF("TIMEOUT - returning to menu\n\r");
      break;
    }
  }

  MPRINTF("\nPress any key to continue...\n\r");
  uint8_t key;
  WAIT_CHAR(&key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Performance test menu

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_STfs_performance_test(uint8_t keycode)
{
  GET_MCBL;
  uint8_t choice;
  bool    exit_menu = false;

  FSP_PARAMETER_NOT_USED(keycode);

  // Initialize filesystem test configuration
  Fs_test_config_init();

  while (!exit_menu)
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF("=== FileX Test Operations ===\n\r");

    // Check if media is open
    if (g_fx_spi_nor_media.fx_media_id != FX_MEDIA_ID)
    {
      MPRINTF("\nERROR: FileX media not initialized!\n\r");
      MPRINTF("Please initialize FileX first from the main menu.\n\r");
      MPRINTF("\nPress any key to return...\n\r");
      WAIT_CHAR(&choice, ms_to_ticks(100000));
      return;
    }

    _Print_STfs_info();
    _Print_test_config();

    MPRINTF("\n\rTest operations:\n\r");
    MPRINTF("<1> - Write files test\n\r");
    MPRINTF("<2> - Read files test\n\r");
    MPRINTF("<3> - Delete files test\n\r");
    MPRINTF("<4> - Format filesystem test\n\r");
    MPRINTF("<5> - Run full test (write+read+delete)\n\r");
    MPRINTF("\n\rConfiguration:\n\r");
    MPRINTF("<6> - Change files count (1-10000)\n\r");
    MPRINTF("<7> - Change file size (1KB-1MB)\n\r");
    MPRINTF("<8> - Change block size (1KB-128KB)\n\r");
    MPRINTF("<A> - Change data pattern\n\r");
    MPRINTF("<B> - Toggle data verification\n\r");
    MPRINTF("<C> - Change constant pattern value\n\r");
    MPRINTF("<9> - Reset to defaults\n\r");
    MPRINTF("<ESC> - Return to previous menu\n\r");
    MPRINTF("\n\rEnter choice: ");

    if (WAIT_CHAR(&choice, ms_to_ticks(100000)) == RES_OK)
    {
      switch (choice)
      {
        case '1':
          _Do_write_test();
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '2':
          _Do_read_test();
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '3':
          _Do_delete_test();
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '4':
          _Do_format_test();
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '5':
          _Do_full_test();
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '6':
          MPRINTF("\n\rEnter new files count (1-10000) [current: %lu]: ", g_fs_test_config.files_count);
          uint32_t new_files_count;
          if (VT100_input_uint32(&new_files_count, 1, 10000, g_fs_test_config.files_count))
          {
            g_fs_test_config.files_count = new_files_count;
            MPRINTF("Files count changed to %lu\n\r", g_fs_test_config.files_count);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '7':
          MPRINTF("\n\rEnter new file size in KB (1-1024) [current: %.1f]: ", (float)g_fs_test_config.file_size / 1024.0f);
          uint32_t new_file_size_kb;
          if (VT100_input_uint32(&new_file_size_kb, 1, 1024, g_fs_test_config.file_size / 1024))
          {
            g_fs_test_config.file_size = new_file_size_kb * 1024;
            MPRINTF("File size changed to %lu bytes (%.1f KB)\n\r", g_fs_test_config.file_size, (float)g_fs_test_config.file_size / 1024.0f);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '8':
          MPRINTF("\n\rEnter new block size in KB (1-128) [current: %.1f]: ", (float)g_fs_test_config.block_size / 1024.0f);
          uint32_t new_block_size_kb;
          if (VT100_input_uint32(&new_block_size_kb, 1, 128, g_fs_test_config.block_size / 1024))
          {
            g_fs_test_config.block_size = new_block_size_kb * 1024;
            MPRINTF("Block size changed to %lu bytes (%.1f KB)\n\r", g_fs_test_config.block_size, (float)g_fs_test_config.block_size / 1024.0f);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case 'A':
        case 'a':
          MPRINTF("\n\rSelect data pattern:\n\r");
          MPRINTF("0 - Constant pattern\n\r");
          MPRINTF("1 - Incremental pattern\n\r");
          MPRINTF("2 - Random pattern\n\r");
          MPRINTF("3 - Checksum pattern\n\r");
          MPRINTF("Current pattern: %lu\n\r", g_fs_test_config.data_pattern);
          MPRINTF("Enter choice: ");
          uint32_t new_pattern;
          if (VT100_input_uint32(&new_pattern, 0, 3, g_fs_test_config.data_pattern))
          {
            g_fs_test_config.data_pattern = new_pattern;
            MPRINTF("Data pattern changed to %lu\n\r", g_fs_test_config.data_pattern);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case 'B':
        case 'b':
          g_fs_test_config.data_verification = !g_fs_test_config.data_verification;
          MPRINTF("\n\rData verification %s\n\r", g_fs_test_config.data_verification ? "enabled" : "disabled");
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case 'C':
        case 'c':
          MPRINTF("\n\rEnter new constant value (0-255) [current: 0x%02X]: ", (uint8_t)g_fs_test_config.fill_constant);
          uint32_t new_constant;
          if (VT100_input_uint32(&new_constant, 0, 255, (uint8_t)g_fs_test_config.fill_constant))
          {
            g_fs_test_config.fill_constant = (uint8_t)new_constant;
            MPRINTF("Constant value changed to 0x%02X\n\r", (uint8_t)g_fs_test_config.fill_constant);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '9':
          Fs_test_config_reset_to_defaults();
          MPRINTF("\n\rConfiguration reset to defaults.\n\r");
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case VT100_ESC:
          exit_menu = true;
          break;

        default:
          MPRINTF("\n\rInvalid choice. Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;
      }
    }
  }

  // Free test buffer when exiting menu
  _Free_test_buffer();
}

/*-----------------------------------------------------------------------------------------------------
  Description: Defragment STfs file system

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_STfs_defrag(uint8_t keycode)
{
  GET_MCBL;
  T_sys_timestump start_ts, end_ts;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== STfs Defragmentation ===\n\r");

  // Check if STfs is initialized
  // TODO: Replace with STfs initialization check when STfs API is implemented
  MPRINTF("WARNING: This will defragment the STfs file system!\n\r");
  MPRINTF("This may take some time and will reorganize file storage.\n\r");
  MPRINTF("Press 'Y' to confirm or any other key to cancel: ");

  uint8_t confirm;
  if (WAIT_CHAR(&confirm, ms_to_ticks(30000)) != RES_OK || (confirm != 'Y' && confirm != 'y'))
  {
    MPRINTF("\nDefragmentation cancelled.\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("\nStarting STfs defragmentation...\n\r");

  Get_hw_timestump(&start_ts);

  // TODO: Replace with actual STfs_defrag() call when STfs API is implemented
  // uint32_t status = STfs_defrag();
  uint32_t status = 0; // Placeholder - assume success for now

  Get_hw_timestump(&end_ts);
  uint32_t defrag_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (status == 0) // TODO: Replace with STfs success constant
  {
    MPRINTF("Defragmentation completed successfully in %lu us (%.2f ms)\n\r",
            defrag_time, (float)defrag_time / 1000.0f);

    // TODO: Add STfs info display when STfs API is implemented
    // _Print_STfs_info();
  }
  else
  {
    MPRINTF("Defragmentation failed with error: %lu\n\r", status);
  }

  MPRINTF("\nPress any key to continue...\n\r");
  uint8_t key;
  WAIT_CHAR(&key, ms_to_ticks(100000));
}
