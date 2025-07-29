#include "App.h"
#include "Performance_Stats.h"
#include "Test_Patterns.h"

#define MAX_PATH_LENGTH              256
#define MAX_DIRS_IN_STACK            32

// Performance test configuration
#define LFS_TEST_FILES_COUNT_DEFAULT 10      // Default number of files
#define LFS_TEST_FILE_SIZE_DEFAULT   (10*1024) // Default file size (10 KB)
#define LFS_TEST_BLOCK_SIZE_DEFAULT  (64*1024)// Default block size (64 KB)
#define LFS_TEST_FILE_PREFIX         "test_"  // File name prefix
#define LFS_MAX_FILENAME_LENGTH      64       // Maximum filename length

// Data integrity definitions
#define CRC32_SIZE                   4     // CRC32 size in bytes

// Performance test parameters (configurable)
static uint32_t g_test_files_count         = LFS_TEST_FILES_COUNT_DEFAULT;
static uint32_t g_test_file_size           = LFS_TEST_FILE_SIZE_DEFAULT;
static uint32_t g_test_block_size          = LFS_TEST_BLOCK_SIZE_DEFAULT;
static uint32_t g_data_pattern             = DATA_PATTERN_CONSTANT;
static uint32_t g_fill_constant            = DEFAULT_FILL_CONSTANT;
static bool     g_enable_data_verification = true;

// Structure for directory stack (non-recursive tree traversal)
typedef struct
{
  char    path[MAX_PATH_LENGTH];
  uint8_t depth;
} T_dir_stack_item;

static T_dir_stack_item g_dir_stack[MAX_DIRS_IN_STACK];
static uint8_t          g_stack_top;

// External global LittleFS context
extern T_littlefs_context g_littlefs_context;

// Function declarations
void Do_LittleFS_init(uint8_t keycode);
void Do_LittleFS_list_files(uint8_t keycode);
void Do_LittleFS_performance_test(uint8_t keycode);

const T_VT100_Menu_item MENU_LittleFS_items[] = {
  { '1', Do_LittleFS_init, NULL },
  { '2', Do_LittleFS_list_files, NULL },
  { '3', Do_LittleFS_performance_test, NULL },
  { 'R', NULL, NULL },
  { 0 }  // End of menu
};

const T_VT100_Menu MENU_LittleFS = {
  "LittleFS Manager",
  "\033[5C LittleFS file system management menu\r\n"
  "\033[5C <1> - Initialize LittleFS (auto-format if needed)\r\n"
  "\033[5C <2> - List files (with option to read file as HEX dump)\r\n"
  "\033[5C <3> - Performance test\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_LittleFS_items
};

