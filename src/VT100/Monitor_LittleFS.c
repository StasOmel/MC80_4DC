#include "App.h"

#define LFS_TEST_FILE_SIZE 256

static uint8_t g_lfs_test_buffer[LFS_TEST_FILE_SIZE];

// External global LittleFS context
extern T_littlefs_context g_littlefs_context;

const T_VT100_Menu_item MENU_LittleFS_items[] = {
  { '1', Do_LittleFS_init, NULL },
  { '2', Do_LittleFS_list_files, NULL },
  { '3', Do_LittleFS_test_file_ops, NULL },
  { '4', Do_LittleFS_comprehensive_test, NULL },
  { 'R', NULL, NULL },
  { 0 }  // End of menu
};

const T_VT100_Menu MENU_LittleFS = {
  "LittleFS Testing",
  "\033[5C LittleFS file system testing menu\r\n"
  "\033[5C <1> - Initialize LittleFS (auto-format if needed)\r\n"
  "\033[5C <2> - List files\r\n"
  "\033[5C <3> - Test file operations\r\n"
  "\033[5C <4> - Run comprehensive test\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_LittleFS_items
};

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
  Description: Internal function to list files without user interface

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void Do_LittleFS_list_files_internal(void)
{
  GET_MCBL;

  if (!Littlefs_is_mounted())
  {
    MPRINTF("Filesystem not mounted.\n\r");
    return;
  }

  lfs_dir_t       dir;
  struct lfs_info info;
  int             result;
  int             file_count = 0;

  // Open root directory
  result                     = lfs_dir_open(&g_littlefs_context.lfs, &dir, "/");
  if (result < 0)
  {
    MPRINTF("Failed to open root directory: %s\n\r", _Littlefs_error_to_string(result));
    return;
  }

  // Read directory entries
  while (true)
  {
    result = lfs_dir_read(&g_littlefs_context.lfs, &dir, &info);
    if (result < 0)
    {
      MPRINTF("Failed to read directory: %s\n\r", _Littlefs_error_to_string(result));
      lfs_dir_close(&g_littlefs_context.lfs, &dir);
      return;
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

    if (info.type == LFS_TYPE_REG)
    {
      MPRINTF("  File: %s (size: %u bytes)\n\r", info.name, info.size);
    }
    else if (info.type == LFS_TYPE_DIR)
    {
      MPRINTF("  Dir:  %s\n\r", info.name);
    }

    file_count++;
  }

  // Close directory
  lfs_dir_close(&g_littlefs_context.lfs, &dir);
  MPRINTF("Total entries: %d\n\r", file_count);
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

  // Initialize LittleFS configuration
  int result = Littlefs_initialize();
  if (result != 0)
  {
    MPRINTF("LittleFS initialization failed: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  // Try to mount the filesystem
  result = Littlefs_mount();
  if (result != 0)
  {
    MPRINTF("Mount failed: %s\n\r", _Littlefs_error_to_string(result));
    MPRINTF("Trying to format...\n\r");

    // Format the filesystem if mount fails
    result = Littlefs_format();
    if (result != 0)
    {
      MPRINTF("LittleFS format failed: %s\n\r", _Littlefs_error_to_string(result));
      goto exit;
    }

    // Try to mount again after format
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
  Description: List all files in the LittleFS filesystem

  Parameters: keycode - input key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_list_files(uint8_t keycode)
{
  GET_MCBL;
  lfs_dir_t       dir;
  struct lfs_info info;
  int             result;
  int             file_count = 0;

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS File List ===\n\r");

  // Check if filesystem is mounted
  if (!Littlefs_is_mounted())
  {
    MPRINTF("Filesystem not mounted. Please initialize first.\n\r");
    goto exit;
  }

  // Open root directory
  result = lfs_dir_open(&g_littlefs_context.lfs, &dir, "/");
  if (result < 0)
  {
    MPRINTF("Failed to open root directory: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  MPRINTF("Files in root directory:\n\r");

  // Read directory entries
  while (true)
  {
    result = lfs_dir_read(&g_littlefs_context.lfs, &dir, &info);
    if (result < 0)
    {
      MPRINTF("Failed to read directory: %s\n\r", _Littlefs_error_to_string(result));
      lfs_dir_close(&g_littlefs_context.lfs, &dir);
      goto exit;
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

    if (info.type == LFS_TYPE_REG)
    {
      MPRINTF("  File: %s (size: %u bytes)\n\r", info.name, info.size);
    }
    else if (info.type == LFS_TYPE_DIR)
    {
      MPRINTF("  Dir:  %s\n\r", info.name);
    }

    file_count++;
  }

  // Close directory
  result = lfs_dir_close(&g_littlefs_context.lfs, &dir);
  if (result < 0)
  {
    MPRINTF("Failed to close directory: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  MPRINTF("Total entries: %d\n\r", file_count);

exit:
  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Test basic file operations (write, read, verify, delete)

  Parameters: keycode - input key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_test_file_ops(uint8_t keycode)
{
  GET_MCBL;
  const char *test_filename = "/test.bin";
  lfs_file_t  file;
  int         result;
  lfs_ssize_t written;
  lfs_ssize_t read_bytes;
  bool        data_ok = true;

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== File Operations Test ===\n\r");

  // Check if filesystem is mounted
  if (!Littlefs_is_mounted())
  {
    MPRINTF("Filesystem not mounted. Please initialize first.\n\r");
    goto exit;
  }

  // Fill test buffer with pattern
  for (uint32_t i = 0; i < LFS_TEST_FILE_SIZE; i++)
  {
    g_lfs_test_buffer[i] = (uint8_t)(i & 0xFF);
  }

  MPRINTF("Testing file write...\n\r");

  // Open file for writing
  result = lfs_file_open(&g_littlefs_context.lfs, &file, test_filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
  if (result < 0)
  {
    MPRINTF("Failed to open file for writing: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  // Write data to file
  written = lfs_file_write(&g_littlefs_context.lfs, &file, g_lfs_test_buffer, LFS_TEST_FILE_SIZE);
  if (written < 0)
  {
    MPRINTF("Failed to write data to file: %s\n\r", _Littlefs_error_to_string((int)written));
    lfs_file_close(&g_littlefs_context.lfs, &file);
    goto exit;
  }

  if ((size_t)written != LFS_TEST_FILE_SIZE)
  {
    MPRINTF("Partial write: %d of %u bytes\n\r", (int)written, LFS_TEST_FILE_SIZE);
  }

  // Close file
  result = lfs_file_close(&g_littlefs_context.lfs, &file);
  if (result < 0)
  {
    MPRINTF("Failed to close file after write: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  MPRINTF("File write successful (%d bytes)\n\r", (int)written);

  // Clear buffer and read back
  memset(g_lfs_test_buffer, 0, LFS_TEST_FILE_SIZE);

  MPRINTF("Testing file read...\n\r");

  // Open file for reading
  result = lfs_file_open(&g_littlefs_context.lfs, &file, test_filename, LFS_O_RDONLY);
  if (result < 0)
  {
    MPRINTF("Failed to open file for reading: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  // Read data from file
  read_bytes = lfs_file_read(&g_littlefs_context.lfs, &file, g_lfs_test_buffer, LFS_TEST_FILE_SIZE);
  if (read_bytes < 0)
  {
    MPRINTF("Failed to read data from file: %s\n\r", _Littlefs_error_to_string((int)read_bytes));
    lfs_file_close(&g_littlefs_context.lfs, &file);
    goto exit;
  }

  // Close file
  result = lfs_file_close(&g_littlefs_context.lfs, &file);
  if (result < 0)
  {
    MPRINTF("Failed to close file after read: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  MPRINTF("File read successful (%d bytes)\n\r", (int)read_bytes);

  // Verify data integrity
  data_ok = true;
  for (int i = 0; i < read_bytes; i++)
  {
    if (g_lfs_test_buffer[i] != (uint8_t)(i & 0xFF))
    {
      data_ok = false;
      break;
    }
  }

  if (data_ok)
  {
    MPRINTF("Data integrity check: PASSED\n\r");
  }
  else
  {
    MPRINTF("Data integrity check: FAILED\n\r");
    goto exit;
  }

  // Test file deletion
  MPRINTF("Testing file deletion...\n\r");
  result = lfs_remove(&g_littlefs_context.lfs, test_filename);

  if (result == 0)
  {
    MPRINTF("File deletion successful\n\r");
  }
  else
  {
    MPRINTF("File deletion failed: %s\n\r", _Littlefs_error_to_string(result));
  }

exit:
  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Run comprehensive LittleFS test suite

  Parameters: keycode - input key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_comprehensive_test(uint8_t keycode)
{
  GET_MCBL;
  const char *test_filename = "/test_file.txt";
  const char *test_data     = "Hello LittleFS! This is a test file.";
  char        read_buffer[100];
  lfs_file_t  file;
  int         result;
  lfs_ssize_t written;
  lfs_ssize_t read_bytes;

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS Comprehensive Test ===\n\r");

  // Check if filesystem is mounted
  if (!Littlefs_is_mounted())
  {
    MPRINTF("Filesystem not mounted. Please initialize first.\n\r");
    goto exit;
  }

  MPRINTF("Running comprehensive filesystem test...\n\r");

  // List files before test
  MPRINTF("\nFiles before test:\n\r");
  Do_LittleFS_list_files_internal();

  // Write test file
  MPRINTF("\nWriting test file...\n\r");
  result = lfs_file_open(&g_littlefs_context.lfs, &file, test_filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
  if (result < 0)
  {
    MPRINTF("Failed to open file for writing: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  written = lfs_file_write(&g_littlefs_context.lfs, &file, test_data, strlen(test_data));
  if (written < 0)
  {
    MPRINTF("Failed to write data: %s\n\r", _Littlefs_error_to_string((int)written));
    lfs_file_close(&g_littlefs_context.lfs, &file);
    goto exit;
  }

  result = lfs_file_close(&g_littlefs_context.lfs, &file);
  if (result < 0)
  {
    MPRINTF("Failed to close file after write: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  MPRINTF("Written %d bytes\n\r", (int)written);

  // Read test file
  MPRINTF("Reading test file...\n\r");
  memset(read_buffer, 0, sizeof(read_buffer));

  result = lfs_file_open(&g_littlefs_context.lfs, &file, test_filename, LFS_O_RDONLY);
  if (result < 0)
  {
    MPRINTF("Failed to open file for reading: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  read_bytes = lfs_file_read(&g_littlefs_context.lfs, &file, read_buffer, sizeof(read_buffer) - 1);
  if (read_bytes < 0)
  {
    MPRINTF("Failed to read data: %s\n\r", _Littlefs_error_to_string((int)read_bytes));
    lfs_file_close(&g_littlefs_context.lfs, &file);
    goto exit;
  }

  result = lfs_file_close(&g_littlefs_context.lfs, &file);
  if (result < 0)
  {
    MPRINTF("Failed to close file after read: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  MPRINTF("Read %d bytes\n\r", (int)read_bytes);

  // Verify data
  if (strcmp(test_data, read_buffer) == 0)
  {
    MPRINTF("Data verification: SUCCESS!\n\r");
  }
  else
  {
    MPRINTF("Data verification: FAILED!\n\r");
    MPRINTF("Expected: %s\n\r", test_data);
    MPRINTF("Read:     %s\n\r", read_buffer);
    goto exit;
  }

  // List files after test
  MPRINTF("\nFiles after test:\n\r");
  Do_LittleFS_list_files_internal();

  // Delete test file
  MPRINTF("\nDeleting test file...\n\r");
  result = lfs_remove(&g_littlefs_context.lfs, test_filename);
  if (result != 0)
  {
    MPRINTF("Failed to delete test file: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  // List files after delete
  MPRINTF("Files after delete:\n\r");
  Do_LittleFS_list_files_internal();

  MPRINTF("\nLittleFS comprehensive test completed successfully!\n\r");

exit:
  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}
