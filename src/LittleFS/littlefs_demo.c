/*-----------------------------------------------------------------------------------------------------
  Description: LittleFS demo functions for testing file operations

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/

#include "App.h"
#include "littlefs_adapter.h"
#include "RTT_utils.h"

// Global LittleFS context
extern T_littlefs_context g_littlefs_context;

/*-----------------------------------------------------------------------------------------------------
  Description: Get current time in milliseconds

  Parameters: none

  Return: time in milliseconds
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Get_time_ms(void)
{
  return tx_time_get() * (1000 / TX_TIMER_TICKS_PER_SECOND);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize and mount LittleFS filesystem

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_init(void)
{
  int err;

  // Initialize LittleFS configuration
  err = Littlefs_initialize();
  if (err != 0)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("LittleFS initialization failed: %d\n\r", err);
    return err;
  }

  // Try to mount the filesystem
  err = Littlefs_mount();
  if (err != 0)
  {
    LITTLEFS_DEBUG_PRINTF("Mount failed, trying to format...\n\r");

    // Format the filesystem if mount fails
    err = Littlefs_format();
    if (err != 0)
    {
      LITTLEFS_DEBUG_ERR_PRINTF("LittleFS format failed: %d\n\r", err);
      return err;
    }

    // Try to mount again after format
    err = Littlefs_mount();
    if (err != 0)
    {
      LITTLEFS_DEBUG_ERR_PRINTF("LittleFS mount failed after format: %d\n\r", err);
      return err;
    }
  }

  LITTLEFS_DEBUG_PRINTF("LittleFS initialized and mounted successfully\n\r");
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Write test data to a file

  Parameters: filename - name of the file to write
              data - data to write
              size - size of data to write

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_write_file(const char *filename, const void *data, size_t size)
{
  lfs_file_t file;
  int        err;
  uint32_t   start_time, end_time, open_time, close_time;
  uint32_t   open_start, open_end, close_start, close_end;
  uint32_t   operation_time, speed_kbps;

  LITTLEFS_DEBUG_PRINTF("Writing file %s (%u bytes)... ", filename, size);

  start_time = _Get_time_ms();

  // Open file for writing with timing
  open_start = _Get_time_ms();
  err = lfs_file_open(&g_littlefs_context.lfs, &file, filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
  open_end = _Get_time_ms();
  open_time = open_end - open_start;

  if (err < 0)
  {
    end_time = _Get_time_ms();
    operation_time = end_time - start_time;
    LITTLEFS_DEBUG_ERR_PRINTF("FAILED (open): %d (open: %u ms, total: %u ms)\n\r", err, open_time, operation_time);
    return err;
  }

  LITTLEFS_DEBUG_PRINTF("opened in %u ms, ", open_time);

  // Write data to file
  lfs_ssize_t written = lfs_file_write(&g_littlefs_context.lfs, &file, data, size);
  if (written < 0)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("FAILED (write): %d\n\r", (int)written);
    lfs_file_close(&g_littlefs_context.lfs, &file);
    end_time = _Get_time_ms();
    operation_time = end_time - start_time;
    return (int)written;
  }

  if ((size_t)written != size)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("Partial write: %d of %u bytes\n\r", (int)written, size);
  }

  // Close file with timing
  close_start = _Get_time_ms();
  err = lfs_file_close(&g_littlefs_context.lfs, &file);
  close_end = _Get_time_ms();
  close_time = close_end - close_start;
  end_time = _Get_time_ms();
  operation_time = end_time - start_time;

  if (err < 0)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("FAILED (close): %d (close: %u ms, total: %u ms)\n\r", err, close_time, operation_time);
    return err;
  }

  // Calculate speed in KB/s (avoid division by zero)
  if (operation_time > 0)
  {
    speed_kbps = (size * 1000) / (operation_time * 1024);
  }
  else
  {
    speed_kbps = 0;
  }

  LITTLEFS_DEBUG_PRINTF("closed in %u ms, %d bytes written, total: %u ms, %u KB/s\n\r",
                        close_time, (int)written, operation_time, speed_kbps);
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Read test data from a file

  Parameters: filename - name of the file to read
              buffer - buffer to store read data
              buffer_size - size of the buffer

  Return: number of bytes read on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_read_file(const char *filename, void *buffer, size_t buffer_size)
{
  lfs_file_t file;
  int        err;
  uint32_t   start_time, end_time, open_time, close_time;
  uint32_t   open_start, open_end, close_start, close_end;
  uint32_t   operation_time, speed_kbps;

  LITTLEFS_DEBUG_PRINTF("Reading file %s... ", filename);

  start_time = _Get_time_ms();

  // Open file for reading with timing
  open_start = _Get_time_ms();
  err = lfs_file_open(&g_littlefs_context.lfs, &file, filename, LFS_O_RDONLY);
  open_end = _Get_time_ms();
  open_time = open_end - open_start;

  if (err < 0)
  {
    end_time = _Get_time_ms();
    operation_time = end_time - start_time;
    LITTLEFS_DEBUG_ERR_PRINTF("FAILED (open): %d (open: %u ms, total: %u ms)\n\r", err, open_time, operation_time);
    return err;
  }

  LITTLEFS_DEBUG_PRINTF("opened in %u ms, ", open_time);

  // Read data from file
  lfs_ssize_t read_bytes = lfs_file_read(&g_littlefs_context.lfs, &file, buffer, buffer_size);
  if (read_bytes < 0)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("FAILED (read): %d\n\r", (int)read_bytes);
    lfs_file_close(&g_littlefs_context.lfs, &file);
    end_time = _Get_time_ms();
    operation_time = end_time - start_time;
    return (int)read_bytes;
  }

  // Close file with timing
  close_start = _Get_time_ms();
  err = lfs_file_close(&g_littlefs_context.lfs, &file);
  close_end = _Get_time_ms();
  close_time = close_end - close_start;
  end_time = _Get_time_ms();
  operation_time = end_time - start_time;

  if (err < 0)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("FAILED (close): %d (close: %u ms, total: %u ms)\n\r", err, close_time, operation_time);
    return err;
  }

  // Calculate speed in KB/s (avoid division by zero)
  if (operation_time > 0)
  {
    speed_kbps = (read_bytes * 1000) / (operation_time * 1024);
  }
  else
  {
    speed_kbps = 0;
  }

  LITTLEFS_DEBUG_PRINTF("closed in %u ms, %d bytes read, total: %u ms, %u KB/s\n\r",
                        close_time, (int)read_bytes, operation_time, speed_kbps);
  return (int)read_bytes;
}

/*-----------------------------------------------------------------------------------------------------
  Description: List files in the root directory

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_list_files(void)
{
  lfs_dir_t       dir;
  struct lfs_info info;
  int             err;
  int             file_count = 0;

  // Open root directory
  err                        = lfs_dir_open(&g_littlefs_context.lfs, &dir, "/");
  if (err < 0)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("Failed to open root directory: %d\n\r", err);
    return err;
  }

  LITTLEFS_DEBUG_PRINTF("Files in root directory:\n\r");

  // Read directory entries
  while (true)
  {
    err = lfs_dir_read(&g_littlefs_context.lfs, &dir, &info);
    if (err < 0)
    {
      LITTLEFS_DEBUG_ERR_PRINTF("Failed to read directory: %d\n\r", err);
      lfs_dir_close(&g_littlefs_context.lfs, &dir);
      return err;
    }

    // End of directory
    if (err == 0)
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
      LITTLEFS_DEBUG_PRINTF("  File: %s (size: %u bytes)\n\r", info.name, info.size);
    }
    else if (info.type == LFS_TYPE_DIR)
    {
      LITTLEFS_DEBUG_PRINTF("  Dir:  %s\n\r", info.name);
    }

    file_count++;
  }

  // Close directory
  err = lfs_dir_close(&g_littlefs_context.lfs, &dir);
  if (err < 0)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("Failed to close directory: %d\n\r", err);
    return err;
  }

  LITTLEFS_DEBUG_PRINTF("Total entries: %d\n\r", file_count);
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Delete a file

  Parameters: filename - name of the file to delete

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_delete_file(const char *filename)
{
  int err;

  err = lfs_remove(&g_littlefs_context.lfs, filename);
  if (err < 0)
  {
    LITTLEFS_DEBUG_ERR_PRINTF("Failed to delete file %s: %d\n\r", filename, err);
    return err;
  }

  LITTLEFS_DEBUG_PRINTF("File %s deleted successfully\n\r", filename);
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Run comprehensive LittleFS test

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_demo_test(void)
{
  int         err;
  const char *test_filename = "test_file.txt";
  const char *test_data     = "Hello LittleFS! This is a test file.";
  char        read_buffer[100];

  LITTLEFS_DEBUG_PRINTF("Starting LittleFS test...\n\r");

  // Initialize filesystem
  err = Littlefs_demo_init();
  if (err != 0)
  {
    return err;
  }

  // List files before test
  LITTLEFS_DEBUG_PRINTF("Files before test:\n\r");
  Littlefs_demo_list_files();

  // Write test file
  err = Littlefs_demo_write_file(test_filename, test_data, strlen(test_data));
  if (err != 0)
  {
    return err;
  }

  // Read test file
  memset(read_buffer, 0, sizeof(read_buffer));
  err = Littlefs_demo_read_file(test_filename, read_buffer, sizeof(read_buffer) - 1);
  if (err < 0)
  {
    return err;
  }

  // Verify data
  if (strcmp(test_data, read_buffer) == 0)
  {
    LITTLEFS_DEBUG_PRINTF("Data verification SUCCESS!\n\r");
  }
  else
  {
    LITTLEFS_DEBUG_ERR_PRINTF("Data verification FAILED!\n\r");
    LITTLEFS_DEBUG_ERR_PRINTF("Expected: %s\n\r", test_data);
    LITTLEFS_DEBUG_ERR_PRINTF("Read:     %s\n\r", read_buffer);
    return -1;
  }

  // List files after test
  LITTLEFS_DEBUG_PRINTF("Files after test:\n\r");
  Littlefs_demo_list_files();

  // Delete test file
  err = Littlefs_demo_delete_file(test_filename);
  if (err != 0)
  {
    return err;
  }

  // List files after delete
  LITTLEFS_DEBUG_PRINTF("Files after delete:\n\r");
  Littlefs_demo_list_files();

  LITTLEFS_DEBUG_PRINTF("LittleFS test completed successfully!\n\r");
  return 0;
}