/*-----------------------------------------------------------------------------------------------------
  Description: Helper function to push directory to stack

  Parameters: path - directory path, depth - depth level

  Return: true if pushed successfully, false if stack full
-----------------------------------------------------------------------------------------------------*/
static bool _Push_dir_to_stack(const char *path, uint8_t depth)
{
  if (g_stack_top >= MAX_DIRS_IN_STACK)
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
  Description: Print operation statistics

  Parameters: operation_name - name of operation, stats - statistics to print

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_stats(const char *operation_name, T_performance_stats *stats)
{
  Performance_stats_print(operation_name, stats, g_enable_data_verification);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Convert LittleFS error code to text description

  Parameters: error - LittleFS error code

  Return: pointer to error description string
-----------------------------------------------------------------------------------------------------*/
static const char *_Littlefs_error_to_string(int error)
{
  switch (error)
  {
    case LFS_ERR_OK:
      return "LFS_ERR_OK: No error";
    case LFS_ERR_IO:
      return "LFS_ERR_IO: Error during device operation";
    case LFS_ERR_CORRUPT:
      return "LFS_ERR_CORRUPT: Corrupted";
    case LFS_ERR_NOENT:
      return "LFS_ERR_NOENT: No directory entry";
    case LFS_ERR_EXIST:
      return "LFS_ERR_EXIST: Entry already exists";
    case LFS_ERR_NOTDIR:
      return "LFS_ERR_NOTDIR: Entry is not a dir";
    case LFS_ERR_ISDIR:
      return "LFS_ERR_ISDIR: Entry is a dir";
    case LFS_ERR_NOTEMPTY:
      return "LFS_ERR_NOTEMPTY: Dir is not empty";
    case LFS_ERR_BADF:
      return "LFS_ERR_BADF: Bad file number";
    case LFS_ERR_FBIG:
      return "LFS_ERR_FBIG: File too large";
    case LFS_ERR_INVAL:
      return "LFS_ERR_INVAL: Invalid parameter";
    case LFS_ERR_NOSPC:
      return "LFS_ERR_NOSPC: No space left on device";
    case LFS_ERR_NOMEM:
      return "LFS_ERR_NOMEM: No more memory available";
    case LFS_ERR_NOATTR:
      return "LFS_ERR_NOATTR: No data/attr available";
    case LFS_ERR_NAMETOOLONG:
      return "LFS_ERR_NAMETOOLONG: File name too long";
    default:
      return "Unknown LittleFS error";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print LittleFS filesystem information

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_littlefs_info(void)
{
  GET_MCBL;
  struct lfs_fsinfo fsinfo;
  int result;

  MPRINTF("\n=== LittleFS Media Information ===\n\r");

  // Check if filesystem is mounted
  if (!Littlefs_is_mounted())
  {
    MPRINTF("Error: Filesystem not mounted. Please initialize first.\n\r");
    return;
  }

  // Get filesystem statistics
  result = lfs_fs_stat(&g_littlefs_context.lfs, &fsinfo);
  if (result == 0)
  {
    uint32_t total_size = fsinfo.block_count * fsinfo.block_size;

    // Get actual used size
    lfs_size_t used_blocks = lfs_fs_size(&g_littlefs_context.lfs);
    uint32_t   used_size   = used_blocks * fsinfo.block_size;
    uint32_t   free_size   = total_size - used_size;

    MPRINTF("Total blocks         : %lu\n\r", fsinfo.block_count);
    MPRINTF("Block size           : %lu bytes\n\r", fsinfo.block_size);
    MPRINTF("Used blocks          : %lu\n\r", (uint32_t)used_blocks);
    MPRINTF("Free blocks          : %lu\n\r", fsinfo.block_count - (uint32_t)used_blocks);
    MPRINTF("Total space          : %lu KB (%lu MB)\n\r", total_size / 1024, total_size / (1024 * 1024));
    MPRINTF("Used space           : %lu KB (%lu MB)\n\r", used_size / 1024, used_size / (1024 * 1024));
    MPRINTF("Available space      : %lu KB (%lu MB)\n\r", free_size / 1024, free_size / (1024 * 1024));

    // Calculate and display usage percentage
    if (total_size > 0)
    {
      uint32_t usage_percent = (used_size * 100) / total_size;
      MPRINTF("Usage                : %lu%% used, %lu%% free\n\r", usage_percent, 100 - usage_percent);
    }
  }
  else
  {
    MPRINTF("Error getting filesystem information: %s\n\r", _Littlefs_error_to_string(result));
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize LittleFS filesystem (performs automatic formatting if mount fails)

  Parameters: keycode - input key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_init(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS Initialization ===\n\r");

  // Display filesystem configuration parameters
  MPRINTF("\nFilesystem configuration:\n\r");
  MPRINTF("  Block size:      %u bytes (%.1f KB)\n\r", LITTLEFS_BLOCK_SIZE, (float)LITTLEFS_BLOCK_SIZE / 1024.0f);
  MPRINTF("  Block count:     %u blocks\n\r", LITTLEFS_BLOCK_COUNT);
  MPRINTF("  Total capacity:  %u KB (%.1f MB)\n\r",
          (LITTLEFS_BLOCK_SIZE * LITTLEFS_BLOCK_COUNT) / 1024,
          (float)(LITTLEFS_BLOCK_SIZE * LITTLEFS_BLOCK_COUNT) / (1024.0f * 1024.0f));
  MPRINTF("  Cache size:      %u bytes\n\r", LITTLEFS_CACHE_SIZE);
  MPRINTF("  Lookahead size:  %u bytes\n\r", LITTLEFS_LOOKAHEAD_SIZE);
  MPRINTF("  Block cycles:    %u\n\r", LITTLEFS_BLOCK_CYCLES);
  MPRINTF("  Read size:       %u bytes\n\r", LITTLEFS_READ_SIZE);
  MPRINTF("  Program size:    %u bytes\n\r", LITTLEFS_PROG_SIZE);
  MPRINTF("\n\r");

  // Initialize LittleFS configuration
  LITTLEFS_DEBUG_PRINTF("Starting LittleFS initialization\n\r");
  int result = Littlefs_initialize();
  if (result != 0)
  {
    MPRINTF("LittleFS initialization failed: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  // Try to mount the filesystem
  LITTLEFS_DEBUG_PRINTF("Attempting to mount filesystem\n\r");
  result = Littlefs_mount();
  if (result != 0)
  {
    MPRINTF("Mount failed: %s\n\r", _Littlefs_error_to_string(result));
    MPRINTF("Trying to format...\n\r");

    // Format the filesystem if mount fails
    LITTLEFS_DEBUG_PRINTF("Formatting filesystem due to mount failure\n\r");
    result = Littlefs_format();
    if (result != 0)
    {
      MPRINTF("LittleFS format failed: %s\n\r", _Littlefs_error_to_string(result));
      goto exit;
    }

    // Try to mount again after format
    LITTLEFS_DEBUG_PRINTF("Attempting to mount filesystem after format\n\r");
    result = Littlefs_mount();
    if (result != 0)
    {
      MPRINTF("LittleFS mount failed after format: %s\n\r", _Littlefs_error_to_string(result));
      goto exit;
    }

    MPRINTF("Filesystem formatted and mounted successfully\n\r");
  }
  else
  {
    MPRINTF("LittleFS mounted successfully\n\r");
  }

exit:
  if (result == 0)
  {
    MPRINTF("LittleFS initialized successfully\n\r");
  }
  else
  {
    MPRINTF("LittleFS initialization failed: %s\n\r", _Littlefs_error_to_string(result));
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: List all files in the LittleFS filesystem in tree format

  Parameters: keycode - input key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_list_files(uint8_t keycode)
{
  GET_MCBL;
  lfs_dir_t       dir;
  struct lfs_info info;
  int             result;
  int             total_files = 0;
  int             total_dirs  = 0;
  char            current_path[MAX_PATH_LENGTH];
  uint8_t         current_depth;

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS Directory Tree ===\n\r");

  // Check if filesystem is mounted
  if (!Littlefs_is_mounted())
  {
    MPRINTF("Filesystem not mounted. Please initialize first.\n\r");
    goto exit;
  }

  // Initialize stack and start with root directory
  g_stack_top = 0;
  _Push_dir_to_stack("/", 0);

  MPRINTF("Root directory tree:\n\r");
  MPRINTF("/\n\r");

  // Process directories using stack (non-recursive)
  while (_Pop_dir_from_stack(current_path, &current_depth))
  {
    // Open current directory
    result = lfs_dir_open(&g_littlefs_context.lfs, &dir, current_path);
    if (result < 0)
    {
      MPRINTF("Failed to open directory %s: %s\n\r", current_path, _Littlefs_error_to_string(result));
      continue;
    }

    // Read directory entries
    while (true)
    {
      result = lfs_dir_read(&g_littlefs_context.lfs, &dir, &info);
      if (result < 0)
      {
        MPRINTF("Failed to read directory %s: %s\n\r", current_path, _Littlefs_error_to_string(result));
        break;
      }

      // End of directory
      if (result == 0)
      {
        break;
      }

      // Skip "." and ".." entries
      if (info.name[0] == '.')
      {
        continue;
      }

      // Print tree indentation
      _Print_tree_indent(current_depth + 1);

      if (info.type == LFS_TYPE_REG)
      {
        // Format file size with appropriate units (bytes and kilobytes only)
        if (info.size >= 1024)
        {
          MPRINTF("%s (%.2f KB)\n\r", info.name, (float)info.size / 1024.0f);
        }
        else
        {
          MPRINTF("%s (%u B)\n\r", info.name, info.size);
        }
        total_files++;
      }
      else if (info.type == LFS_TYPE_DIR)
      {
        MPRINTF("%s/\n\r", info.name);
        total_dirs++;

        // Add subdirectory to stack if not too deep and stack not full
        if (current_depth < 8 && g_stack_top < MAX_DIRS_IN_STACK - 1)
        {
          char subdir_path[MAX_PATH_LENGTH];
          if (strcmp(current_path, "/") == 0)
          {
            snprintf(subdir_path, MAX_PATH_LENGTH, "/%s", info.name);
          }
          else
          {
            snprintf(subdir_path, MAX_PATH_LENGTH, "%s/%s", current_path, info.name);
          }
          _Push_dir_to_stack(subdir_path, current_depth + 1);
        }
      }
      else
      {
        MPRINTF("%s (UNKNOWN)\n\r", info.name);
      }
    }

    // Close directory
    lfs_dir_close(&g_littlefs_context.lfs, &dir);
  }

  MPRINTF("\nSummary: %d files, %d directories\n\r", total_files, total_dirs);

  // Show filesystem statistics
  struct lfs_fsinfo fsinfo;
  result = lfs_fs_stat(&g_littlefs_context.lfs, &fsinfo);
  if (result == 0)
  {
    uint32_t total_size    = fsinfo.block_count * fsinfo.block_size;

    // Get actual used size
    lfs_size_t used_blocks = lfs_fs_size(&g_littlefs_context.lfs);
    uint32_t   used_size   = used_blocks * fsinfo.block_size;
    uint32_t   free_size   = total_size - used_size;

    MPRINTF("\nFilesystem statistics:\n\r");
    MPRINTF("  Total space: %u KB (%u blocks x %u bytes)\n\r",
            total_size / 1024, fsinfo.block_count, fsinfo.block_size);
    MPRINTF("  Used space:  %u KB (%u blocks)\n\r", used_size / 1024, (uint32_t)used_blocks);
    MPRINTF("  Free space:  %u KB\n\r", free_size / 1024);
  }

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
        char filename[LFS_MAX_FILENAME_LENGTH];
        if (VT100_input_filename(filename, LFS_MAX_FILENAME_LENGTH, "test_001.bin"))
        {
          // Add leading slash if not present
          char full_filename[LFS_MAX_FILENAME_LENGTH + 1];
          if (filename[0] != '/')
          {
            snprintf(full_filename, sizeof(full_filename), "/%s", filename);
          }
          else
          {
            strncpy(full_filename, filename, sizeof(full_filename) - 1);
            full_filename[sizeof(full_filename) - 1] = '\0';
          }

          MPRINTF("Opening file: %s\n\r", full_filename);

          // Open file for reading
          lfs_file_t file;
          int        result = lfs_file_open(&g_littlefs_context.lfs, &file, full_filename, LFS_O_RDONLY);
          if (result < 0)
          {
            MPRINTF("Failed to open file: %s\n\r", _Littlefs_error_to_string(result));
          }
          else
          {
            // Get file size
            uint32_t file_size = lfs_file_size(&g_littlefs_context.lfs, &file);
            MPRINTF("File size: %u bytes\n\r", file_size);

            if (file_size == 0)
            {
              MPRINTF("File is empty\n\r");
            }
            else
            {
              // Read file in blocks using LFS_TEST_BLOCK_SIZE_DEFAULT
              uint32_t block_size       = LFS_TEST_BLOCK_SIZE_DEFAULT;
              uint32_t total_bytes_read = 0;
              uint32_t current_offset   = 0;

              // Allocate buffer for one block
              uint8_t *buffer           = (uint8_t *)App_malloc(block_size);
              if (buffer == NULL)
              {
                MPRINTF("Memory allocation failed\n\r");
              }
              else
              {
                // Read file block by block
                while (current_offset < file_size)
                {
                  // Calculate bytes to read for this block
                  uint32_t bytes_to_read = block_size;
                  if (current_offset + bytes_to_read > file_size)
                  {
                    bytes_to_read = file_size - current_offset;
                  }

                  // Read one block
                  int32_t bytes_read = lfs_file_read(&g_littlefs_context.lfs, &file, buffer, bytes_to_read);
                  if (bytes_read < 0)
                  {
                    MPRINTF("Failed to read file at offset %u: %s\n\r",
                            current_offset, _Littlefs_error_to_string((int)bytes_read));
                    break;
                  }

                  if (bytes_read == 0)
                  {
                    // End of file reached
                    break;
                  }

                  // Display this block as HEX dump
                  MPRINTF("Block at offset %u (%d bytes):\n\r", current_offset, bytes_read);
                  VT100_print_dump(current_offset, buffer, (uint32_t)bytes_read);

                  current_offset += bytes_read;
                  total_bytes_read += bytes_read;

                  // Show progress for large files
                  if (file_size > block_size)
                  {
                    uint32_t progress_percent = (current_offset * 100) / file_size;
                    MPRINTF("Progress: %u%% (%u/%u bytes)\n\r",
                            progress_percent, current_offset, file_size);
                  }
                }

                MPRINTF("Successfully read %u bytes total\n\r", total_bytes_read);

                // Free buffer
                App_free(buffer);
              }
            }

            // Close file
            lfs_file_close(&g_littlefs_context.lfs, &file);
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

exit:
  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Check filesystem consistency before tests

  Parameters: none

  Return: true if filesystem is OK, false if corrupted
-----------------------------------------------------------------------------------------------------*/
static bool _Check_filesystem_integrity(void)
{
  GET_MCBL;
  int result;

  // Try to read root directory
  lfs_dir_t dir;
  result = lfs_dir_open(&g_littlefs_context.lfs, &dir, "/");
  if (result < 0)
  {
    MPRINTF("Filesystem integrity check FAILED: Cannot open root directory - %s\n\r",
            _Littlefs_error_to_string(result));
    return false;
  }

  lfs_dir_close(&g_littlefs_context.lfs, &dir);

  // Try to get filesystem statistics
  struct lfs_fsinfo fsinfo;
  result = lfs_fs_stat(&g_littlefs_context.lfs, &fsinfo);
  if (result < 0)
  {
    MPRINTF("Filesystem integrity check FAILED: Cannot get filesystem statistics - %s\n\r",
            _Littlefs_error_to_string(result));
    return false;
  }

  MPRINTF("Filesystem integrity check: OK\n\r");
  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Write test files with performance measurement

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_write_test(void)
{
  GET_MCBL;
  uint8_t          *buffer = NULL;
  T_performance_stats stats;
  char              filename[LFS_MAX_FILENAME_LENGTH];
  lfs_file_t        file;
  int               result;
  uint32_t          open_time, close_time, io_time;
  uint32_t          bytes_written;
  uint32_t          blocks_per_file;
  uint32_t          operation_time;
  uint32_t          speed_kbps;
  uint32_t          data_size;  // Data size without CRC
  uint32_t          crc32_value;

  MPRINTF("=== Write Test ===\n\r");

  // Check filesystem integrity first
  if (!_Check_filesystem_integrity())
  {
    MPRINTF("Aborting write test due to filesystem integrity failure\n\r");
    return;
  }

  MPRINTF("Writing %u files, %u bytes each, %u byte blocks\n\r",
          g_test_files_count, g_test_file_size, g_test_block_size);
  MPRINTF("Data pattern: %s", Test_patterns_get_name(g_data_pattern));
  if (g_data_pattern == DATA_PATTERN_CONSTANT)
  {
    MPRINTF(" (0x%02X)", g_fill_constant);
  }
  MPRINTF(", Verification: %s\n\r", g_enable_data_verification ? "ON" : "OFF");

  // Calculate data size (file size minus CRC32)
  data_size              = g_test_file_size >= CRC32_SIZE ? g_test_file_size - CRC32_SIZE : g_test_file_size;

  // Initialize statistics
  Performance_stats_init(&stats);

  // Allocate memory for buffer
  buffer                 = (uint8_t *)App_malloc(g_test_block_size);
  if (buffer == NULL)
  {
    MPRINTF("Memory allocation failed\n\r");
    return;
  }

  blocks_per_file = (data_size + g_test_block_size - 1) / g_test_block_size;

  // Write files
  for (uint32_t file_idx = 0; file_idx < g_test_files_count; file_idx++)
  {
    snprintf(filename, LFS_MAX_FILENAME_LENGTH, "/%s%03u.bin", LFS_TEST_FILE_PREFIX, file_idx + 1);
    MPRINTF("Writing %s... ", filename);

    T_sys_timestump start_ts, end_ts, open_start_ts, open_end_ts, close_start_ts, close_end_ts;

    Get_hw_timestump(&start_ts);

    // Open file with timing
    LITTLEFS_DEBUG_PRINTF("Opening file: %s\n\r", filename);
    Get_hw_timestump(&open_start_ts);
    result = lfs_file_open(&g_littlefs_context.lfs, &file, filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    Get_hw_timestump(&open_end_ts);
    open_time = Timestump_diff_to_usec(&open_start_ts, &open_end_ts);

    if (result < 0)
    {
      Get_hw_timestump(&end_ts);
      operation_time = Timestump_diff_to_usec(&start_ts, &end_ts);
      MPRINTF("FAILED (open): %s (open: %5u us, total: %6u us)\n\r",
              _Littlefs_error_to_string(result), open_time, operation_time);
      Performance_stats_update_error(&stats);
      continue;
    }

    MPRINTF("opened: %5u us, ", open_time);

    // Initialize CRC calculation
    uint32_t crc = 0xFFFFFFFF;

    // Initialize I/O time measurement
    io_time      = 0;
    T_sys_timestump io_start_ts, io_end_ts;

    // Write file data in blocks
    bytes_written = 0;
    for (uint32_t block = 0; block < blocks_per_file; block++)
    {
      uint32_t bytes_to_write = g_test_block_size;
      if (bytes_written + bytes_to_write > data_size)
      {
        bytes_to_write = data_size - bytes_written;
      }

      // Fill buffer with pattern
      Test_patterns_fill_buffer(buffer, bytes_to_write, g_data_pattern, g_fill_constant, bytes_written);

      // Update CRC with this block
      if (g_enable_data_verification)
      {
        crc = CRC32_IEEE802_3(crc, buffer, bytes_to_write);
      }

      LITTLEFS_DEBUG_PRINTF("Writing block %u: %u bytes to %s\n\r", block, bytes_to_write, filename);
      Get_hw_timestump(&io_start_ts);
      lfs_ssize_t written = lfs_file_write(&g_littlefs_context.lfs, &file, buffer, bytes_to_write);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);
      if (written < 0)
      {
        MPRINTF("FAILED (write block %u): %s\n\r", block, _Littlefs_error_to_string((int)written));
        lfs_file_close(&g_littlefs_context.lfs, &file);
        lfs_remove(&g_littlefs_context.lfs, filename);  // Remove corrupted file
        Get_hw_timestump(&end_ts);
        operation_time = Timestump_diff_to_usec(&start_ts, &end_ts);
        Performance_stats_update_error(&stats);
        goto next_file;
      }
      bytes_written += written;

      if (bytes_written >= data_size)
      {
        break;
      }
    }

    // Write CRC32 at the end of file
    if (g_enable_data_verification && g_test_file_size >= CRC32_SIZE)
    {
      crc32_value = ~crc;
      Get_hw_timestump(&io_start_ts);
      lfs_ssize_t written = lfs_file_write(&g_littlefs_context.lfs, &file, &crc32_value, CRC32_SIZE);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);
      if (written < 0)
      {
        MPRINTF("FAILED (write CRC): %s\n\r", _Littlefs_error_to_string((int)written));
        lfs_file_close(&g_littlefs_context.lfs, &file);
        lfs_remove(&g_littlefs_context.lfs, filename);  // Remove corrupted file
        Get_hw_timestump(&end_ts);
        operation_time = Timestump_diff_to_usec(&start_ts, &end_ts);
        Performance_stats_update_error(&stats);
        goto next_file;
      }
      bytes_written += written;
    }

    // Close file with timing
    LITTLEFS_DEBUG_PRINTF("Closing file: %s\n\r", filename);
    Get_hw_timestump(&close_start_ts);
    result = lfs_file_close(&g_littlefs_context.lfs, &file);
    Get_hw_timestump(&close_end_ts);
    close_time = Timestump_diff_to_usec(&close_start_ts, &close_end_ts);
    Get_hw_timestump(&end_ts);
    operation_time = Timestump_diff_to_usec(&start_ts, &end_ts);

    if (result < 0)
    {
      MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r",
              _Littlefs_error_to_string(result), close_time, operation_time);
      Performance_stats_update_error(&stats);
    }
    else if (bytes_written == g_test_file_size)
    {
      // Calculate speed in KB/s based on I/O time (avoid division by zero)
      // Using binary KB (1 KB = 1024 bytes) for speed calculation with floating point precision
      if (io_time > 0)
      {
        speed_kbps = (uint32_t)((float)g_test_file_size * 1000000.0f / ((float)io_time * 1024.0f));
      }
      else
      {
        speed_kbps = 0;
      }

      MPRINTF("closed: %5u us, I/O: %6u us, total: %6u us, speed: %5u KB/s", close_time, io_time, operation_time, speed_kbps);
      if (g_enable_data_verification)
      {
        MPRINTF(", CRC: 0x%08X", crc32_value);
      }
      MPRINTF("\n\r");

      // Update all statistics using common function
      Performance_stats_update_success(&stats, operation_time, open_time, close_time, io_time, g_test_file_size, speed_kbps);
    }
    else
    {
      MPRINTF("FAILED (partial write: %u/%u bytes, close: %5u us, total: %6u us)\n\r",
              bytes_written, g_test_file_size, close_time, operation_time);
      Performance_stats_update_error(&stats);
    }

  next_file:
    continue;
  }

  // Finalize statistics calculations
  Performance_stats_finalize(&stats);

  Performance_stats_print("Write Test", &stats, g_enable_data_verification);

  App_free(buffer);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Read test files with performance measurement

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_read_test(void)
{
  GET_MCBL;
  uint8_t          *buffer = NULL;
  T_performance_stats stats;
  char              filename[LFS_MAX_FILENAME_LENGTH];
  lfs_file_t        file;
  int               result;
  uint32_t          open_time, close_time, io_time;
  uint32_t          bytes_read;
  uint32_t          blocks_per_file;
  uint32_t          operation_time;
  uint32_t          speed_kbps;
  uint32_t          data_size;  // Data size without CRC
  uint32_t          file_crc32, calculated_crc32;
  bool              crc_valid     = true;
  bool              pattern_valid = true;
  bool              size_valid    = true;

  MPRINTF("=== Read Test ===\n\r");

  // Check filesystem integrity first
  if (!_Check_filesystem_integrity())
  {
    MPRINTF("Aborting read test due to filesystem integrity failure\n\r");
    return;
  }

  MPRINTF("Reading %u files, %u byte blocks\n\r", g_test_files_count, g_test_block_size);
  MPRINTF("Data pattern: %s", Test_patterns_get_name(g_data_pattern));
  if (g_data_pattern == DATA_PATTERN_CONSTANT)
  {
    MPRINTF(" (0x%02X)", g_fill_constant);
  }
  MPRINTF(", Verification: %s\n\r", g_enable_data_verification ? "ON" : "OFF");

  // Calculate data size (file size minus CRC32)
  data_size              = g_test_file_size >= CRC32_SIZE ? g_test_file_size - CRC32_SIZE : g_test_file_size;

  // Initialize statistics
  Performance_stats_init(&stats);

  // Allocate memory for buffer
  buffer                 = (uint8_t *)App_malloc(g_test_block_size);
  if (buffer == NULL)
  {
    MPRINTF("Memory allocation failed\n\r");
    return;
  }

  blocks_per_file = (data_size + g_test_block_size - 1) / g_test_block_size;

  // Read files
  for (uint32_t file_idx = 0; file_idx < g_test_files_count; file_idx++)
  {
    snprintf(filename, LFS_MAX_FILENAME_LENGTH, "/%s%03u.bin", LFS_TEST_FILE_PREFIX, file_idx + 1);
    MPRINTF("Reading %s... ", filename);

    T_sys_timestump start_ts, end_ts, open_start_ts, open_end_ts, close_start_ts, close_end_ts;

    Get_hw_timestump(&start_ts);
    crc_valid     = true;
    pattern_valid = true;
    size_valid    = true;

    // Open file with timing
    LITTLEFS_DEBUG_PRINTF("Opening file for read: %s\n\r", filename);
    Get_hw_timestump(&open_start_ts);
    result = lfs_file_open(&g_littlefs_context.lfs, &file, filename, LFS_O_RDONLY);
    Get_hw_timestump(&open_end_ts);
    open_time = Timestump_diff_to_usec(&open_start_ts, &open_end_ts);

    if (result < 0)
    {
      Get_hw_timestump(&end_ts);
      operation_time = Timestump_diff_to_usec(&start_ts, &end_ts);
      MPRINTF("FAILED (open): %s (open: %5u us, total: %6u us)\n\r",
              _Littlefs_error_to_string(result), open_time, operation_time);
      Performance_stats_update_error(&stats);
      continue;
    }

    MPRINTF("opened: %5u us, ", open_time);

    // Initialize CRC calculation
    uint32_t crc = 0xFFFFFFFF;

    // Initialize I/O time measurement
    io_time      = 0;
    T_sys_timestump io_start_ts, io_end_ts;

    // Read file data in blocks
    bytes_read = 0;
    for (uint32_t block = 0; block < blocks_per_file; block++)
    {
      uint32_t bytes_to_read = g_test_block_size;
      if (bytes_read + bytes_to_read > data_size)
      {
        bytes_to_read = data_size - bytes_read;
      }

      LITTLEFS_DEBUG_PRINTF("Reading block %u: %u bytes from %s\n\r", block, bytes_to_read, filename);
      Get_hw_timestump(&io_start_ts);
      lfs_ssize_t read_result = lfs_file_read(&g_littlefs_context.lfs, &file, buffer, bytes_to_read);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);
      if (read_result < 0)
      {
        MPRINTF("FAILED (read block %u): %s\n\r", block, _Littlefs_error_to_string((int)read_result));
        lfs_file_close(&g_littlefs_context.lfs, &file);
        Get_hw_timestump(&end_ts);
        operation_time = Timestump_diff_to_usec(&start_ts, &end_ts);
        Performance_stats_update_error(&stats);
        goto next_file;
      }

      if (read_result == 0)
      {
        // End of file reached
        break;
      }

      // Verify data pattern if enabled
      if (g_enable_data_verification && pattern_valid)
      {
        if (!Test_patterns_verify_buffer(buffer, read_result, g_data_pattern, g_fill_constant, bytes_read))
        {
          pattern_valid = false;
          Performance_stats_increment_pattern_error(&stats);
        }
      }

      // Update CRC with this block
      if (g_enable_data_verification)
      {
        crc = CRC32_IEEE802_3(crc, buffer, read_result);
      }

      bytes_read += read_result;

      if (bytes_read >= data_size)
      {
        break;
      }
    }

    // Read and verify CRC32 if enabled
    if (g_enable_data_verification && g_test_file_size >= CRC32_SIZE)
    {
      Get_hw_timestump(&io_start_ts);
      lfs_ssize_t crc_read = lfs_file_read(&g_littlefs_context.lfs, &file, &file_crc32, CRC32_SIZE);
      Get_hw_timestump(&io_end_ts);
      io_time += Timestump_diff_to_usec(&io_start_ts, &io_end_ts);
      if (crc_read == CRC32_SIZE)
      {
        calculated_crc32 = ~crc;
        if (file_crc32 != calculated_crc32)
        {
          crc_valid = false;
          Performance_stats_increment_crc_error(&stats);
        }
        bytes_read += crc_read;
      }
      else
      {
        crc_valid = false;
        Performance_stats_increment_crc_error(&stats);
      }
    }

    // Close file with timing
    LITTLEFS_DEBUG_PRINTF("Closing file after read: %s\n\r", filename);
    Get_hw_timestump(&close_start_ts);
    result = lfs_file_close(&g_littlefs_context.lfs, &file);
    Get_hw_timestump(&close_end_ts);
    close_time = Timestump_diff_to_usec(&close_start_ts, &close_end_ts);
    Get_hw_timestump(&end_ts);
    operation_time = Timestump_diff_to_usec(&start_ts, &end_ts);

    if (result < 0)
    {
      MPRINTF("FAILED (close): %s (close: %5u us, total: %6u us)\n\r",
              _Littlefs_error_to_string(result), close_time, operation_time);
      Performance_stats_update_error(&stats);
    }
    else
    {
      // Check file size
      if (bytes_read != g_test_file_size)
      {
        size_valid = false;
        Performance_stats_increment_size_error(&stats);
      }

      // Calculate speed in KB/s based on I/O time (avoid division by zero)
      // Using binary KB (1 KB = 1024 bytes) for speed calculation with floating point precision
      if (io_time > 0)
      {
        speed_kbps = (uint32_t)((float)bytes_read * 1000000.0f / ((float)io_time * 1024.0f));
      }
      else
      {
        speed_kbps = 0;
      }

      MPRINTF("closed: %5u us, size: %5u bytes, I/O: %6u us, total: %6u us, speed: %5u KB/s",
              close_time, bytes_read, io_time, operation_time, speed_kbps);

      if (g_enable_data_verification)
      {
        MPRINTF(", CRC: %s", crc_valid ? "OK" : "ERROR");
        MPRINTF(", Pattern: %s", pattern_valid ? "OK" : "ERROR");
        MPRINTF(", Size: %s", size_valid ? "OK" : "ERROR");
        if (crc_valid && g_test_file_size >= CRC32_SIZE)
        {
          MPRINTF(" (0x%08X)", file_crc32);
        }
      }
      MPRINTF("\n\r");

      // Update all statistics using common function
      Performance_stats_update_success(&stats, operation_time, open_time, close_time, io_time, bytes_read, speed_kbps);
    }

  next_file:
    continue;
  }

  // Calculate averages using common function
  Performance_stats_finalize(&stats);

  _Print_stats("Read Test", &stats);

  App_free(buffer);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Delete test files with performance measurement

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_delete_test(void)
{
  GET_MCBL;
  T_performance_stats stats;
  char              filename[LFS_MAX_FILENAME_LENGTH];
  int               result;
  uint32_t          operation_time;

  MPRINTF("=== Delete Test ===\n\r");
  MPRINTF("Deleting %u files\n\r", g_test_files_count);

  // Initialize statistics
  Performance_stats_init_delete(&stats);

  // Delete files
  for (uint32_t file_idx = 0; file_idx < g_test_files_count; file_idx++)
  {
    snprintf(filename, LFS_MAX_FILENAME_LENGTH, "/%s%03u.bin", LFS_TEST_FILE_PREFIX, file_idx + 1);
    MPRINTF("Deleting %s... ", filename);

    // Delete file
    LITTLEFS_DEBUG_PRINTF("Deleting file: %s\n\r", filename);
    T_sys_timestump start_ts, end_ts;
    Get_hw_timestump(&start_ts);
    result = lfs_remove(&g_littlefs_context.lfs, filename);
    Get_hw_timestump(&end_ts);
    operation_time = Timestump_diff_to_usec(&start_ts, &end_ts);

    if (result < 0)
    {
      MPRINTF("FAILED: %s (%6u us)\n\r", _Littlefs_error_to_string(result), operation_time);
      Performance_stats_update_error(&stats);
    }
    else
    {
      MPRINTF("deleted: %6u us\n\r", operation_time);

      // Update statistics with common function
      Performance_stats_update_delete_success(&stats, operation_time);
    }
  }

  // Calculate averages using common function
  Performance_stats_finalize(&stats);

  _Print_stats("Delete Test", &stats);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Format filesystem with performance measurement

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_format_test(void)
{
  GET_MCBL;
  int      result;
  uint32_t format_time;

  MPRINTF("=== Format Test ===\n\r");
  MPRINTF("Formatting filesystem...\n\r");

  // Unmount first if mounted (ensures clean state)
  if (Littlefs_is_mounted())
  {
    MPRINTF("Unmounting filesystem... ");
    Littlefs_unmount();  // Always succeeds - just resets internal state
    MPRINTF("OK\n\r");
  }

  // Format filesystem
  MPRINTF("Formatting... ");
  LITTLEFS_DEBUG_PRINTF("Starting filesystem format operation\n\r");
  T_sys_timestump start_ts, end_ts;
  Get_hw_timestump(&start_ts);
  result = Littlefs_format();
  Get_hw_timestump(&end_ts);
  format_time = Timestump_diff_to_usec(&start_ts, &end_ts);

  if (result != 0)
  {
    MPRINTF("FAILED: %s\n\r", _Littlefs_error_to_string(result));
    return;
  }

  MPRINTF("OK (%u us)\n\r", format_time);

  // Remount filesystem
  MPRINTF("Remounting... ");
  result = Littlefs_mount();
  if (result != 0)
  {
    MPRINTF("FAILED: %s\n\r", _Littlefs_error_to_string(result));
    return;
  }
  MPRINTF("OK\n\r");

  MPRINTF("\n=== Format Test Statistics ===\n\r");
  MPRINTF("Format time: %u us\n\r", format_time);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Run full test (write + read + delete)

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Do_full_test(void)
{
  GET_MCBL;
  uint32_t total_test_time;

  MPRINTF("=== Full Test (Write + Read + Delete) ===\n\r");
  MPRINTF("Configuration: %u files × %u bytes, %u byte blocks\n\r",
          g_test_files_count, g_test_file_size, g_test_block_size);

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
  total_test_time = Timestump_diff_to_usec(&total_start_ts, &total_end_ts);

  MPRINTF("\n=== Full Test Summary ===\n\r");
  MPRINTF("Total test duration: %u us\n\r", total_test_time);
  MPRINTF("Average time per file (all operations): %.1f us\n\r",
          (float)total_test_time / g_test_files_count);
}

/*-----------------------------------------------------------------------------------------------------
  Description: LittleFS performance test menu and operations

  Parameters: keycode - input key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_performance_test(uint8_t keycode)
{
  GET_MCBL;
  uint8_t choice;
  bool    exit_menu = false;

  while (!exit_menu)
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF("=== LittleFS Performance Test ===\n\r");

    // Check if filesystem is mounted
    if (!Littlefs_is_mounted())
    {
      MPRINTF("\nERROR: LittleFS not mounted!\n\r");
      MPRINTF("Please initialize LittleFS first from the main menu.\n\r");
      MPRINTF("\nPress any key to return...\n\r");
      WAIT_CHAR(&choice, ms_to_ticks(100000));
      return;
    }

    _Print_littlefs_info();

    MPRINTF("\n=== Test Configuration ===\n\r");
    MPRINTF("Files count      : %u\n\r", g_test_files_count);
    MPRINTF("File size        : %u bytes (%.1f KB)\n\r", g_test_file_size, (float)g_test_file_size / 1024.0f);
    MPRINTF("Block size       : %u bytes (%.1f KB)\n\r", g_test_block_size, (float)g_test_block_size / 1024.0f);
    MPRINTF("Data pattern     : %s", Test_patterns_get_name(g_data_pattern));
    if (g_data_pattern == DATA_PATTERN_CONSTANT)
    {
      MPRINTF(" (0x%02X)", g_fill_constant);
    }
    MPRINTF("\n\r");
    MPRINTF("Data verification: %s\n\r", g_enable_data_verification ? "Enabled" : "Disabled");

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
          if (!Littlefs_is_mounted())
          {
            MPRINTF("\n\rFilesystem not mounted. Please initialize first.\n\r");
          }
          else
          {
            _Do_write_test();
          }
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '2':
          if (!Littlefs_is_mounted())
          {
            MPRINTF("\n\rFilesystem not mounted. Please initialize first.\n\r");
          }
          else
          {
            _Do_read_test();
          }
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '3':
          if (!Littlefs_is_mounted())
          {
            MPRINTF("\n\rFilesystem not mounted. Please initialize first.\n\r");
          }
          else
          {
            _Do_delete_test();
          }
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '4':
          _Do_format_test();
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '5':
          if (!Littlefs_is_mounted())
          {
            MPRINTF("\n\rFilesystem not mounted. Please initialize first.\n\r");
          }
          else
          {
            _Do_full_test();
          }
          MPRINTF("\n\rPress any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '6':
          MPRINTF("\n\rEnter new files count (1-10000) [current: %u]: ", g_test_files_count);
          uint32_t new_files_count;
          if (VT100_input_uint32(&new_files_count, 1, 10000, g_test_files_count))
          {
            g_test_files_count = new_files_count;
            MPRINTF("Files count changed to %u\n\r", g_test_files_count);
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
            MPRINTF("File size changed to %u bytes (%.1f KB)\n\r", g_test_file_size, (float)g_test_file_size / 1024.0f);
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
            MPRINTF("Block size changed to %u bytes (%.1f KB)\n\r", g_test_block_size, (float)g_test_block_size / 1024.0f);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case '9':
          g_test_files_count         = LFS_TEST_FILES_COUNT_DEFAULT;
          g_test_file_size           = LFS_TEST_FILE_SIZE_DEFAULT;
          g_test_block_size          = LFS_TEST_BLOCK_SIZE_DEFAULT;
          g_data_pattern             = DATA_PATTERN_CONSTANT;
          g_fill_constant            = DEFAULT_FILL_CONSTANT;
          g_enable_data_verification = true;
          MPRINTF("\n\rParameters reset to defaults\n\r");
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case 'A':
        case 'a':
          MPRINTF("\n\rSelect data pattern:\n\r");
          MPRINTF("<1> - Constant pattern\n\r");
          MPRINTF("<2> - Counter pattern\n\r");
          MPRINTF("<3> - Random pattern\n\r");
          MPRINTF("Current: %s\n\r", Test_patterns_get_name(g_data_pattern));
          MPRINTF("Enter choice (1-3): ");
          uint8_t pattern_choice;
          if (WAIT_CHAR(&pattern_choice, ms_to_ticks(100000)) == RES_OK)
          {
            switch (pattern_choice)
            {
              case '1':
                g_data_pattern = DATA_PATTERN_CONSTANT;
                MPRINTF("\n\rPattern changed to: %s\n\r", Test_patterns_get_name(g_data_pattern));
                break;
              case '2':
                g_data_pattern = DATA_PATTERN_COUNTER;
                MPRINTF("\n\rPattern changed to: %s\n\r", Test_patterns_get_name(g_data_pattern));
                break;
              case '3':
                g_data_pattern = DATA_PATTERN_RANDOM;
                MPRINTF("\n\rPattern changed to: %s\n\r", Test_patterns_get_name(g_data_pattern));
                break;
              default:
                MPRINTF("\n\rInvalid choice, keeping current pattern\n\r");
                break;
            }
          }
          else
          {
            MPRINTF("\n\rInput timeout, keeping current pattern\n\r");
          }
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case 'B':
        case 'b':
          g_enable_data_verification = !g_enable_data_verification;
          MPRINTF("\n\rData verification %s\n\r", g_enable_data_verification ? "ENABLED" : "DISABLED");
          MPRINTF("Press any key to continue...\n\r");
          WAIT_CHAR(&choice, ms_to_ticks(100000));
          break;

        case 'C':
        case 'c':
          MPRINTF("\n\rEnter new constant pattern value (0-255) [current: 0x%02X]: ", g_fill_constant);
          uint32_t new_constant;
          if (VT100_input_uint32(&new_constant, 0, 255, g_fill_constant))
          {
            g_fill_constant = (uint8_t)new_constant;
            MPRINTF("Constant pattern value changed to 0x%02X\n\r", g_fill_constant);
          }
          else
          {
            MPRINTF("Input cancelled, keeping current value\n\r");
          }
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
