#include "App.h"
#include "LevelX_config.h"

#define FILEX_TEST_FILES_COUNT_DEFAULT     100
#define FILEX_TEST_FILE_SIZE_DEFAULT       (4 * 1024)    // 4KB
#define FILEX_TEST_BLOCK_SIZE_DEFAULT      (64 * 1024)   // 64KB
#define DEFAULT_FILL_CONSTANT              0xAA
#define MAX_PATH_LENGTH                    256
#define MAX_FILENAME_LENGTH                64
#define MAX_DIR_STACK_DEPTH                32

// Test data patterns
enum
{
  DATA_PATTERN_CONSTANT   = 0,
  DATA_PATTERN_INCREMENTAL = 1,
  DATA_PATTERN_RANDOM     = 2,
  DATA_PATTERN_CHECKSUM   = 3
};

// Test configuration variables
static uint32_t g_test_files_count         = FILEX_TEST_FILES_COUNT_DEFAULT;
static uint32_t g_test_file_size           = FILEX_TEST_FILE_SIZE_DEFAULT;
static uint32_t g_test_block_size          = FILEX_TEST_BLOCK_SIZE_DEFAULT;
static uint32_t g_data_pattern             = DATA_PATTERN_CONSTANT;
static uint32_t g_fill_constant            = DEFAULT_FILL_CONSTANT;
static bool     g_verify_data              = true;

// Directory navigation stack
typedef struct
{
  char     path[MAX_PATH_LENGTH];
  uint8_t  depth;
} T_dir_entry;

static T_dir_entry g_dir_stack[MAX_DIR_STACK_DEPTH];
static uint32_t    g_stack_top = 0;

// Test statistics structure
typedef struct
{
  uint32_t total_time;        // Total time in ms
  uint32_t avg_time;          // Average time per operation
  uint32_t max_time;          // Maximum time
  uint32_t min_time;          // Minimum time
  uint32_t operations_count;  // Number of operations performed
  uint32_t errors_count;      // Number of errors encountered
  uint32_t bytes_processed;   // Total bytes processed
} T_filex_stats;

// FileX media instance
extern FX_MEDIA g_fx_spi_nor_media;

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

static uint8_t g_test_buffer[8192];  // Test buffer for file operations

