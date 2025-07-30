#include "App.h"
#include "LevelX_config.h"
#include "Performance_Stats.h"
#include "Test_Patterns.h"
#include "FS_Test_Config.h"

#define MAX_PATH_LENGTH          256
#define MAX_DIR_STACK_DEPTH      32
#define FILEX_MEMORY_BUFFER_SIZE (32 * 1024)  // 32KB
#define FILEX_TEST_BUFFER_SIZE   (16 * 1024)  // 16KB for test operations

// Directory navigation stack
typedef struct
{
  char    path[MAX_PATH_LENGTH];
  uint8_t depth;
} T_dir_entry;

static T_dir_entry g_dir_stack[MAX_DIR_STACK_DEPTH];
static uint32_t    g_stack_top = 0;



// Global pointer to YFFS2 memory buffer
static uint8_t *g_YFFS2_memory_buffer      = NULL;

// Global pointer to test buffer for file operations
static uint8_t *g_test_buffer              = NULL;

// Menu definition
const T_VT100_Menu_item MENU_YFFS2_items[] = {
  { '1', Do_YFFS2_init, NULL },
  { '2', Do_YFFS2_list_files, NULL },
  { '3', Do_YFFS2_performance_test, NULL },
  { 'R', NULL, NULL },
  { 0 }  // End of menu
};

