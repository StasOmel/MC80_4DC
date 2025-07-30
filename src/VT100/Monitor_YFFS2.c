#include "App.h"
#include "yaffs_nor_device.h"
#include "yaffs_guts.h"
#include "yaffsfs.h"
#include "Performance_Stats.h"
#include "Test_Patterns.h"
#include "FS_Test_Config.h"
#include <time.h>

// YAFFS2 file operation constants
#ifndef O_CREAT
#define O_CREAT   0x0200
#endif
#ifndef O_WRONLY
#define O_WRONLY  0x0001
#endif
#ifndef O_RDONLY
#define O_RDONLY  0x0000
#endif
#ifndef O_TRUNC
#define O_TRUNC   0x0400
#endif
#ifndef S_IREAD
#define S_IREAD   0x0100
#endif
#ifndef S_IWRITE
#define S_IWRITE  0x0080
#endif
#ifndef O_SYNC
#define O_SYNC    0x1000  // Synchronous I/O - writes immediately to storage
#endif

#define MAX_PATH_LENGTH           256
#define MAX_DIR_STACK_DEPTH       32
#define YAFFS2_MEMORY_BUFFER_SIZE (32 * 1024)  // 32KB
#define YAFFS2_TEST_BUFFER_SIZE   (16 * 1024)  // 16KB for test operations

// Directory navigation stack
typedef struct
{
  char    path[MAX_PATH_LENGTH];
  uint8_t depth;
} T_dir_entry;

static T_dir_entry g_dir_stack[MAX_DIR_STACK_DEPTH];
static uint32_t    g_stack_top              = 0;

// Global pointer to test buffer for file operations
static uint8_t *g_test_buffer               = NULL;


// Menu definition
const T_VT100_Menu_item MENU_YAFFS2_items[] = {
  { '1', Do_YAFFS2_init, NULL },
  { '2', Do_YAFFS2_list_files, NULL },
  { '3', Do_YAFFS2_performance_test, NULL },
  { '4', Do_YAFFS2_list_lost_found, NULL },
  { 'R', NULL, NULL },
  { 0 }  // End of menu
};