/*-----------------------------------------------------------------------------------------------------
  Description: Static function declarations

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_write_test(void);
static void _Do_read_test(void);
static void _Do_delete_test(void);
static void _Do_format_test(void);
static void _Do_full_test(void);
static void _Print_filex_info(void);
static void _Print_test_config(void);
static void _Print_statistics(T_filex_stats *stats, const char *operation_name);
static void _Fill_test_buffer(uint8_t *buffer, uint32_t size, uint32_t file_index);
static bool _Verify_test_buffer(uint8_t *buffer, uint32_t size, uint32_t file_index);
static bool _Push_dir_to_stack(const char *path, uint8_t depth);
static bool _Pop_dir_from_stack(char *path, uint8_t *depth);
static void _Print_tree_indent(uint8_t depth);
static void _List_directory_tree(const char *root_path, uint8_t max_depth);
static const char *_Get_pattern_name(uint32_t pattern);
static const char *_Get_filex_error_description(UINT status);

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
    case DATA_PATTERN_INCREMENTAL:
      return "Incremental";
    case DATA_PATTERN_RANDOM:
      return "Random";
    case DATA_PATTERN_CHECKSUM:
      return "Checksum";
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

  Parameters: buffer - buffer to fill, size - buffer size, file_index - file index for pattern

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Fill_test_buffer(uint8_t *buffer, uint32_t size, uint32_t file_index)
{
  switch (g_data_pattern)
  {
    case DATA_PATTERN_CONSTANT:
      memset(buffer, g_fill_constant, size);
      break;

    case DATA_PATTERN_INCREMENTAL:
      for (uint32_t i = 0; i < size; i++)
      {
        buffer[i] = (uint8_t)((file_index + i) & 0xFF);
      }
      break;

    case DATA_PATTERN_RANDOM:
      for (uint32_t i = 0; i < size; i++)
      {
        buffer[i] = (uint8_t)(rand() & 0xFF);
      }
      break;

    case DATA_PATTERN_CHECKSUM:
      {
        uint32_t checksum = file_index;
        for (uint32_t i = 0; i < size; i++)
        {
          buffer[i] = (uint8_t)(checksum & 0xFF);
          checksum = (checksum + 1) * 0x1234567;
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

  Parameters: buffer - buffer to verify, size - buffer size, file_index - file index for pattern

  Return: true if data matches, false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Verify_test_buffer(uint8_t *buffer, uint32_t size, uint32_t file_index)
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

  _Fill_test_buffer(expected_buffer, size, file_index);
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
  ULONG total_clusters, available_clusters;
  ULONG sectors_per_cluster, bytes_per_sector;

  MPRINTF("\n=== FileX Media Information ===\n\r");

  // Get media information
  UINT status = fx_media_space_available(&g_fx_spi_nor_media, &available_clusters);
  if (status == FX_SUCCESS)
  {
    total_clusters = g_fx_spi_nor_media.fx_media_total_clusters;
    sectors_per_cluster = g_fx_spi_nor_media.fx_media_sectors_per_cluster;
    bytes_per_sector = g_fx_spi_nor_media.fx_media_bytes_per_sector;

    MPRINTF("Media ID: 0x%lX\n\r", g_fx_spi_nor_media.fx_media_id);
    MPRINTF("Total clusters: %lu\n\r", total_clusters);
    MPRINTF("Available clusters: %lu\n\r", available_clusters);
    MPRINTF("Used clusters: %lu\n\r", total_clusters - available_clusters);
    MPRINTF("Sectors per cluster: %lu\n\r", sectors_per_cluster);
    MPRINTF("Bytes per sector: %lu\n\r", bytes_per_sector);
    MPRINTF("Cluster size: %lu bytes\n\r", sectors_per_cluster * bytes_per_sector);
    MPRINTF("Total space: %lu KB\n\r", (total_clusters * sectors_per_cluster * bytes_per_sector) / 1024);
    MPRINTF("Available space: %lu KB\n\r", (available_clusters * sectors_per_cluster * bytes_per_sector) / 1024);
    MPRINTF("Used space: %lu KB\n\r", ((total_clusters - available_clusters) * sectors_per_cluster * bytes_per_sector) / 1024);
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
  MPRINTF("Files count: %lu\n\r", g_test_files_count);
  MPRINTF("File size: %lu bytes (%.1f KB)\n\r", g_test_file_size, (float)g_test_file_size / 1024.0f);
  MPRINTF("Block size: %lu bytes (%.1f KB)\n\r", g_test_block_size, (float)g_test_block_size / 1024.0f);
  MPRINTF("Data pattern: %s", _Get_pattern_name(g_data_pattern));
  if (g_data_pattern == DATA_PATTERN_CONSTANT)
  {
    MPRINTF(" (0x%02X)", (uint8_t)g_fill_constant);
  }
  MPRINTF("\n\r");
  MPRINTF("Data verification: %s\n\r", g_verify_data ? "Enabled" : "Disabled");
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print operation statistics

  Parameters: stats - statistics structure, operation_name - name of operation

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_statistics(T_filex_stats *stats, const char *operation_name)
{
  GET_MCBL;

  MPRINTF("\n=== %s Statistics ===\n\r", operation_name);
  MPRINTF("Operations: %lu\n\r", stats->operations_count);
  MPRINTF("Errors: %lu\n\r", stats->errors_count);
  MPRINTF("Bytes processed: %lu (%lu KB)\n\r", stats->bytes_processed, stats->bytes_processed / 1024);
  MPRINTF("Total time: %lu us\n\r", stats->total_time);

  if (stats->operations_count > 0)
  {
    MPRINTF("Average time: %lu us\n\r", stats->avg_time);
    MPRINTF("Min time: %lu us\n\r", stats->min_time);
    MPRINTF("Max time: %lu us\n\r", stats->max_time);

    if (stats->total_time > 0)
    {
      uint32_t throughput = (stats->bytes_processed * 1000000) / (stats->total_time * 1024);  // KB/s (microseconds to seconds)
      MPRINTF("Throughput: %lu KB/s\n\r", throughput);
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
  CHAR entry_name[FX_MAX_LONG_NAME_LEN];
  UINT attributes;
  ULONG size;
  UINT year, month, day, hour, minute, second;
  UINT status;
  char current_path[MAX_PATH_LENGTH];
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
  T_filex_stats stats = {0};
  T_sys_timestump start_ts, end_ts;
  FX_FILE file;
  CHAR filename[MAX_FILENAME_LENGTH];
  UINT status;

  MPRINTF("\n=== FileX Write Test ===\n\r");
  _Print_test_config();

  Get_hw_timestump(&start_ts);
  stats.min_time = UINT32_MAX;

  // Create test directory
  status = fx_directory_create(&g_fx_spi_nor_media, "test_files");
  if (status != FX_SUCCESS && status != FX_ALREADY_CREATED)
  {
    MPRINTF("Error creating test directory: %s\n\r", _Get_filex_error_description(status));
    return;
  }

  // Change to test directory
  status = fx_directory_default_set(&g_fx_spi_nor_media, "/test_files");
  if (status != FX_SUCCESS)
  {
    MPRINTF("Error changing to test directory: %s\n\r", _Get_filex_error_description(status));
    return;
  }

  MPRINTF("Writing %lu files of %lu bytes each...\n\r", g_test_files_count, g_test_file_size);

  for (uint32_t i = 0; i < g_test_files_count; i++)
  {
    snprintf(filename, sizeof(filename), "test_%04lu.dat", i);

    T_sys_timestump file_start_ts, file_end_ts;
    Get_hw_timestump(&file_start_ts);

    // Create and open file
    status = fx_file_create(&g_fx_spi_nor_media, filename);
    if (status == FX_SUCCESS || status == FX_ALREADY_CREATED)
    {
      status = fx_file_open(&g_fx_spi_nor_media, &file, filename, FX_OPEN_FOR_WRITE);
      if (status == FX_SUCCESS)
      {
        uint32_t bytes_to_write = g_test_file_size;
        uint32_t total_written = 0;
        bool write_error = false;

        while (total_written < bytes_to_write && !write_error)
        {
          uint32_t chunk_size = (bytes_to_write - total_written > sizeof(g_test_buffer)) ?
                                sizeof(g_test_buffer) : (bytes_to_write - total_written);

          _Fill_test_buffer(g_test_buffer, chunk_size, i);

          status = fx_file_write(&file, g_test_buffer, chunk_size);
          if (status == FX_SUCCESS)
          {
            total_written += chunk_size;
          }
          else
          {
            MPRINTF("Write error in file %s at offset %lu: %s\n\r", filename, total_written, _Get_filex_error_description(status));
            write_error = true;
            stats.errors_count++;
          }
        }

        fx_file_close(&file);

        if (!write_error)
        {
          stats.bytes_processed += total_written;
        }
      }
      else
      {
        MPRINTF("Error opening file %s: %s\n\r", filename, _Get_filex_error_description(status));
        stats.errors_count++;
      }
    }
    else
    {
      MPRINTF("Error creating file %s: %s\n\r", filename, _Get_filex_error_description(status));
      stats.errors_count++;
    }

    Get_hw_timestump(&file_end_ts);
    uint32_t file_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

    if (file_time < stats.min_time) stats.min_time = file_time;
    if (file_time > stats.max_time) stats.max_time = file_time;

    stats.operations_count++;

    // Progress indicator
    if ((i + 1) % 10 == 0 || i == g_test_files_count - 1)
    {
      MPRINTF("Progress: %lu/%lu files written\r", i + 1, g_test_files_count);
    }
  }

  Get_hw_timestump(&end_ts);
  stats.total_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (stats.operations_count > 0)
  {
    stats.avg_time = stats.total_time / stats.operations_count;
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
  T_filex_stats stats = {0};
  T_sys_timestump start_ts, end_ts;
  FX_FILE file;
  CHAR filename[MAX_FILENAME_LENGTH];
  UINT status;
  ULONG actual_read;

  MPRINTF("\n=== FileX Read Test ===\n\r");
  _Print_test_config();

  // Change to test directory
  status = fx_directory_default_set(&g_fx_spi_nor_media, "/test_files");
  if (status != FX_SUCCESS)
  {
    MPRINTF("Error: test_files directory not found. Run write test first.\n\r");
    return;
  }

  Get_hw_timestump(&start_ts);
  stats.min_time = UINT32_MAX;

  MPRINTF("Reading %lu files of %lu bytes each...\n\r", g_test_files_count, g_test_file_size);

  for (uint32_t i = 0; i < g_test_files_count; i++)
  {
    snprintf(filename, sizeof(filename), "test_%04lu.dat", i);

    T_sys_timestump file_start_ts, file_end_ts;
    Get_hw_timestump(&file_start_ts);

    // Open file for reading
    status = fx_file_open(&g_fx_spi_nor_media, &file, filename, FX_OPEN_FOR_READ);
    if (status == FX_SUCCESS)
    {
      uint32_t bytes_to_read = g_test_file_size;
      uint32_t total_read = 0;
      bool read_error = false;
      bool verify_error = false;

      while (total_read < bytes_to_read && !read_error)
      {
        uint32_t chunk_size = (bytes_to_read - total_read > sizeof(g_test_buffer)) ?
                              sizeof(g_test_buffer) : (bytes_to_read - total_read);

        status = fx_file_read(&file, g_test_buffer, chunk_size, &actual_read);
        if (status == FX_SUCCESS && actual_read == chunk_size)
        {
          total_read += actual_read;

          // Verify data if enabled
          if (g_verify_data && !_Verify_test_buffer(g_test_buffer, actual_read, i))
          {
            MPRINTF("Data verification failed in file %s at offset %lu\n\r", filename, total_read - actual_read);
            verify_error = true;
            stats.errors_count++;
          }
        }
        else
        {
          MPRINTF("Read error in file %s at offset %lu: %s (read %lu, expected %lu)\n\r",
                  filename, total_read, _Get_filex_error_description(status), actual_read, chunk_size);
          read_error = true;
          stats.errors_count++;
        }
      }

      fx_file_close(&file);

      if (!read_error && !verify_error)
      {
        stats.bytes_processed += total_read;
      }
    }
    else
    {
      MPRINTF("Error opening file %s: %s\n\r", filename, _Get_filex_error_description(status));
      stats.errors_count++;
    }

    Get_hw_timestump(&file_end_ts);
    uint32_t file_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

    if (file_time < stats.min_time) stats.min_time = file_time;
    if (file_time > stats.max_time) stats.max_time = file_time;

    stats.operations_count++;

    // Progress indicator
    if ((i + 1) % 10 == 0 || i == g_test_files_count - 1)
    {
      MPRINTF("Progress: %lu/%lu files read\r", i + 1, g_test_files_count);
    }
  }

  Get_hw_timestump(&end_ts);
  stats.total_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (stats.operations_count > 0)
  {
    stats.avg_time = stats.total_time / stats.operations_count;
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
  T_filex_stats stats = {0};
  T_sys_timestump start_ts, end_ts;
  CHAR filename[MAX_FILENAME_LENGTH];
  UINT status;

  MPRINTF("\n=== FileX Delete Test ===\n\r");

  // Change to test directory
  status = fx_directory_default_set(&g_fx_spi_nor_media, "/test_files");
  if (status != FX_SUCCESS)
  {
    MPRINTF("Error: test_files directory not found. Run write test first.\n\r");
    return;
  }

  Get_hw_timestump(&start_ts);
  stats.min_time = UINT32_MAX;

  MPRINTF("Deleting %lu test files...\n\r", g_test_files_count);

  for (uint32_t i = 0; i < g_test_files_count; i++)
  {
    snprintf(filename, sizeof(filename), "test_%04lu.dat", i);

    T_sys_timestump file_start_ts, file_end_ts;
    Get_hw_timestump(&file_start_ts);

    status = fx_file_delete(&g_fx_spi_nor_media, filename);
    if (status == FX_SUCCESS)
    {
      stats.bytes_processed += g_test_file_size;  // Assume file was the expected size
    }
    else
    {
      MPRINTF("Error deleting file %s: %s\n\r", filename, _Get_filex_error_description(status));
      stats.errors_count++;
    }

    Get_hw_timestump(&file_end_ts);
    uint32_t file_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

    if (file_time < stats.min_time) stats.min_time = file_time;
    if (file_time > stats.max_time) stats.max_time = file_time;

    stats.operations_count++;

    // Progress indicator
    if ((i + 1) % 10 == 0 || i == g_test_files_count - 1)
    {
      MPRINTF("Progress: %lu/%lu files deleted\r", i + 1, g_test_files_count);
    }
  }

  // Return to root directory and delete test directory
  fx_directory_default_set(&g_fx_spi_nor_media, "/");
  status = fx_directory_delete(&g_fx_spi_nor_media, "test_files");
  if (status != FX_SUCCESS)
  {
    MPRINTF("Warning: Could not delete test_files directory: %s\n\r", _Get_filex_error_description(status));
  }

  Get_hw_timestump(&end_ts);
  stats.total_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (stats.operations_count > 0)
  {
    stats.avg_time = stats.total_time / stats.operations_count;
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
  UINT status;

  MPRINTF("\n=== FileX Format Test ===\n\r");
  MPRINTF("WARNING: This will erase all data on the media!\n\r");
  MPRINTF("Press 'Y' to confirm or any other key to cancel: ");

  uint8_t confirm;
  if (WAIT_CHAR(&confirm, ms_to_ticks(30000)) != RES_OK || (confirm != 'Y' && confirm != 'y'))
  {
    MPRINTF("\nFormat cancelled.\n\r");
    return;
  }

  MPRINTF("\nFormatting media...\n\r");

  Get_hw_timestump(&start_ts);

  // Close media first
  fx_media_close(&g_fx_spi_nor_media);

  // Format the media using LevelX NOR driver
  status = fx_media_format(&g_fx_spi_nor_media,
                          MC80_FileX_LevelX_DeviceDriver,  // Driver function
                          (void*)&g_rm_filex_levelx_NOR_instance,   // Driver info pointer
                          (UCHAR*)g_test_buffer,             // Memory pointer for work area
                          sizeof(g_test_buffer),             // Memory size
                          "FILEX_TEST",                      // Volume name
                          1,                                 // Number of FATs
                          32,                                // Directory entries
                          0,                                 // Hidden sectors
                          0,                                 // Total sectors (0 = use all available)
                          512,                               // Bytes per sector
                          1,                                 // Sectors per cluster
                          1,                                 // Heads
                          1);                                // Sectors per track

  if (status == FX_SUCCESS)
  {
    // Reopen the media
    status = fx_media_open(&g_fx_spi_nor_media, "FileX Media", MC80_FileX_LevelX_DeviceDriver,
                          (void*)&g_rm_filex_levelx_NOR_instance, NULL, 0);
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
  UINT status;
  T_sys_timestump start_ts, end_ts;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== FileX with LevelX Initialization ===\n\r");

  Get_hw_timestump(&start_ts);

  // Initialize FileX media
  status = fx_media_open(&g_fx_spi_nor_media, "FileX NOR Media",
                        MC80_FileX_LevelX_DeviceDriver,
                        (void*)&g_rm_filex_levelx_NOR_instance,
                        NULL, 0);

  Get_hw_timestump(&end_ts);
  uint32_t init_time = Timestump_diff_to_usec(&start_ts, &end_ts) / 1000;  // Convert to ms

  if (status == FX_SUCCESS)
  {
    MPRINTF("FileX media initialized successfully in %lu ms\n\r", init_time);
    _Print_filex_info();
  }
  else
  {
    MPRINTF("FileX media initialization failed: %s\n\r", _Get_filex_error_description(status));
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
  bool exit_menu = false;

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
          g_test_file_size = FILEX_TEST_FILE_SIZE_DEFAULT;
          g_test_block_size = FILEX_TEST_BLOCK_SIZE_DEFAULT;
          g_data_pattern = DATA_PATTERN_CONSTANT;
          g_fill_constant = DEFAULT_FILL_CONSTANT;
          g_verify_data = true;
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
}
