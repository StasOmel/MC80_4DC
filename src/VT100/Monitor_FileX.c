#include "App.h"
#include "LevelX_config.h"

#define FILEX_TEST_FILES_COUNT_DEFAULT 10
#define FILEX_TEST_FILE_SIZE_DEFAULT   (10 * 1024)  // 10KB
#define FILEX_TEST_BLOCK_SIZE_DEFAULT  (64 * 1024)  // 64KB
#define FILEX_TEST_FILE_PREFIX         "test_"      // File name prefix
#define DEFAULT_FILL_CONSTANT          0xAA
#define MAX_PATH_LENGTH                256
#define MAX_FILENAME_LENGTH            64
#define MAX_DIR_STACK_DEPTH            32
#define FILEX_MEMORY_BUFFER_SIZE       (32 * 1024)  // 32KB
#define FILEX_TEST_BUFFER_SIZE         (16 * 1024)  // 16KB for test operations
#define CRC32_SIZE                     4             // CRC32 size in bytes

// Test data patterns
enum
{
  DATA_PATTERN_CONSTANT    = 0,
  DATA_PATTERN_COUNTER     = 1,  // Fill with 32-bit counter (like LittleFS)
  DATA_PATTERN_RANDOM      = 2
};

// Test configuration variables
static uint32_t g_test_files_count = FILEX_TEST_FILES_COUNT_DEFAULT;
static uint32_t g_test_file_size   = FILEX_TEST_FILE_SIZE_DEFAULT;
static uint32_t g_test_block_size  = FILEX_TEST_BLOCK_SIZE_DEFAULT;
static uint32_t g_data_pattern     = DATA_PATTERN_CONSTANT;
static uint32_t g_fill_constant    = DEFAULT_FILL_CONSTANT;
static bool     g_verify_data      = true;

// Directory navigation stack
typedef struct
{
  char    path[MAX_PATH_LENGTH];
  uint8_t depth;
} T_dir_entry;

static T_dir_entry g_dir_stack[MAX_DIR_STACK_DEPTH];
static uint32_t    g_stack_top = 0;

// Test statistics structure (matches LittleFS format)
typedef struct
{
  uint32_t min_time;          // Minimum time
  uint32_t max_time;          // Maximum time
  uint32_t avg_time;          // Average time
  uint32_t total_time;        // Total time of all operations
  uint32_t success_count;     // Number of successful operations
  uint32_t error_count;       // Number of failed operations
  uint32_t total_bytes;       // Total bytes processed
  uint32_t min_speed_kbps;    // Minimum speed in KB/s
  uint32_t max_speed_kbps;    // Maximum speed in KB/s
  uint32_t avg_speed_kbps;    // Average speed in KB/s
  uint32_t total_open_time;   // Total time for file open operations
  uint32_t min_open_time;     // Minimum open time
  uint32_t max_open_time;     // Maximum open time
  uint32_t total_close_time;  // Total time for file close operations
  uint32_t min_close_time;    // Minimum close time
  uint32_t max_close_time;    // Maximum close time
  uint32_t total_io_time;     // Total time for pure I/O operations (read/write only)
  uint32_t crc_errors;        // Number of CRC verification errors
  uint32_t pattern_errors;    // Number of data pattern errors
  uint32_t size_errors;       // Number of file size errors
} T_filex_stats;

// FileX media instance
extern FX_MEDIA g_fx_spi_nor_media;

// Global pointer to FileX memory buffer
static uint8_t *g_filex_memory_buffer      = NULL;

// Global pointer to test buffer for file operations
static uint8_t *g_test_buffer              = NULL;

// Menu definition
const T_VT100_Menu_item MENU_FileX_items[] = {
  { '1', Do_FileX_init, NULL },
  { '2', Do_FileX_list_files, NULL },
  { '3', Do_FileX_performance_test, NULL },
  { 'R', NULL, NULL },
  { 0 }  // End of menu
};