const T_VT100_Menu MENU_YFFS2 = {
  "YFFS2 with LevelX Manager",
  "\033[5C YFFS2 file system with LevelX wear leveling management menu\r\n"
  "\033[5C <1> - Initialize YFFS2 with LevelX (auto-format if needed)\r\n"
  "\033[5C <2> - List files and directories\r\n"
  "\033[5C <3> - Performance test\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_YFFS2_items
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
static void        _Print_YFFS2_info(void);
static void        _Print_test_config(void);
static bool        _Push_dir_to_stack(const char *path, uint8_t depth);
static bool        _Pop_dir_from_stack(char *path, uint8_t *depth);
static void        _Print_tree_indent(uint8_t depth);
static void        _List_directory_tree(const char *root_path, uint8_t max_depth);
static const char *_Get_YFFS2_error_description(UINT status);
static bool        _Allocate_test_buffer(void);
static void        _Free_test_buffer(void);

/*-----------------------------------------------------------------------------------------------------
  Description: Helper function to push directory to stack

  Parameters: path - directory path, depth - depth level

  Return: true if pushed successfully, false if stack full
-----------------------------------------------------------------------------------------------------*/
static bool _Push_dir_to_stack(const char *path, uint8_t depth)
{
  if (g_stack_top >= MAX_DIR_STACK_DEPTH)
  {
    return false;  // Stack full
  }

  strncpy(g_dir_stack[g_stack_top].path, path, MAX_PATH_LENGTH - 1);
  g_dir_stack[g_stack_top].path[MAX_PATH_LENGTH - 1] = '\0';
  g_dir_stack[g_stack_top].depth                     = depth;
  g_stack_top++;
  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Helper function to pop directory from stack

  Parameters: path - buffer for path, depth - pointer to depth variable

  Return: true if popped successfully, false if stack empty
-----------------------------------------------------------------------------------------------------*/
static bool _Pop_dir_from_stack(char *path, uint8_t *depth)
{
  if (g_stack_top == 0)
  {
    return false;  // Stack empty
  }

  g_stack_top--;
  strcpy(path, g_dir_stack[g_stack_top].path);
  *depth = g_dir_stack[g_stack_top].depth;
  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get YFFS2 error description string

  Parameters: status - YFFS2 error status code

  Return: pointer to error description string
-----------------------------------------------------------------------------------------------------*/
static const char *_Get_YFFS2_error_description(UINT status)
{
  switch (status)
  {
    case FX_SUCCESS:
      return "FX_SUCCESS";
    case FX_BOOT_ERROR:
      return "FX_BOOT_ERROR";
    case FX_MEDIA_NOT_OPEN:
      return "FX_MEDIA_NOT_OPEN";
    case FX_NOT_FOUND:
      return "FX_NOT_FOUND";
    case FX_NOT_A_FILE:
      return "FX_NOT_A_FILE";
    case FX_ACCESS_ERROR:
      return "FX_ACCESS_ERROR";
    case FX_FILE_CORRUPT:
      return "FX_FILE_CORRUPT";
    case FX_INVALID_PATH:
      return "FX_INVALID_PATH";
    case FX_ALREADY_CREATED:
      return "FX_ALREADY_CREATED";
    case FX_INVALID_NAME:
      return "FX_INVALID_NAME";
    case FX_MEDIA_INVALID:
      return "FX_MEDIA_INVALID";
    case FX_IO_ERROR:
      return "FX_IO_ERROR";
    case FX_WRITE_PROTECT:
      return "FX_WRITE_PROTECT";
    case FX_PTR_ERROR:
      return "FX_PTR_ERROR";
    case FX_CALLER_ERROR:
      return "FX_CALLER_ERROR";
    case FX_INVALID_OPTION:
      return "FX_INVALID_OPTION";
    case FX_SECTOR_INVALID:
      return "FX_SECTOR_INVALID";
    case FX_NO_MORE_SPACE:
      return "FX_NO_MORE_SPACE";
    case FX_NO_MORE_ENTRIES:
      return "FX_NO_MORE_ENTRIES";
    case FX_NOT_DIRECTORY:
      return "FX_NOT_DIRECTORY";
    case FX_END_OF_FILE:
      return "FX_END_OF_FILE";
    case FX_NOT_IMPLEMENTED:
      return "FX_NOT_IMPLEMENTED";
    case FX_READ_CONTINUE:
      return "FX_READ_CONTINUE";
    case FX_BUFFER_ERROR:
      return "FX_BUFFER_ERROR";
    default:
      return "UNKNOWN_ERROR";
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
    g_test_buffer = App_malloc(FILEX_TEST_BUFFER_SIZE);
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
  Description: Helper function to print tree indentation

  Parameters: depth - depth level

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_tree_indent(uint8_t depth)
{
  GET_MCBL;

  for (uint8_t i = 0; i < depth; i++)
  {
    if (i == depth - 1)
    {
      MPRINTF("+-- ");
    }
    else
    {
      MPRINTF("|   ");
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print YFFS2 media information

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_YFFS2_info(void)
{
  GET_MCBL;
  ULONG total_clusters, available_bytes;
  ULONG sectors_per_cluster, bytes_per_sector;

  MPRINTF("\n=== YFFS2 Media Information ===\n\r");

  // Get media information - fx_media_space_available returns available bytes, not clusters
  UINT status = fx_media_space_available(&g_fx_spi_nor_media, &available_bytes);
  if (status == FX_SUCCESS)
  {
    total_clusters             = g_fx_spi_nor_media.fx_media_total_clusters;
    sectors_per_cluster        = g_fx_spi_nor_media.fx_media_sectors_per_cluster;
    bytes_per_sector           = g_fx_spi_nor_media.fx_media_bytes_per_sector;

    // Calculate sizes in bytes first to avoid overflow
    ULONG cluster_size_bytes   = sectors_per_cluster * bytes_per_sector;

    // Use data cluster count from media structure (this is the actual user data area)
    ULONG data_clusters        = g_fx_spi_nor_media.fx_media_available_clusters;

    // Calculate sizes based on data clusters from media structure
    ULONG total_size_bytes     = total_clusters * cluster_size_bytes;
    ULONG data_size_bytes      = data_clusters * cluster_size_bytes;
    ULONG used_size_bytes      = total_size_bytes - available_bytes;

    MPRINTF("Media ID             : 0x%lX\n\r", g_fx_spi_nor_media.fx_media_id);
    MPRINTF("Total clusters       : %lu\n\r", total_clusters);
    MPRINTF("Data clusters        : %lu\n\r", data_clusters);
    MPRINTF("Sectors per cluster  : %lu\n\r", sectors_per_cluster);
    MPRINTF("Bytes per sector     : %lu\n\r", bytes_per_sector);
    MPRINTF("Cluster size         : %lu bytes\n\r", cluster_size_bytes);
    MPRINTF("Total space          : %lu KB (%lu MB)\n\r", total_size_bytes / 1024, total_size_bytes / (1024 * 1024));
    MPRINTF("Data space           : %lu KB (%lu MB)\n\r", data_size_bytes / 1024, data_size_bytes / (1024 * 1024));
    MPRINTF("Available space      : %lu KB (%lu MB)\n\r", available_bytes / 1024, available_bytes / (1024 * 1024));
    MPRINTF("Used space           : %lu KB (%lu MB)\n\r", used_size_bytes / 1024, used_size_bytes / (1024 * 1024));

    // Calculate and display usage percentage based on total disk space
    if (total_size_bytes > 0)
    {
      ULONG usage_percent = (used_size_bytes * 100) / total_size_bytes;
      MPRINTF("Usage                : %lu%% used, %lu%% free\n\r", usage_percent, 100 - usage_percent);
    }
  }
  else
  {
    MPRINTF("Error getting media information: %s\n\r", _Get_YFFS2_error_description(status));
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
  Description: List YFFS2 directory tree (non-recursive with stack)

  Parameters: root_path - starting directory path, max_depth - maximum depth to traverse

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _List_directory_tree(const char *root_path, uint8_t max_depth)
{
  GET_MCBL;
  CHAR    entry_name[FX_MAX_LONG_NAME_LEN];
  UINT    attributes;
  ULONG   size;
  UINT    year, month, day, hour, minute, second;
  UINT    status;
  char    current_path[MAX_PATH_LENGTH];
  uint8_t current_depth;

  // Initialize stack and start with root directory
  g_stack_top = 0;
  _Push_dir_to_stack(root_path, 0);

  // Process directories using stack (non-recursive)
  while (_Pop_dir_from_stack(current_path, &current_depth))
  {
    // Skip if we've reached maximum depth
    if (current_depth >= max_depth)
    {
      continue;
    }

    // Set current directory
    status = fx_directory_default_set(&g_fx_spi_nor_media, current_path);
    if (status != FX_SUCCESS)
    {
      MPRINTF("Failed to set directory %s: %s\n\r", current_path, _Get_YFFS2_error_description(status));
      continue;
    }

    // Get first directory entry
    status = fx_directory_first_full_entry_find(&g_fx_spi_nor_media, entry_name, &attributes, &size,
                                                &year, &month, &day, &hour, &minute, &second);

    while (status == FX_SUCCESS)
    {
      // Skip "." and ".." entries
      if (strcmp(entry_name, ".") != 0 && strcmp(entry_name, "..") != 0)
      {
        _Print_tree_indent(current_depth);

        if (attributes & FX_DIRECTORY)
        {
          MPRINTF("[DIR]  %s/\n\r", entry_name);

          // Add subdirectory to stack if not too deep and stack not full
          if (current_depth < max_depth - 1 && g_stack_top < MAX_DIR_STACK_DEPTH - 1)
          {
            char subdir_path[MAX_PATH_LENGTH];
            if (strcmp(current_path, "/") == 0)
            {
              snprintf(subdir_path, MAX_PATH_LENGTH, "/%s", entry_name);
            }
            else
            {
              snprintf(subdir_path, MAX_PATH_LENGTH, "%s/%s", current_path, entry_name);
            }
            _Push_dir_to_stack(subdir_path, current_depth + 1);
          }
        }
        else
        {
          MPRINTF("[FILE] %s (%lu bytes) %02u/%02u/%04u %02u:%02u:%02u\n\r",
                  entry_name, size, month, day, year, hour, minute, second);
        }
      }

      // Get next directory entry
      status = fx_directory_next_full_entry_find(&g_fx_spi_nor_media, entry_name, &attributes, &size,
                                                 &year, &month, &day, &hour, &minute, &second);
    }
  }
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

  MPRINTF("\n=== YFFS2 Write Test ===\n\r");
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

  // Ensure we're in root directory
  status = fx_directory_default_set(&g_fx_spi_nor_media, "/");
  if (status != FX_SUCCESS)
  {
    MPRINTF("Error changing to root directory: %s\n\r", _Get_YFFS2_error_description(status));
    return;
  }

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
      MPRINTF("FAILED (create): %s (total: %6u us)\n\r", _Get_YFFS2_error_description(status), operation_time);
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
      MPRINTF("FAILED (open): %s (open: %5u us, total: %6u us)\n\r", _Get_YFFS2_error_description(status), open_time, operation_time);
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
      uint32_t chunk_size = (bytes_to_write - total_written > FILEX_TEST_BUFFER_SIZE) ? FILEX_TEST_BUFFER_SIZE : (bytes_to_write - total_written);

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
        MPRINTF("FAILED (write at offset %lu): %s (I/O: %6u us, total: %6u us)\n\r", total_written, _Get_YFFS2_error_description(status), io_time, operation_time);
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
          MPRINTF("FAILED (write CRC): %s (I/O: %6u us, total: %6u us)\n\r", _Get_YFFS2_error_description(status), io_time, operation_time);
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
        MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r", _Get_YFFS2_error_description(status), close_time, operation_time);
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

  // Return to root directory
  fx_directory_default_set(&g_fx_spi_nor_media, "/");

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

  MPRINTF("\n=== YFFS2 Read Test ===\n\r");
  _Print_test_config();

  // Initialize statistics (matches LittleFS format)
  Performance_stats_init(&stats);

  // Allocate test buffer
  if (!_Allocate_test_buffer())
  {
    MPRINTF("Error: Failed to allocate test buffer\n\r");
    return;
  }

  // Change to root directory
  status = fx_directory_default_set(&g_fx_spi_nor_media, "/");
  if (status != FX_SUCCESS)
  {
    MPRINTF("Error: Could not access root directory. Run write test first.\n\r");
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
              _Get_YFFS2_error_description(status), open_time, operation_time);
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
      uint32_t chunk_size = (bytes_to_read - total_read > FILEX_TEST_BUFFER_SIZE) ? FILEX_TEST_BUFFER_SIZE : (bytes_to_read - total_read);

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
        MPRINTF("FAILED (read at offset %lu): %s (read %lu, expected %lu, I/O: %6u us, total: %6u us)\n\r", total_read, _Get_YFFS2_error_description(status), actual_read, chunk_size, io_time, operation_time);
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
      MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r", _Get_YFFS2_error_description(status), close_time, operation_time);
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

  // Return to root directory
  fx_directory_default_set(&g_fx_spi_nor_media, "/");

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

  MPRINTF("\n=== YFFS2 Delete Test ===\n\r");

  // Initialize statistics for delete operations (limited fields used)
  Performance_stats_init_delete(&stats);

  // Change to root directory
  status = fx_directory_default_set(&g_fx_spi_nor_media, "/");
  if (status != FX_SUCCESS)
  {
    MPRINTF("Error: Could not access root directory. Run write test first.\n\r");
    return;
  }

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
      MPRINTF("FAILED: %s (%6u us)\n\r", _Get_YFFS2_error_description(status), operation_time);
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
  Description: Perform format test

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_format_test(void)
{
  GET_MCBL;
  T_sys_timestump start_ts, end_ts;
  UINT            status;

  MPRINTF("\n=== YFFS2 Format Test ===\n\r");
  MPRINTF("WARNING: This will erase all data on the media!\n\r");
  MPRINTF("Press 'Y' to confirm or any other key to cancel: ");

  uint8_t confirm;
  if (WAIT_CHAR(&confirm, ms_to_ticks(30000)) != RES_OK || (confirm != 'Y' && confirm != 'y'))
  {
    MPRINTF("\nFormat cancelled.\n\r");
    return;
  }

  // Allocate test buffer
  if (!_Allocate_test_buffer())
  {
    MPRINTF("\nError: Failed to allocate test buffer\n\r");
    return;
  }

  MPRINTF("\nFormatting media...\n\r");

  Get_hw_timestump(&start_ts);

  // Close media first
  fx_media_close(&g_fx_spi_nor_media);

  // Format the media using LevelX NOR driver
  status = fx_media_format(&g_fx_spi_nor_media,
                           MC80_YFFS2_LevelX_DeviceDriver,           // Driver function
                           (void *)&g_rm_YFFS2_levelx_NOR_instance,  // Driver info pointer
                           (UCHAR *)g_test_buffer,                   // Memory pointer for work area
                           FILEX_TEST_BUFFER_SIZE,                   // Memory size
                           G_FX_MEDIA_OSPI_NOR_VOLUME_NAME,          // Volume name
                           G_FX_MEDIA_OSPI_NOR_NUMBER_OF_FATS,       // Number of FATs
                           G_FX_MEDIA_OSPI_NOR_DIRECTORY_ENTRIES,    // Directory entries
                           G_FX_MEDIA_OSPI_NOR_HIDDEN_SECTORS,       // Hidden sectors
                           G_FX_MEDIA_OSPI_NOR_TOTAL_SECTORS,        // Total sectors
                           G_FX_MEDIA_OSPI_NOR_BYTES_PER_SECTOR,     // Bytes per sector
                           G_FX_MEDIA_OSPI_NOR_SECTORS_PER_CLUSTER,  // Sectors per cluster
                           1,                                        // Heads
                           1);                                       // Sectors per track

  if (status == FX_SUCCESS)
  {
    // Reopen the media with the saved memory buffer
    status = fx_media_open(&g_fx_spi_nor_media, "YFFS2 Media", MC80_YFFS2_LevelX_DeviceDriver,
                           (void *)&g_rm_YFFS2_levelx_NOR_instance, g_YFFS2_memory_buffer, FILEX_MEMORY_BUFFER_SIZE);
  }

  Get_hw_timestump(&end_ts);
  uint32_t format_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (status == FX_SUCCESS)
  {
    MPRINTF("Format completed successfully in %lu us\n\r", format_time);
    _Print_YFFS2_info();
  }
  else
  {
    MPRINTF("Format failed with error: %s\n\r", _Get_YFFS2_error_description(status));
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
  Description: Initialize YFFS2 media

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_YFFS2_init(uint8_t keycode)
{
  GET_MCBL;
  UINT            status;
  T_sys_timestump start_ts, end_ts;
  uint8_t        *media_memory;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== YFFS2 with LevelX Initialization ===\n\r");

  // Check if media is already open
  if (g_fx_spi_nor_media.fx_media_id == FX_MEDIA_ID)
  {
    MPRINTF("YFFS2 media is already initialized and open.\n\r");
    _Print_YFFS2_info();
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  // Allocate memory for YFFS2 operations
  media_memory = App_malloc(FILEX_MEMORY_BUFFER_SIZE);
  if (media_memory == NULL)
  {
    MPRINTF("Error: Failed to allocate memory for YFFS2 operations\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  Get_hw_timestump(&start_ts);

  // Initialize YFFS2 media
  status = fx_media_open(&g_fx_spi_nor_media, "YFFS2 NOR Media",
                         MC80_YFFS2_LevelX_DeviceDriver,
                         (void *)&g_rm_YFFS2_levelx_NOR_instance,
                         media_memory, FILEX_MEMORY_BUFFER_SIZE);

  Get_hw_timestump(&end_ts);
  uint32_t init_time = Timestump_diff_to_usec(&start_ts, &end_ts) / 1000;  // Convert to ms

  if (status == FX_SUCCESS)
  {
    MPRINTF("YFFS2 media initialized successfully in %lu ms\n\r", init_time);
    g_YFFS2_memory_buffer = media_memory;  // Save pointer for later use
    _Print_YFFS2_info();
  }
  else if (status == FX_BOOT_ERROR)
  {
    MPRINTF("YFFS2 media initialization failed: %s\n\r", _Get_YFFS2_error_description(status));
    MPRINTF("Media appears to be unformatted. Attempting to format...\n\r");

    // Allocate test buffer for formatting
    if (!_Allocate_test_buffer())
    {
      MPRINTF("Error: Failed to allocate test buffer for formatting\n\r");
      App_free(media_memory);
      MPRINTF("\nPress any key to continue...\n\r");
      uint8_t key;
      WAIT_CHAR(&key, ms_to_ticks(100000));
      return;
    }

    // Close media first
    fx_media_close(&g_fx_spi_nor_media);

    // Try to format the media
    UINT format_status = fx_media_format(&g_fx_spi_nor_media,
                                         MC80_YFFS2_LevelX_DeviceDriver,
                                         (void *)&g_rm_YFFS2_levelx_NOR_instance,
                                         (UCHAR *)g_test_buffer,
                                         FILEX_TEST_BUFFER_SIZE,
                                         G_FX_MEDIA_OSPI_NOR_VOLUME_NAME,          // Volume name
                                         G_FX_MEDIA_OSPI_NOR_NUMBER_OF_FATS,       // Number of FATs
                                         G_FX_MEDIA_OSPI_NOR_DIRECTORY_ENTRIES,    // Directory entries
                                         G_FX_MEDIA_OSPI_NOR_HIDDEN_SECTORS,       // Hidden sectors
                                         G_FX_MEDIA_OSPI_NOR_TOTAL_SECTORS,        // Total sectors
                                         G_FX_MEDIA_OSPI_NOR_BYTES_PER_SECTOR,     // Bytes per sector
                                         G_FX_MEDIA_OSPI_NOR_SECTORS_PER_CLUSTER,  // Sectors per cluster
                                         1,                                        // Heads
                                         1);                                       // Sectors per track

    if (format_status == FX_SUCCESS)
    {
      MPRINTF("Format successful! Reopening media...\n\r");

      // Try to reopen the formatted media
      status = fx_media_open(&g_fx_spi_nor_media, "YFFS2 NOR Media",
                             MC80_YFFS2_LevelX_DeviceDriver,
                             (void *)&g_rm_YFFS2_levelx_NOR_instance,
                             media_memory, FILEX_MEMORY_BUFFER_SIZE);

      if (status == FX_SUCCESS)
      {
        MPRINTF("YFFS2 media formatted and opened successfully!\n\r");
        g_YFFS2_memory_buffer = media_memory;
        _Print_YFFS2_info();
      }
      else
      {
        MPRINTF("Failed to reopen formatted media: %s\n\r", _Get_YFFS2_error_description(status));
        App_free(media_memory);
        _Free_test_buffer();
      }
    }
    else
    {
      MPRINTF("Format failed: %s\n\r", _Get_YFFS2_error_description(format_status));
      App_free(media_memory);
      _Free_test_buffer();
    }
  }
  else
  {
    MPRINTF("YFFS2 media initialization failed: %s\n\r", _Get_YFFS2_error_description(status));
    // Free allocated memory on failure
    App_free(media_memory);
  }

  MPRINTF("\nPress any key to continue...\n\r");
  uint8_t key;
  WAIT_CHAR(&key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: List files and directories

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_YFFS2_list_files(uint8_t keycode)
{
  GET_MCBL;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== YFFS2 Directory Listing ===\n\r");

  // Check if media is open
  if (g_fx_spi_nor_media.fx_media_id != FX_MEDIA_ID)
  {
    MPRINTF("Error: YFFS2 media not initialized. Please initialize first.\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  _Print_YFFS2_info();

  MPRINTF("\n=== Directory Tree (max depth 5) ===\n\r");
  g_stack_top = 0;  // Reset directory stack
  _List_directory_tree("/", 5);

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
            MPRINTF("Failed to open file: %s\n\r", _Get_YFFS2_error_description(status));
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
                  // Read file in blocks using FILEX_TEST_BUFFER_SIZE
                  uint32_t block_size       = FILEX_TEST_BUFFER_SIZE;
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
                              current_offset, _Get_YFFS2_error_description(status));
                      break;
                    }

                    // Read one block
                    ULONG actual_bytes_read;
                    status = fx_file_read(&file, g_test_buffer, bytes_to_read, &actual_bytes_read);
                    if (status != FX_SUCCESS)
                    {
                      MPRINTF("Failed to read file at offset %lu: %s\n\r",
                              current_offset, _Get_YFFS2_error_description(status));
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
              MPRINTF("Failed to get file size: %s\n\r", _Get_YFFS2_error_description(status));
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
void Do_YFFS2_performance_test(uint8_t keycode)
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
    MPRINTF("=== YFFS2 Test Operations ===\n\r");

    // Check if media is open
    if (g_fx_spi_nor_media.fx_media_id != FX_MEDIA_ID)
    {
      MPRINTF("\nERROR: YFFS2 media not initialized!\n\r");
      MPRINTF("Please initialize YFFS2 first from the main menu.\n\r");
      MPRINTF("\nPress any key to return...\n\r");
      WAIT_CHAR(&choice, ms_to_ticks(100000));
      return;
    }

    _Print_YFFS2_info();
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