const T_VT100_Menu MENU_YAFFS2 = {
  "YAFFS2 NOR Flash Manager",
  "\033[5C YAFFS2 file system for NOR Flash menu\r\n"
  "\033[5C <1> - Initialize YAFFS2 (auto-format if needed)\r\n"
  "\033[5C <2> - List files and directories\r\n"
  "\033[5C <3> - Performance test\r\n"
  "\033[5C <4> - List lost+found directory\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_YAFFS2_items
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
static void        _Print_YAFFS2_info(void);
static void        _Print_test_config(void);
static bool        _Push_dir_to_stack(const char *path, uint8_t depth);
static bool        _Pop_dir_from_stack(char *path, uint8_t *depth);
static void        _Print_tree_indent(uint8_t depth);
static void        _List_directory_tree(const char *root_path, uint8_t max_depth);
static const char *_Get_YAFFS2_error_description(int status);
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
  Description: Get YAFFS2 error description string

  Parameters: status - YAFFS2 error status code

  Return: pointer to error description string
-----------------------------------------------------------------------------------------------------*/
static const char *_Get_YAFFS2_error_description(int status)
{
  switch (status)
  {
    case 0:
      return "SUCCESS";
    case -1:
      return "GENERIC_ERROR";
    case -2:
      return "NOT_FOUND";
    case -3:
      return "EXISTS";
    case -4:
      return "NOT_DIRECTORY";
    case -5:
      return "IS_DIRECTORY";
    case -6:
      return "NO_SPACE";
    case -7:
      return "IO_ERROR";
    case -8:
      return "INVALID_PARAMETER";
    case -9:
      return "TOO_MANY_OBJECTS";
    case -10:
      return "ACCESS_DENIED";
    case -11:
      return "NO_MEMORY";
    case -12:
      return "BUSY";
    case -13:
      return "READ_ONLY";
    case -14:
      return "TIMEOUT";
    case -15:
      return "CROSS_DEVICE_LINK";
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
    g_test_buffer = App_malloc(YAFFS2_TEST_BUFFER_SIZE);
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
  Description: Print YAFFS2 media information

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_YAFFS2_info(void)
{
  GET_MCBL;

  MPRINTF("\n=== YAFFS2 Media Information ===\n\r");

  // Get YAFFS2 filesystem statistics using yaffs_freespace
  Y_LOFF_T free_space = yaffs_freespace("/");

  if (free_space >= 0)
  {
    // For YAFFS2, we can estimate total space based on device configuration
    // This is a simplified approach - in real implementation you'd get this from device config
    uint64_t total_space = 32 * 1024 * 1024;  // 32MB estimated (adjust based on your NOR Flash size)
    uint64_t used_space  = total_space - free_space;

    MPRINTF("Mount point          : /\n\r");
    MPRINTF("Filesystem type      : YAFFS2\n\r");
    MPRINTF("Total space          : %lu KB (%lu MB)\n\r", (uint32_t)(total_space / 1024), (uint32_t)(total_space / (1024 * 1024)));
    MPRINTF("Free space           : %lu KB (%lu MB)\n\r", (uint32_t)(free_space / 1024), (uint32_t)(free_space / (1024 * 1024)));
    MPRINTF("Used space           : %lu KB (%lu MB)\n\r", (uint32_t)(used_space / 1024), (uint32_t)(used_space / (1024 * 1024)));

    // Calculate and display usage percentage
    if (total_space > 0)
    {
      uint32_t usage_percent = (uint32_t)((used_space * 100) / total_space);
      MPRINTF("Usage                : %lu%% used, %lu%% free\n\r", usage_percent, 100 - usage_percent);
    }
  }
  else
  {
    MPRINTF("Error getting filesystem information: %s\n\r", _Get_YAFFS2_error_description(yaffs_get_error()));
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
  Description: List YAFFS2 directory tree (non-recursive with stack)

  Parameters: root_path - starting directory path, max_depth - maximum depth to traverse

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _List_directory_tree(const char *root_path, uint8_t max_depth)
{
  GET_MCBL;
  yaffs_DIR    *dir_ptr;
  struct yaffs_dirent *entry;
  char         current_path[MAX_PATH_LENGTH];
  uint8_t      current_depth;

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
    dir_ptr = yaffs_opendir(current_path);
    if (dir_ptr == NULL)
    {
      MPRINTF("Failed to open directory %s: %s\n\r", current_path, _Get_YAFFS2_error_description(yaffs_get_error()));
      continue;
    }

    // Read directory entries
    while ((entry = yaffs_readdir(dir_ptr)) != NULL)
    {
      // Skip "." and ".." entries
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
      {
        _Print_tree_indent(current_depth);

        // Get file/directory information
        char full_path[MAX_PATH_LENGTH];
        if (strcmp(current_path, "/") == 0)
        {
          snprintf(full_path, MAX_PATH_LENGTH, "/%s", entry->d_name);
        }
        else
        {
          snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", current_path, entry->d_name);
        }

        struct yaffs_stat stat_buf;
        if (yaffs_lstat(full_path, &stat_buf) >= 0)
        {
          if (S_ISDIR(stat_buf.st_mode))
          {
            MPRINTF("[DIR]  %s/\n\r", entry->d_name);

            // Add subdirectory to stack if not too deep and stack not full
            if (current_depth < max_depth - 1 && g_stack_top < MAX_DIR_STACK_DEPTH - 1)
            {
              _Push_dir_to_stack(full_path, current_depth + 1);
            }
          }
          else
          {
            // Convert time to readable format (simplified)
            time_t mod_time = stat_buf.yst_mtime;
            struct tm *time_info = localtime(&mod_time);

            MPRINTF("[FILE] %s (%lu bytes) %02d/%02d/%04d %02d:%02d:%02d\n\r",
                    entry->d_name, (uint32_t)stat_buf.st_size,
                    time_info->tm_mon + 1, time_info->tm_mday, time_info->tm_year + 1900,
                    time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
          }
        }
        else
        {
          MPRINTF("[UNKNOWN] %s (stat failed)\n\r", entry->d_name);
        }
      }
    }

    yaffs_closedir(dir_ptr);
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
  int                 file_fd;
  char                filename[FS_MAX_FILENAME_LENGTH];
  int                 result;

  MPRINTF("\n=== YAFFS2 Write Test ===\n\r");
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

  // YAFFS2 works with full paths, no need to set current directory

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

    // Open file for writing (creates automatically if doesn't exist) with timing
    // O_SYNC ensures immediate write to storage without buffering
    Get_hw_timestump(&open_start_ts);
    file_fd = yaffs_open(filename, O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IREAD | S_IWRITE);
    Get_hw_timestump(&open_end_ts);
    open_time = Timestump_diff_to_usec(&open_start_ts, &open_end_ts);

    if (file_fd < 0)
    {
      Get_hw_timestump(&file_end_ts);
      operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
      MPRINTF("FAILED (open): %s (open: %5u us, total: %6u us)\n\r", _Get_YAFFS2_error_description(yaffs_get_error()), open_time, operation_time);
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
      uint32_t chunk_size = (bytes_to_write - total_written > YAFFS2_TEST_BUFFER_SIZE) ? YAFFS2_TEST_BUFFER_SIZE : (bytes_to_write - total_written);

      Test_patterns_fill_buffer(g_test_buffer, chunk_size, g_fs_test_config.data_pattern, g_fs_test_config.fill_constant, total_written);

      // Update CRC with this chunk
      if (g_fs_test_config.data_verification && g_fs_test_config.file_size >= FS_CRC32_SIZE)
      {
        crc = CRC32_IEEE802_3(crc, g_test_buffer, chunk_size);
      }

      Get_hw_timestump(&io_start_ts);
      int bytes_written = yaffs_write(file_fd, g_test_buffer, chunk_size);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

      if (bytes_written == (int)chunk_size)
      {
        total_written += chunk_size;
      }
      else
      {
        Get_hw_timestump(&file_end_ts);
        operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
        MPRINTF("FAILED (write at offset %lu): %s (I/O: %6u us, total: %6u us)\n\r", total_written, _Get_YAFFS2_error_description(yaffs_get_error()), io_time, operation_time);
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
        int crc_bytes_written = yaffs_write(file_fd, &crc32_value, FS_CRC32_SIZE);
        Get_hw_timestump(&io_end_ts);
        io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

        if (crc_bytes_written == FS_CRC32_SIZE)
        {
          total_written += FS_CRC32_SIZE;
        }
        else
        {
          Get_hw_timestump(&file_end_ts);
          operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
          MPRINTF("FAILED (write CRC): %s (I/O: %6u us, total: %6u us)\n\r", _Get_YAFFS2_error_description(yaffs_get_error()), io_time, operation_time);
          write_error = true;
          Performance_stats_update_error(&stats);
        }
      }
    }

    if (!write_error)
    {
      // Close file with timing
      Get_hw_timestump(&close_start_ts);
      result = yaffs_close(file_fd);
      Get_hw_timestump(&close_end_ts);
      close_time = Timestump_diff_to_usec(&close_start_ts, &close_end_ts);
      Get_hw_timestump(&file_end_ts);
      operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

      if (result < 0)
      {
        MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r", _Get_YAFFS2_error_description(yaffs_get_error()), close_time, operation_time);
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
      yaffs_close(file_fd);
    }
  }

  Get_hw_timestump(&end_ts);

  // Finalize statistics calculations
  Performance_stats_finalize(&stats);

  // Return to root directory
  // YAFFS2 doesn't need to return to root directory

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
  int                 file_fd;
  char                filename[FS_MAX_FILENAME_LENGTH];
  int                 bytes_read;

  MPRINTF("\n=== YAFFS2 Read Test ===\n\r");
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
    file_fd = yaffs_open(filename, O_RDONLY, 0);
    Get_hw_timestump(&open_end_ts);
    open_time = Timestump_diff_to_usec(&open_start_ts, &open_end_ts);

    if (file_fd < 0)
    {
      Get_hw_timestump(&file_end_ts);
      operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
      MPRINTF("FAILED (open): %s (open: %5u us, total: %6u us)\n\r",
              _Get_YAFFS2_error_description(yaffs_get_error()), open_time, operation_time);
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
      uint32_t chunk_size = (bytes_to_read - total_read > YAFFS2_TEST_BUFFER_SIZE) ? YAFFS2_TEST_BUFFER_SIZE : (bytes_to_read - total_read);

      Get_hw_timestump(&io_start_ts);
      bytes_read = yaffs_read(file_fd, g_test_buffer, chunk_size);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

      if (bytes_read == (int)chunk_size)
      {
        // Verify data if enabled (use offset before incrementing total_read)
        if (g_fs_test_config.data_verification && !Test_patterns_verify_buffer(g_test_buffer, bytes_read, g_fs_test_config.data_pattern, g_fs_test_config.fill_constant, total_read))
        {
          Get_hw_timestump(&file_end_ts);
          operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
          MPRINTF("FAILED (verify at offset %lu): Data verification failed (I/O: %6u us, total: %6u us)\n\r", total_read, io_time, operation_time);
          pattern_valid = false;
          verify_error  = true;
          Performance_stats_increment_pattern_error(&stats);
          break;
        }

        total_read += bytes_read;

        // Update CRC with this chunk if verification enabled
        if (g_fs_test_config.data_verification && g_fs_test_config.file_size >= FS_CRC32_SIZE)
        {
          crc = CRC32_IEEE802_3(crc, g_test_buffer, bytes_read);
        }
      }
      else
      {
        Get_hw_timestump(&file_end_ts);
        operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);
        MPRINTF("FAILED (read at offset %lu): %s (read %d, expected %lu, I/O: %6u us, total: %6u us)\n\r", total_read, _Get_YAFFS2_error_description(yaffs_get_error()), bytes_read, chunk_size, io_time, operation_time);
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
      bytes_read = yaffs_read(file_fd, &file_crc32, FS_CRC32_SIZE);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);

      if (bytes_read == FS_CRC32_SIZE)
      {
        calculated_crc32 = ~crc;
        if (file_crc32 != calculated_crc32)
        {
          crc_valid = false;
          Performance_stats_increment_crc_error(&stats);
        }
        total_read += bytes_read;
      }
      else
      {
        crc_valid = false;
        Performance_stats_increment_crc_error(&stats);
      }
    }

    // Close file with timing
    Get_hw_timestump(&close_start_ts);
    int result = yaffs_close(file_fd);
    Get_hw_timestump(&close_end_ts);
    close_time = Timestump_diff_to_usec(&close_start_ts, &close_end_ts);
    Get_hw_timestump(&file_end_ts);
    operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

    if (result < 0)
    {
      MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r", _Get_YAFFS2_error_description(yaffs_get_error()), close_time, operation_time);
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
  // YAFFS2 doesn't need to return to root directory

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
  char                filename[FS_MAX_FILENAME_LENGTH];
  int                 result;

  MPRINTF("\n=== YAFFS2 Delete Test ===\n\r");

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

    result = yaffs_unlink(filename);
    Get_hw_timestump(&file_end_ts);
    operation_time = Timestump_diff_to_usec(&file_start_ts, &file_end_ts);

    if (result >= 0)
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
      MPRINTF("FAILED: %s (%6u us)\n\r", _Get_YAFFS2_error_description(yaffs_get_error()), operation_time);
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
  int             result;

  MPRINTF("\n=== YAFFS2 Format Test ===\n\r");
  MPRINTF("WARNING: This will erase all data on the media!\n\r");
  MPRINTF("Press 'Y' to confirm or any other key to cancel: ");

  uint8_t confirm;
  if (WAIT_CHAR(&confirm, ms_to_ticks(30000)) != RES_OK || (confirm != 'Y' && confirm != 'y'))
  {
    MPRINTF("\nFormat cancelled.\n\r");
    return;
  }

  MPRINTF("\nFormatting YAFFS2 filesystem...\n\r");

  Get_hw_timestump(&start_ts);

  // Unmount filesystem first
  result = yaffs_unmount("/");
  if (result < 0)
  {
    MPRINTF("Warning: Failed to unmount filesystem: %s\n\r", _Get_YAFFS2_error_description(yaffs_get_error()));
  }

  // YAFFS2 doesn't have a direct format function like other filesystems
  // The closest equivalent is to unmount and remount the filesystem
  // This will cause YAFFS2 to scan and rebuild its internal structures

  // Remount the filesystem (this will rebuild YAFFS2 structures)
  result = yaffs_mount("/");

  Get_hw_timestump(&end_ts);
  uint32_t format_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (result >= 0)
  {
    MPRINTF("Format completed successfully in %lu us\n\r", format_time);
    _Print_YAFFS2_info();
  }
  else
  {
    MPRINTF("Format failed with error: %s\n\r", _Get_YAFFS2_error_description(yaffs_get_error()));
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
  Description: Initialize YAFFS2 media

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_YAFFS2_init(uint8_t keycode)
{
  GET_MCBL;
  T_sys_timestump start_ts, end_ts;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== YAFFS2 NOR Flash Initialization ===\n\r");

  // Initialize YAFFS2 device (register device in device list)
  MPRINTF("Initializing YAFFS2 device...\n\r");
  if (Yaffs_nor_device_init() != 0)
  {
    MPRINTF("Failed to initialize YAFFS2 device\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("YAFFS2 device initialized successfully\n\r");

  // Check if filesystem is already mounted
  Y_LOFF_T free_space = yaffs_freespace("/");
  if (free_space >= 0)
  {
    MPRINTF("YAFFS2 filesystem is already mounted and accessible.\n\r");
    _Print_YAFFS2_info();
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Attempting to mount YAFFS2 filesystem...\n\r");

  Get_hw_timestump(&start_ts);

  // Try to mount filesystem
  int result = yaffs_mount("/");

  Get_hw_timestump(&end_ts);
  uint32_t mount_time = Timestump_diff_to_usec(&start_ts, &end_ts) / 1000;  // Convert to ms

  if (result >= 0)
  {
    MPRINTF("YAFFS2 filesystem mounted successfully in %lu ms\n\r", mount_time);
    _Print_YAFFS2_info();
  }
  else
  {
    MPRINTF("Failed to mount YAFFS2 filesystem: %s\n\r", _Get_YAFFS2_error_description(yaffs_get_error()));
    MPRINTF("This could indicate:\n\r");
    MPRINTF("- Device not properly configured\n\r");
    MPRINTF("- Filesystem corrupted or unformatted\n\r");
    MPRINTF("- Hardware connection issues\n\r");
    MPRINTF("\nYou can try using the format option from the performance test menu.\n\r");
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
void Do_YAFFS2_list_files(uint8_t keycode)
{
  GET_MCBL;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== YFFS2 Directory Listing ===\n\r");

  // Check if filesystem is mounted by checking free space
  Y_LOFF_T free_space = yaffs_freespace("/");
  if (free_space < 0)
  {
    MPRINTF("Error: YFFS2 filesystem not mounted. Please initialize first.\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  _Print_YAFFS2_info();

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
          int fd = yaffs_open(full_filename, O_RDONLY, 0);
          if (fd < 0)
          {
            MPRINTF("Failed to open file: %s\n\r", full_filename);
          }
          else
          {
            // Get file size using lseek
            yaffs_lseek(fd, 0, SEEK_END);
            int file_size = yaffs_lseek(fd, 0, SEEK_CUR);
            yaffs_lseek(fd, 0, SEEK_SET);

            if (file_size >= 0)
            {
              MPRINTF("File size: %d bytes\n\r", file_size);

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
                  // Read file in blocks using YAFFS2_TEST_BUFFER_SIZE
                  uint32_t block_size       = YAFFS2_TEST_BUFFER_SIZE;
                  uint32_t total_bytes_read = 0;
                  uint32_t current_offset   = 0;

                  // Read file block by block
                  while (current_offset < (uint32_t)file_size)
                  {
                    // Calculate bytes to read for this block
                    uint32_t bytes_to_read = block_size;
                    if (current_offset + bytes_to_read > (uint32_t)file_size)
                    {
                      bytes_to_read = (uint32_t)file_size - current_offset;
                    }

                    // Seek to current position
                    int seek_result = yaffs_lseek(fd, current_offset, SEEK_SET);
                    if (seek_result < 0)
                    {
                      MPRINTF("Failed to seek to offset %lu\n\r", current_offset);
                      break;
                    }

                    // Read one block
                    int actual_bytes_read = yaffs_read(fd, g_test_buffer, bytes_to_read);
                    if (actual_bytes_read < 0)
                    {
                      MPRINTF("Failed to read file at offset %lu\n\r", current_offset);
                      break;
                    }

                    if (actual_bytes_read == 0)
                    {
                      // End of file reached
                      break;
                    }

                    // Display this block as HEX dump
                    MPRINTF("Block at offset %lu (%d bytes):\n\r", current_offset, actual_bytes_read);
                    VT100_print_dump(current_offset, g_test_buffer, actual_bytes_read);

                    current_offset += actual_bytes_read;
                    total_bytes_read += actual_bytes_read;

                    // Show progress for large files
                    if ((uint32_t)file_size > block_size)
                    {
                      uint32_t progress_percent = (current_offset * 100) / (uint32_t)file_size;
                      MPRINTF("Progress: %lu%% (%lu/%d bytes)\n\r",
                              progress_percent, current_offset, file_size);
                    }
                  }

                  MPRINTF("Successfully read %lu bytes total\n\r", total_bytes_read);
                }
              }
            }
            else
            {
              MPRINTF("Failed to get file size\n\r");
            }

            // Close file
            yaffs_close(fd);
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
void Do_YAFFS2_performance_test(uint8_t keycode)
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

    // Check if filesystem is mounted by checking free space
    Y_LOFF_T free_space = yaffs_freespace("/");
    if (free_space < 0)
    {
      MPRINTF("\nERROR: YFFS2 filesystem not mounted!\n\r");
      MPRINTF("Please initialize YFFS2 first from the main menu.\n\r");
      MPRINTF("\nPress any key to return...\n\r");
      WAIT_CHAR(&choice, ms_to_ticks(100000));
      return;
    }

    _Print_YAFFS2_info();
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
  Description: List contents of lost+found directory

  Parameters: keycode - key code from menu

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_YAFFS2_list_lost_found(uint8_t keycode)
{
  GET_MCBL;
  yaffs_DIR           *dir_ptr;
  struct yaffs_dirent *entry;
  const char          *lost_found_path = "/lost+found";
  int                 file_count = 0;

  FSP_PARAMETER_NOT_USED(keycode);

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== YAFFS2 Lost+Found Directory ===\n\r");

  // Check if filesystem is mounted by checking free space
  Y_LOFF_T free_space = yaffs_freespace("/");
  if (free_space < 0)
  {
    MPRINTF("Error: YAFFS2 filesystem not mounted. Please initialize first.\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Scanning lost+found directory: %s\n\r", lost_found_path);

  // Open lost+found directory
  dir_ptr = yaffs_opendir(lost_found_path);
  if (dir_ptr == NULL)
  {
    MPRINTF("Failed to open lost+found directory: %s\n\r", _Get_YAFFS2_error_description(yaffs_get_error()));
    MPRINTF("This could mean:\n\r");
    MPRINTF("- Directory doesn't exist (normal for new filesystem)\n\r");
    MPRINTF("- Filesystem is corrupted\n\r");
    MPRINTF("- Insufficient permissions\n\r");
    MPRINTF("\nPress any key to continue...\n\r");
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("\nContents of lost+found directory:\n\r");
  MPRINTF("==================================\n\r");

  // Read directory entries
  while ((entry = yaffs_readdir(dir_ptr)) != NULL)
  {
    // Skip "." and ".." entries
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
    {
      // Get file/directory information
      char full_path[MAX_PATH_LENGTH];
      snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", lost_found_path, entry->d_name);

      struct yaffs_stat stat_buf;
      if (yaffs_lstat(full_path, &stat_buf) >= 0)
      {
        if (S_ISDIR(stat_buf.st_mode))
        {
          MPRINTF("[DIR]  %s/\n\r", entry->d_name);
        }
        else
        {
          // Convert time to readable format (simplified)
          time_t mod_time = stat_buf.yst_mtime;
          struct tm *time_info = localtime(&mod_time);

          MPRINTF("[FILE] %s (%lu bytes) %02d/%02d/%04d %02d:%02d:%02d\n\r",
                  entry->d_name, (uint32_t)stat_buf.st_size,
                  time_info->tm_mon + 1, time_info->tm_mday, time_info->tm_year + 1900,
                  time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
        }
        file_count++;
      }
      else
      {
        MPRINTF("[UNKNOWN] %s (stat failed)\n\r", entry->d_name);
        file_count++;
      }
    }
  }

  yaffs_closedir(dir_ptr);

  if (file_count == 0)
  {
    MPRINTF("Directory is empty (no lost files)\n\r");
    MPRINTF("\nThis is normal and indicates:\n\r");
    MPRINTF("- Filesystem is healthy\n\r");
    MPRINTF("- No corrupted files were found during last mount\n\r");

    // Get device info to check auto-cleanup setting
    struct yaffs_dev *dev = yaffs_getdev("/");
    if (dev && dev->param.empty_lost_n_found)
    {
      MPRINTF("- Auto-cleanup is enabled (empty_lost_n_found=1)\n\r");
    }
  }
  else
  {
    MPRINTF("\nTotal files/directories found: %d\n\r", file_count);
    MPRINTF("\nWARNING: Files in lost+found may indicate:\n\r");
    MPRINTF("- Previous filesystem corruption\n\r");
    MPRINTF("- Interrupted write operations\n\r");
    MPRINTF("- Power loss during file operations\n\r");
    MPRINTF("\nYou may want to examine these files and restore them manually.\n\r");
  }

  MPRINTF("\nPress any key to continue...\n\r");
  uint8_t key;
  WAIT_CHAR(&key, ms_to_ticks(100000));
}