const T_VT100_Menu MENU_FileX = {
  "FileX with LevelX Manager",
  "\033[5C FileX file system with LevelX wear leveling management menu\r\n"
  "\033[5C <1> - Initialize FileX with LevelX (auto-format if needed)\r\n"
  "\033[5C <2> - List files and directories\r\n"
  "\033[5C <3> - Performance test\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_FileX_items
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
static void        _Print_filex_info(void);
static void        _Print_test_config(void);
static void        _Print_statistics(T_filex_stats *stats, const char *operation_name);
static void        _Fill_test_buffer(uint8_t *buffer, uint32_t size, uint32_t file_index, uint32_t start_offset);
static bool        _Verify_test_buffer(uint8_t *buffer, uint32_t size, uint32_t file_index, uint32_t start_offset);
static bool        _Push_dir_to_stack(const char *path, uint8_t depth);
static bool        _Pop_dir_from_stack(char *path, uint8_t *depth);
static void        _Print_tree_indent(uint8_t depth);
static void        _List_directory_tree(const char *root_path, uint8_t max_depth);
static const char *_Get_pattern_name(uint32_t pattern);
static const char *_Get_filex_error_description(UINT status);
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
  Description: Get pattern name string

  Parameters: pattern - pattern type

  Return: pointer to pattern name string
-----------------------------------------------------------------------------------------------------*/
static const char *_Get_pattern_name(uint32_t pattern)
{
  switch (pattern)
  {
    case DATA_PATTERN_CONSTANT:
      return "Constant";
    case DATA_PATTERN_COUNTER:
      return "Counter";
    case DATA_PATTERN_RANDOM:
      return "Random";
    default:
      return "Unknown";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get FileX error description string

  Parameters: status - FileX error status code

  Return: pointer to error description string
-----------------------------------------------------------------------------------------------------*/
static const char *_Get_filex_error_description(UINT status)
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
  Description: Fill test buffer with specified pattern

  Parameters: buffer - buffer to fill, size - buffer size, file_index - file index for pattern, start_offset - starting offset for patterns

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Fill_test_buffer(uint8_t *buffer, uint32_t size, uint32_t file_index, uint32_t start_offset)
{
  switch (g_data_pattern)
  {
    case DATA_PATTERN_CONSTANT:
      memset(buffer, g_fill_constant, size);
      break;

    case DATA_PATTERN_COUNTER:
    {
      uint32_t *word_ptr   = (uint32_t *)buffer;
      uint32_t  counter    = start_offset / 4;
      uint32_t  word_count = size / 4;

      // Fill with 32-bit counter values
      for (uint32_t i = 0; i < word_count; i++)
      {
        word_ptr[i] = counter + i;
      }

      // Fill remaining bytes
      uint32_t remaining_bytes = size % 4;
      if (remaining_bytes > 0)
      {
        uint32_t last_value = counter + word_count;
        uint8_t *byte_ptr   = &buffer[word_count * 4];
        for (uint32_t i = 0; i < remaining_bytes; i++)
        {
          byte_ptr[i] = (uint8_t)((last_value >> (i * 8)) & 0xFF);
        }
      }
    }
    break;

    case DATA_PATTERN_RANDOM:
    {
      uint32_t seed = 0x12345678 + start_offset;
      for (uint32_t i = 0; i < size; i++)
      {
        seed      = seed * 1103515245 + 12345;  // Simple LCG
        buffer[i] = (uint8_t)(seed >> 16);
      }
    }
    break;

    default:
      memset(buffer, 0x00, size);
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Verify test buffer data matches expected pattern

  Parameters: buffer - buffer to verify, size - buffer size, file_index - file index for pattern, start_offset - starting offset for patterns

  Return: true if data matches, false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Verify_test_buffer(uint8_t *buffer, uint32_t size, uint32_t file_index, uint32_t start_offset)
{
  if (!g_verify_data)
  {
    return true;  // Skip verification if disabled
  }

  uint8_t *expected_buffer = App_malloc(size);
  if (expected_buffer == NULL)
  {
    return false;  // Memory allocation failed
  }

  _Fill_test_buffer(expected_buffer, size, file_index, start_offset);
  bool result = (memcmp(buffer, expected_buffer, size) == 0);
  App_free(expected_buffer);

  return result;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print FileX media information

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_filex_info(void)
{
  GET_MCBL;
  ULONG total_clusters, available_bytes;
  ULONG sectors_per_cluster, bytes_per_sector;

  MPRINTF("\n=== FileX Media Information ===\n\r");

  // Get media information - fx_media_space_available returns available bytes, not clusters
  UINT status = fx_media_space_available(&g_fx_spi_nor_media, &available_bytes);
  if (status == FX_SUCCESS)
  {
    total_clusters             = g_fx_spi_nor_media.fx_media_total_clusters;
    sectors_per_cluster        = g_fx_spi_nor_media.fx_media_sectors_per_cluster;
    bytes_per_sector           = g_fx_spi_nor_media.fx_media_bytes_per_sector;

    // Calculate sizes in bytes first to avoid overflow
    ULONG cluster_size_bytes   = sectors_per_cluster * bytes_per_sector;

    // Calculate cluster counts - available_bytes contains free space in bytes
    ULONG available_clusters   = available_bytes / cluster_size_bytes;

    // Use data cluster count from media structure (this is the actual user data area)
    ULONG data_clusters        = g_fx_spi_nor_media.fx_media_available_clusters;
    ULONG used_clusters        = data_clusters - available_clusters;

    // Calculate sizes based on data clusters from media structure
    ULONG total_size_bytes     = total_clusters * cluster_size_bytes;
    ULONG data_size_bytes      = data_clusters * cluster_size_bytes;
    ULONG available_size_bytes = available_bytes;  // Use actual available bytes from API
    ULONG used_size_bytes      = used_clusters * cluster_size_bytes;

    MPRINTF("Media ID             : 0x%lX\n\r", g_fx_spi_nor_media.fx_media_id);
    MPRINTF("Total clusters       : %lu\n\r", total_clusters);
    MPRINTF("Data clusters        : %lu\n\r", data_clusters);
    MPRINTF("Available clusters   : %lu\n\r", available_clusters);
    MPRINTF("Used clusters        : %lu\n\r", used_clusters);
    MPRINTF("Sectors per cluster  : %lu\n\r", sectors_per_cluster);
    MPRINTF("Bytes per sector     : %lu\n\r", bytes_per_sector);
    MPRINTF("Cluster size         : %lu bytes\n\r", cluster_size_bytes);
    MPRINTF("Total space          : %lu KB (%lu MB)\n\r", total_size_bytes / 1024, total_size_bytes / (1024 * 1024));
    MPRINTF("Data space           : %lu KB (%lu MB)\n\r", data_size_bytes / 1024, data_size_bytes / (1024 * 1024));
    MPRINTF("Available space      : %lu KB (%lu MB)\n\r", available_size_bytes / 1024, available_size_bytes / (1024 * 1024));
    MPRINTF("Used space           : %lu KB (%lu MB)\n\r", used_size_bytes / 1024, used_size_bytes / (1024 * 1024));

    // Calculate and display usage percentage based on data space
    if (data_size_bytes > 0)
    {
      ULONG usage_percent = (used_size_bytes * 100) / data_size_bytes;
      MPRINTF("Usage                : %lu%% used, %lu%% free\n\r", usage_percent, 100 - usage_percent);
    }
  }
  else
  {
    MPRINTF("Error getting media information: %s\n\r", _Get_filex_error_description(status));
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
  MPRINTF("Files count      : %lu\n\r", g_test_files_count);
  MPRINTF("File size        : %lu bytes (%.1f KB)\n\r", g_test_file_size, (float)g_test_file_size / 1024.0f);
  MPRINTF("Block size       : %lu bytes (%.1f KB)\n\r", g_test_block_size, (float)g_test_block_size / 1024.0f);
  MPRINTF("Data pattern     : %s", _Get_pattern_name(g_data_pattern));
  if (g_data_pattern == DATA_PATTERN_CONSTANT)
  {
    MPRINTF(" (0x%02X)", (uint8_t)g_fill_constant);
  }
  MPRINTF("\n\r");
  MPRINTF("Data verification: %s\n\r", g_verify_data ? "Enabled" : "Disabled");
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print operation statistics (matches LittleFS format)

  Parameters: stats - statistics structure, operation_name - name of operation

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_statistics(T_filex_stats *stats, const char *operation_name)
{
  GET_MCBL;

  MPRINTF("\n=== %s Statistics ===\n\r", operation_name);
  MPRINTF("Successful operations: %u\n\r", stats->success_count);
  MPRINTF("Failed operations:     %u\n\r", stats->error_count);

  if (stats->success_count > 0)
  {
    MPRINTF("Data processed: %u KB (%u bytes)\n\r", stats->total_bytes / 1024, stats->total_bytes);

    MPRINTF("\nTiming breakdown:\n\r");

    // File open timing statistics
    if (stats->total_open_time > 0)
    {
      float avg_open_time          = (float)stats->total_open_time / stats->success_count;
      float open_time_diff_percent = 0.0f;
      if (stats->min_open_time > 0)
      {
        open_time_diff_percent = ((float)(stats->max_open_time - stats->min_open_time) * 100.0f) / stats->min_open_time;
      }
      MPRINTF("  Open time     - Avg: %6.1f us, Min: %6u us, Max: %6u us, Diff: %5.1f%%\n\r", avg_open_time, stats->min_open_time, stats->max_open_time, open_time_diff_percent);
    }

    // File close timing statistics
    if (stats->total_close_time > 0)
    {
      float avg_close_time          = (float)stats->total_close_time / stats->success_count;
      float close_time_diff_percent = 0.0f;
      if (stats->min_close_time > 0)
      {
        close_time_diff_percent = ((float)(stats->max_close_time - stats->min_close_time) * 100.0f) / stats->min_close_time;
      }
      MPRINTF("  Close time    - Avg: %6.1f us, Min: %6u us, Max: %6u us, Diff: %5.1f%%\n\r", avg_close_time, stats->min_close_time, stats->max_close_time, close_time_diff_percent);
    }

    // Speed statistics
    MPRINTF("\nSpeed statistics:\n\r");
    MPRINTF("  Max speed:    %5u KB/s\n\r", stats->max_speed_kbps);
    MPRINTF("  Avg speed:    %5u KB/s\n\r", stats->avg_speed_kbps);
    MPRINTF("  Min speed:    %5u KB/s\n\r", stats->min_speed_kbps);

    // Print data integrity statistics if enabled
    if (g_verify_data)
    {
      MPRINTF("\nData integrity:\n\r");
      MPRINTF("  CRC errors    : %u\n\r", stats->crc_errors);
      MPRINTF("  Pattern errors: %u\n\r", stats->pattern_errors);
      MPRINTF("  Size errors   : %u\n\r", stats->size_errors);
      uint32_t total_integrity_errors = stats->crc_errors + stats->pattern_errors + stats->size_errors;
      MPRINTF("  Total errors  : %u\n\r", total_integrity_errors);
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: List FileX directory tree (non-recursive with stack)

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
      MPRINTF("Failed to set directory %s: %s\n\r", current_path, _Get_filex_error_description(status));
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
  T_filex_stats   stats;
  T_sys_timestump start_ts, end_ts;
  FX_FILE         file;
  CHAR            filename[MAX_FILENAME_LENGTH];
  UINT            status;

  MPRINTF("\n=== FileX Write Test ===\n\r");
  _Print_test_config();

  // Initialize statistics (matches LittleFS format)
  stats.min_time         = UINT32_MAX;
  stats.max_time         = 0;
  stats.avg_time         = 0;
  stats.total_time       = 0;
  stats.success_count    = 0;
  stats.error_count      = 0;
  stats.total_bytes      = 0;
  stats.min_speed_kbps   = UINT32_MAX;
  stats.max_speed_kbps   = 0;
  stats.avg_speed_kbps   = 0;
  stats.total_open_time  = 0;
  stats.min_open_time    = UINT32_MAX;
  stats.max_open_time    = 0;
  stats.total_close_time = 0;
  stats.min_close_time   = UINT32_MAX;
  stats.max_close_time   = 0;
  stats.total_io_time    = 0;
  stats.crc_errors       = 0;
  stats.pattern_errors   = 0;
  stats.size_errors      = 0;

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
    MPRINTF("Error changing to root directory: %s\n\r", _Get_filex_error_description(status));
    return;
  }

  MPRINTF("Writing %lu files of %lu bytes each...\n\r", g_test_files_count, g_test_file_size);

  for (uint32_t i = 0; i < g_test_files_count; i++)
  {
    snprintf(filename, sizeof(filename), "%s%03lu.bin", FILEX_TEST_FILE_PREFIX, i + 1);

    T_sys_timestump file_start_ts, file_end_ts;
    T_sys_timestump open_start_ts, open_end_ts, close_start_ts, close_end_ts;
    uint32_t open_time = 0, close_time = 0, io_time = 0, operation_time = 0;
    uint32_t speed_kbps = 0;

    Get_hw_timestump(&file_start_ts);

    MPRINTF("File %s: ", filename);

    // Create file
    status = fx_file_create(&g_fx_spi_nor_media, filename);
    if (status != FX_SUCCESS && status != FX_ALREADY_CREATED)
    {
      Get_hw_timestump(&file_end_ts);
      operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
      MPRINTF("FAILED (create): %s (total: %6u us)\n\r", _Get_filex_error_description(status), operation_time);
      stats.error_count++;
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
      MPRINTF("FAILED (open): %s (open: %5u us, total: %6u us)\n\r",
              _Get_filex_error_description(status), open_time, operation_time);
      stats.error_count++;
      continue;
    }

    MPRINTF("opened: %5u us, ", open_time);

    // Initialize CRC calculation
    uint32_t crc = 0xFFFFFFFF;

    // Write file data with I/O timing
    uint32_t bytes_to_write = g_test_file_size > CRC32_SIZE ? g_test_file_size - CRC32_SIZE : 0;
    uint32_t total_written  = 0;
    bool     write_error    = false;
    T_sys_timestump io_start_ts, io_end_ts;

    while (total_written < bytes_to_write && !write_error)
    {
      uint32_t chunk_size = (bytes_to_write - total_written > FILEX_TEST_BUFFER_SIZE) ? FILEX_TEST_BUFFER_SIZE : (bytes_to_write - total_written);

      _Fill_test_buffer(g_test_buffer, chunk_size, i, total_written);

      // Update CRC with this chunk
      if (g_verify_data && g_test_file_size >= CRC32_SIZE)
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
        MPRINTF("FAILED (write at offset %lu): %s (I/O: %6u us, total: %6u us)\n\r",
                total_written, _Get_filex_error_description(status), io_time, operation_time);
        write_error = true;
        stats.error_count++;
      }
    }

    if (!write_error)
    {
      // Write CRC32 at the end of file if verification enabled
      if (g_verify_data && g_test_file_size >= CRC32_SIZE)
      {
        uint32_t crc32_value = ~crc;
        Get_hw_timestump(&io_start_ts);
        status = fx_file_write(&file, &crc32_value, CRC32_SIZE);
        Get_hw_timestump(&io_end_ts);
        io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

        if (status == FX_SUCCESS)
        {
          total_written += CRC32_SIZE;
        }
        else
        {
          Get_hw_timestump(&file_end_ts);
          operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
          MPRINTF("FAILED (write CRC): %s (I/O: %6u us, total: %6u us)\n\r",
                  _Get_filex_error_description(status), io_time, operation_time);
          write_error = true;
          stats.error_count++;
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
        MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r",
                _Get_filex_error_description(status), close_time, operation_time);
        stats.error_count++;
      }
      else
      {
        // Calculate speed in KB/s based on I/O time
        if (io_time > 0)
        {
          speed_kbps = (uint32_t)((float)g_test_file_size * 1000000.0f / ((float)io_time * 1024.0f));
        }
        else
        {
          speed_kbps = 0;
        }

        MPRINTF("closed: %5u us, I/O: %6u us, total: %6u us, speed: %5u KB/s",
                close_time, io_time, operation_time, speed_kbps);

        // Show CRC32 if verification enabled
        if (g_verify_data && g_test_file_size >= CRC32_SIZE)
        {
          uint32_t final_crc = ~crc;
          MPRINTF(", CRC32: 0x%08lX", final_crc);
        }
        MPRINTF("\n\r");

        stats.total_bytes += total_written;
        stats.success_count++;

        // Update timing statistics
        uint32_t file_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
        if (file_time < stats.min_time) stats.min_time = file_time;
        if (file_time > stats.max_time) stats.max_time = file_time;
        stats.total_time += file_time;

        // Update open/close timing statistics
        stats.total_open_time += open_time;
        if (open_time < stats.min_open_time) stats.min_open_time = open_time;
        if (open_time > stats.max_open_time) stats.max_open_time = open_time;

        stats.total_close_time += close_time;
        if (close_time < stats.min_close_time) stats.min_close_time = close_time;
        if (close_time > stats.max_close_time) stats.max_close_time = close_time;

        stats.total_io_time += io_time;

        // Update speed statistics
        if (speed_kbps < stats.min_speed_kbps) stats.min_speed_kbps = speed_kbps;
        if (speed_kbps > stats.max_speed_kbps) stats.max_speed_kbps = speed_kbps;
      }
    }
    else
    {
      // Close file even if write failed
      fx_file_close(&file);
    }
  }

  Get_hw_timestump(&end_ts);

  // Calculate averages
  if (stats.success_count > 0)
  {
    stats.avg_time = stats.total_time / stats.success_count;
    if (stats.total_io_time > 0)
    {
      stats.avg_speed_kbps = (uint32_t)((float)stats.total_bytes * 1000000.0f / ((float)stats.total_io_time * 1024.0f));
    }
    if (stats.min_speed_kbps == UINT32_MAX)
    {
      stats.min_speed_kbps = 0;
    }
    if (stats.min_open_time == UINT32_MAX)
    {
      stats.min_open_time = 0;
    }
    if (stats.min_close_time == UINT32_MAX)
    {
      stats.min_close_time = 0;
    }
  }
  else
  {
    stats.min_time = 0;
    stats.avg_time = 0;
    stats.min_speed_kbps = 0;
    stats.min_open_time = 0;
    stats.max_open_time = 0;
    stats.min_close_time = 0;
    stats.max_close_time = 0;
  }

  // Return to root directory
  fx_directory_default_set(&g_fx_spi_nor_media, "/");

  MPRINTF("\n\r");
  _Print_statistics(&stats, "Write Test");
}

/*-----------------------------------------------------------------------------------------------------
  Description: Perform read test

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_read_test(void)
{
  GET_MCBL;
  T_filex_stats   stats;
  T_sys_timestump start_ts, end_ts;
  FX_FILE         file;
  CHAR            filename[MAX_FILENAME_LENGTH];
  UINT            status;
  ULONG           actual_read;

  MPRINTF("\n=== FileX Read Test ===\n\r");
  _Print_test_config();

  // Initialize statistics (matches LittleFS format)
  stats.min_time         = UINT32_MAX;
  stats.max_time         = 0;
  stats.avg_time         = 0;
  stats.total_time       = 0;
  stats.success_count    = 0;
  stats.error_count      = 0;
  stats.total_bytes      = 0;
  stats.min_speed_kbps   = UINT32_MAX;
  stats.max_speed_kbps   = 0;
  stats.avg_speed_kbps   = 0;
  stats.total_open_time  = 0;
  stats.min_open_time    = UINT32_MAX;
  stats.max_open_time    = 0;
  stats.total_close_time = 0;
  stats.min_close_time   = UINT32_MAX;
  stats.max_close_time   = 0;
  stats.total_io_time    = 0;
  stats.crc_errors       = 0;
  stats.pattern_errors   = 0;
  stats.size_errors      = 0;

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
  stats.min_time = UINT32_MAX;

  MPRINTF("Reading %lu files of %lu bytes each...\n\r", g_test_files_count, g_test_file_size);

  for (uint32_t i = 0; i < g_test_files_count; i++)
  {
    snprintf(filename, sizeof(filename), "%s%03lu.bin", FILEX_TEST_FILE_PREFIX, i + 1);

    T_sys_timestump file_start_ts, file_end_ts;
    T_sys_timestump open_start_ts, open_end_ts, close_start_ts, close_end_ts;
    uint32_t open_time = 0, close_time = 0, io_time = 0, operation_time = 0;
    uint32_t speed_kbps = 0;

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
              _Get_filex_error_description(status), open_time, operation_time);
      stats.error_count++;
      continue;
    }

    MPRINTF("opened: %5u us, ", open_time);

    // Initialize CRC calculation
    uint32_t crc = 0xFFFFFFFF;
    bool     crc_valid = true;

    // Read file data with I/O timing
    uint32_t bytes_to_read = g_test_file_size > CRC32_SIZE ? g_test_file_size - CRC32_SIZE : g_test_file_size;
    uint32_t total_read    = 0;
    bool     read_error    = false;
    bool     verify_error  = false;
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
        if (g_verify_data && !_Verify_test_buffer(g_test_buffer, actual_read, i, total_read))
        {
          Get_hw_timestump(&file_end_ts);
          operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
          MPRINTF("FAILED (verify at offset %lu): Data verification failed (I/O: %6u us, total: %6u us)\n\r",
                  total_read, io_time, operation_time);
          verify_error = true;
          stats.pattern_errors++;
          break;
        }

        total_read += actual_read;

        // Update CRC with this chunk if verification enabled
        if (g_verify_data && g_test_file_size >= CRC32_SIZE)
        {
          crc = CRC32_IEEE802_3(crc, g_test_buffer, actual_read);
        }
      }
      else
      {
        Get_hw_timestump(&file_end_ts);
        operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
        MPRINTF("FAILED (read at offset %lu): %s (read %lu, expected %lu, I/O: %6u us, total: %6u us)\n\r",
                total_read, _Get_filex_error_description(status), actual_read, chunk_size, io_time, operation_time);
        read_error = true;
        stats.error_count++;
        break;
      }
    }

    // Read and verify CRC32 if enabled
    if (!read_error && !verify_error && g_verify_data && g_test_file_size >= CRC32_SIZE)
    {
      uint32_t file_crc32, calculated_crc32;
      Get_hw_timestump(&io_start_ts);
      status = fx_file_read(&file, &file_crc32, CRC32_SIZE, &actual_read);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

      if (status == FX_SUCCESS && actual_read == CRC32_SIZE)
      {
        calculated_crc32 = ~crc;
        if (file_crc32 != calculated_crc32)
        {
          crc_valid = false;
          stats.crc_errors++;
        }
        total_read += actual_read;
      }
      else
      {
        crc_valid = false;
        stats.crc_errors++;
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
      MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r",
              _Get_filex_error_description(status), close_time, operation_time);
      stats.error_count++;
    }
    else if (!read_error && !verify_error)
    {
      // Calculate speed in KB/s based on I/O time
      if (io_time > 0)
      {
        speed_kbps = (uint32_t)((float)g_test_file_size * 1000000.0f / ((float)io_time * 1024.0f));
      }
      else
      {
        speed_kbps = 0;
      }

      MPRINTF("closed: %5u us, I/O: %6u us, total: %6u us, speed: %5u KB/s",
              close_time, io_time, operation_time, speed_kbps);

      // Show CRC32 status if verification enabled
      if (g_verify_data && g_test_file_size >= CRC32_SIZE)
      {
        if (crc_valid)
        {
          uint32_t calculated_crc = ~crc;
          MPRINTF(", CRC32: OK (0x%08lX)", calculated_crc);
        }
        else
        {
          MPRINTF(", CRC32: FAILED");
        }
      }
      MPRINTF("\n\r");

      stats.total_bytes += total_read;
      stats.success_count++;

      // Update timing statistics
      uint32_t file_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
      if (file_time < stats.min_time) stats.min_time = file_time;
      if (file_time > stats.max_time) stats.max_time = file_time;
      stats.total_time += file_time;

      // Update open/close timing statistics
      stats.total_open_time += open_time;
      if (open_time < stats.min_open_time) stats.min_open_time = open_time;
      if (open_time > stats.max_open_time) stats.max_open_time = open_time;

      stats.total_close_time += close_time;
      if (close_time < stats.min_close_time) stats.min_close_time = close_time;
      if (close_time > stats.max_close_time) stats.max_close_time = close_time;

      stats.total_io_time += io_time;

      // Update speed statistics
      if (speed_kbps < stats.min_speed_kbps) stats.min_speed_kbps = speed_kbps;
      if (speed_kbps > stats.max_speed_kbps) stats.max_speed_kbps = speed_kbps;
    }
  }

  Get_hw_timestump(&end_ts);

  // Calculate averages
  if (stats.success_count > 0)
  {
    stats.avg_time = stats.total_time / stats.success_count;
    if (stats.total_io_time > 0)
    {
      stats.avg_speed_kbps = (uint32_t)((float)stats.total_bytes * 1000000.0f / ((float)stats.total_io_time * 1024.0f));
    }
    if (stats.min_speed_kbps == UINT32_MAX)
    {
      stats.min_speed_kbps = 0;
    }
    if (stats.min_open_time == UINT32_MAX)
    {
      stats.min_open_time = 0;
    }
    if (stats.min_close_time == UINT32_MAX)
    {
      stats.min_close_time = 0;
    }
  }
  else
  {
    stats.min_time = 0;
    stats.avg_time = 0;
    stats.min_speed_kbps = 0;
    stats.min_open_time = 0;
    stats.max_open_time = 0;
    stats.min_close_time = 0;
    stats.max_close_time = 0;
  }

  // Return to root directory
  fx_directory_default_set(&g_fx_spi_nor_media, "/");

  MPRINTF("\n\r");
  _Print_statistics(&stats, "Read Test");
}

/*-----------------------------------------------------------------------------------------------------
  Description: Perform delete test

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_delete_test(void)
{
  GET_MCBL;
  T_filex_stats   stats;
  T_sys_timestump start_ts, end_ts;
  CHAR            filename[MAX_FILENAME_LENGTH];
  UINT            status;

  MPRINTF("\n=== FileX Delete Test ===\n\r");

  // Initialize statistics (matches LittleFS format)
  stats.min_time         = UINT32_MAX;
  stats.max_time         = 0;
  stats.avg_time         = 0;
  stats.total_time       = 0;
  stats.success_count    = 0;
  stats.error_count      = 0;
  stats.total_bytes      = 0;  // Not applicable for delete
  stats.min_speed_kbps   = 0;  // Not applicable for delete
  stats.max_speed_kbps   = 0;  // Not applicable for delete
  stats.avg_speed_kbps   = 0;  // Not applicable for delete
  stats.total_open_time  = 0;  // Not applicable for delete
  stats.min_open_time    = 0;  // Not applicable for delete
  stats.max_open_time    = 0;  // Not applicable for delete
  stats.total_close_time = 0;  // Not applicable for delete
  stats.min_close_time   = 0;  // Not applicable for delete
  stats.max_close_time   = 0;  // Not applicable for delete
  stats.total_io_time    = 0;  // Not applicable for delete
  stats.crc_errors       = 0;  // Not applicable for delete
  stats.pattern_errors   = 0;  // Not applicable for delete
  stats.size_errors      = 0;  // Not applicable for delete

  // Change to root directory
  status = fx_directory_default_set(&g_fx_spi_nor_media, "/");
  if (status != FX_SUCCESS)
  {
    MPRINTF("Error: Could not access root directory. Run write test first.\n\r");
    return;
  }

  Get_hw_timestump(&start_ts);
  stats.min_time = UINT32_MAX;

  MPRINTF("Deleting %lu test files...\n\r", g_test_files_count);

  for (uint32_t i = 0; i < g_test_files_count; i++)
  {
    snprintf(filename, sizeof(filename), "%s%03lu.bin", FILEX_TEST_FILE_PREFIX, i + 1);

    T_sys_timestump file_start_ts, file_end_ts;
    uint32_t operation_time = 0;

    Get_hw_timestump(&file_start_ts);

    MPRINTF("File %s: ", filename);

    status = fx_file_delete(&g_fx_spi_nor_media, filename);
    Get_hw_timestump(&file_end_ts);
    operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

    if (status == FX_SUCCESS)
    {
      MPRINTF("deleted: %6u us\n\r", operation_time);
      stats.success_count++;

      // Update timing statistics
      if (operation_time < stats.min_time) stats.min_time = operation_time;
      if (operation_time > stats.max_time) stats.max_time = operation_time;
      stats.total_time += operation_time;
    }
    else
    {
      MPRINTF("FAILED: %s (%6u us)\n\r", _Get_filex_error_description(status), operation_time);
      stats.error_count++;
    }
  }

  Get_hw_timestump(&end_ts);

  // Calculate averages
  if (stats.success_count > 0)
  {
    stats.avg_time = stats.total_time / stats.success_count;
  }
  else
  {
    stats.min_time = 0;
    stats.avg_time = 0;
  }

  MPRINTF("\n\r");
  _Print_statistics(&stats, "Delete Test");
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

  MPRINTF("\n=== FileX Format Test ===\n\r");
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
                           MC80_FileX_LevelX_DeviceDriver,           // Driver function
                           (void *)&g_rm_filex_levelx_NOR_instance,  // Driver info pointer
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
    status = fx_media_open(&g_fx_spi_nor_media, "FileX Media", MC80_FileX_LevelX_DeviceDriver,
                           (void *)&g_rm_filex_levelx_NOR_instance, g_filex_memory_buffer, FILEX_MEMORY_BUFFER_SIZE);
  }

  Get_hw_timestump(&end_ts);
  uint32_t format_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (status == FX_SUCCESS)
  {
    MPRINTF("Format completed successfully in %lu us\n\r", format_time);
    _Print_filex_info();
  }
  else
  {
    MPRINTF("Format failed with error: %s\n\r", _Get_filex_error_description(status));
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
  MPRINTF("Files processed: %lu\n\r", g_test_files_count);
  MPRINTF("Data per file: %lu bytes\n\r", g_test_file_size);
  MPRINTF("Total data: %lu KB\n\r", (g_test_files_count * g_test_file_size) / 1024);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize FileX media

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_FileX_init(uint8_t keycode)
{
  GET_MCBL;
  UINT            status;
  T_sys_timestump start_ts, end_ts;
  uint8_t        *media_memory;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== FileX with LevelX Initialization ===\n\r");

  // Check if media is already open
  if (g_fx_spi_nor_media.fx_media_id == FX_MEDIA_ID)
  {
    MPRINTF("FileX media is already initialized and open.\n\r");
    _Print_filex_info();
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  // Allocate memory for FileX operations
  media_memory = App_malloc(FILEX_MEMORY_BUFFER_SIZE);
  if (media_memory == NULL)
  {
    MPRINTF("Error: Failed to allocate memory for FileX operations\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  Get_hw_timestump(&start_ts);

  // Initialize FileX media
  status = fx_media_open(&g_fx_spi_nor_media, "FileX NOR Media",
                         MC80_FileX_LevelX_DeviceDriver,
                         (void *)&g_rm_filex_levelx_NOR_instance,
                         media_memory, FILEX_MEMORY_BUFFER_SIZE);

  Get_hw_timestump(&end_ts);
  uint32_t init_time = Timestump_diff_to_usec(&start_ts, &end_ts) / 1000;  // Convert to ms

  if (status == FX_SUCCESS)
  {
    MPRINTF("FileX media initialized successfully in %lu ms\n\r", init_time);
    g_filex_memory_buffer = media_memory;  // Save pointer for later use
    _Print_filex_info();
  }
  else if (status == FX_BOOT_ERROR)
  {
    MPRINTF("FileX media initialization failed: %s\n\r", _Get_filex_error_description(status));
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
                                         MC80_FileX_LevelX_DeviceDriver,
                                         (void *)&g_rm_filex_levelx_NOR_instance,
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
      status = fx_media_open(&g_fx_spi_nor_media, "FileX NOR Media",
                             MC80_FileX_LevelX_DeviceDriver,
                             (void *)&g_rm_filex_levelx_NOR_instance,
                             media_memory, FILEX_MEMORY_BUFFER_SIZE);

      if (status == FX_SUCCESS)
      {
        MPRINTF("FileX media formatted and opened successfully!\n\r");
        g_filex_memory_buffer = media_memory;
        _Print_filex_info();
      }
      else
      {
        MPRINTF("Failed to reopen formatted media: %s\n\r", _Get_filex_error_description(status));
        App_free(media_memory);
        _Free_test_buffer();
      }
    }
    else
    {
      MPRINTF("Format failed: %s\n\r", _Get_filex_error_description(format_status));
      App_free(media_memory);
      _Free_test_buffer();
    }
  }
  else
  {
    MPRINTF("FileX media initialization failed: %s\n\r", _Get_filex_error_description(status));
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
void Do_FileX_list_files(uint8_t keycode)
{
  GET_MCBL;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== FileX Directory Listing ===\n\r");

  // Check if media is open
  if (g_fx_spi_nor_media.fx_media_id != FX_MEDIA_ID)
  {
    MPRINTF("Error: FileX media not initialized. Please initialize first.\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  _Print_filex_info();

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
        char filename[MAX_FILENAME_LENGTH];
        if (VT100_input_filename(filename, MAX_FILENAME_LENGTH, "test_001.bin"))
        {
          // Add leading slash if not present
          char full_filename[MAX_FILENAME_LENGTH + 1];
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
            MPRINTF("Failed to open file: %s\n\r", _Get_filex_error_description(status));
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
                              current_offset, _Get_filex_error_description(status));
                      break;
                    }

                    // Read one block
                    ULONG actual_bytes_read;
                    status = fx_file_read(&file, g_test_buffer, bytes_to_read, &actual_bytes_read);
                    if (status != FX_SUCCESS)
                    {
                      MPRINTF("Failed to read file at offset %lu: %s\n\r",
                              current_offset, _Get_filex_error_description(status));
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
              MPRINTF("Failed to get file size: %s\n\r", _Get_filex_error_description(status));
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
void Do_FileX_performance_test(uint8_t keycode)
{
  GET_MCBL;
  uint8_t choice;
  bool    exit_menu = false;

  FSP_PARAMETER_NOT_USED(keycode);

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

    _Print_filex_info();
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
          MPRINTF("\n\rEnter new files count (1-10000) [current: %lu]: ", g_test_files_count);
          uint32_t new_files_count;
          if (VT100_input_uint32(&new_files_count, 1, 10000, g_test_files_count))
          {
            g_test_files_count = new_files_count;
            MPRINTF("Files count changed to %lu\n\r", g_test_files_count);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '7':
          MPRINTF("\n\rEnter new file size in KB (1-1024) [current: %.1f]: ", (float)g_test_file_size / 1024.0f);
          uint32_t new_file_size_kb;
          if (VT100_input_uint32(&new_file_size_kb, 1, 1024, g_test_file_size / 1024))
          {
            g_test_file_size = new_file_size_kb * 1024;
            MPRINTF("File size changed to %lu bytes (%.1f KB)\n\r", g_test_file_size, (float)g_test_file_size / 1024.0f);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '8':
          MPRINTF("\n\rEnter new block size in KB (1-128) [current: %.1f]: ", (float)g_test_block_size / 1024.0f);
          uint32_t new_block_size_kb;
          if (VT100_input_uint32(&new_block_size_kb, 1, 128, g_test_block_size / 1024))
          {
            g_test_block_size = new_block_size_kb * 1024;
            MPRINTF("Block size changed to %lu bytes (%.1f KB)\n\r", g_test_block_size, (float)g_test_block_size / 1024.0f);
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
          MPRINTF("Current pattern: %lu\n\r", g_data_pattern);
          MPRINTF("Enter choice: ");
          uint32_t new_pattern;
          if (VT100_input_uint32(&new_pattern, 0, 3, g_data_pattern))
          {
            g_data_pattern = new_pattern;
            MPRINTF("Data pattern changed to %lu\n\r", g_data_pattern);
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
          g_verify_data = !g_verify_data;
          MPRINTF("\n\rData verification %s\n\r", g_verify_data ? "enabled" : "disabled");
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case 'C':
        case 'c':
          MPRINTF("\n\rEnter new constant value (0-255) [current: 0x%02X]: ", (uint8_t)g_fill_constant);
          uint32_t new_constant;
          if (VT100_input_uint32(&new_constant, 0, 255, (uint8_t)g_fill_constant))
          {
            g_fill_constant = (uint8_t)new_constant;
            MPRINTF("Constant value changed to 0x%02X\n\r", (uint8_t)g_fill_constant);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '9':
          g_test_files_count = FILEX_TEST_FILES_COUNT_DEFAULT;
          g_test_file_size   = FILEX_TEST_FILE_SIZE_DEFAULT;
          g_test_block_size  = FILEX_TEST_BLOCK_SIZE_DEFAULT;
          g_data_pattern     = DATA_PATTERN_CONSTANT;
          g_fill_constant    = DEFAULT_FILL_CONSTANT;
          g_verify_data      = true;
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
