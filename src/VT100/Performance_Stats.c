#include "Performance_Stats.h"
#include "FS_Test_Config.h"

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize performance statistics structure

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_init(T_performance_stats *stats)
{
  stats->min_time         = UINT32_MAX;
  stats->max_time         = 0;
  stats->avg_time         = 0;
  stats->total_time       = 0;
  stats->success_count    = 0;
  stats->error_count      = 0;
  stats->total_bytes      = 0;
  stats->min_speed_kbps   = UINT32_MAX;
  stats->max_speed_kbps   = 0;
  stats->avg_speed_kbps   = 0;
  stats->total_open_time  = 0;
  stats->min_open_time    = UINT32_MAX;
  stats->max_open_time    = 0;
  stats->total_close_time = 0;
  stats->min_close_time   = UINT32_MAX;
  stats->max_close_time   = 0;
  stats->total_io_time    = 0;
  stats->crc_errors       = 0;
  stats->pattern_errors   = 0;
  stats->size_errors      = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize performance statistics for delete operations

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_init_delete(T_performance_stats *stats)
{
  stats->min_time         = UINT32_MAX;
  stats->max_time         = 0;
  stats->avg_time         = 0;
  stats->total_time       = 0;
  stats->success_count    = 0;
  stats->error_count      = 0;
  stats->total_bytes      = 0;  // Not applicable for delete
  stats->min_speed_kbps   = 0;  // Not applicable for delete
  stats->max_speed_kbps   = 0;  // Not applicable for delete
  stats->avg_speed_kbps   = 0;  // Not applicable for delete
  stats->total_open_time  = 0;  // Not applicable for delete
  stats->min_open_time    = 0;  // Not applicable for delete
  stats->max_open_time    = 0;  // Not applicable for delete
  stats->total_close_time = 0;  // Not applicable for delete
  stats->min_close_time   = 0;  // Not applicable for delete
  stats->max_close_time   = 0;  // Not applicable for delete
  stats->total_io_time    = 0;  // Not applicable for delete
  stats->crc_errors       = 0;  // Not applicable for delete
  stats->pattern_errors   = 0;  // Not applicable for delete
  stats->size_errors      = 0;  // Not applicable for delete
}

/*-----------------------------------------------------------------------------------------------------
  Description: Update timing statistics for successful operation

  Parameters: stats - pointer to statistics structure
              operation_time - total operation time
              open_time - file open time
              close_time - file close time
              io_time - pure I/O time
              bytes_processed - number of bytes processed
              speed_kbps - operation speed in KB/s

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_update_success(T_performance_stats *stats,
                                      uint32_t             operation_time,
                                      uint32_t             open_time,
                                      uint32_t             close_time,
                                      uint32_t             io_time,
                                      uint32_t             bytes_processed,
                                      uint32_t             speed_kbps)
{
  stats->total_bytes += bytes_processed;
  stats->success_count++;

  // Update timing statistics
  if (operation_time < stats->min_time) stats->min_time = operation_time;
  if (operation_time > stats->max_time) stats->max_time = operation_time;
  stats->total_time += operation_time;

  // Update open/close timing statistics
  stats->total_open_time += open_time;
  if (open_time < stats->min_open_time) stats->min_open_time = open_time;
  if (open_time > stats->max_open_time) stats->max_open_time = open_time;

  stats->total_close_time += close_time;
  if (close_time < stats->min_close_time) stats->min_close_time = close_time;
  if (close_time > stats->max_close_time) stats->max_close_time = close_time;

  stats->total_io_time += io_time;

  // Update speed statistics
  if (speed_kbps < stats->min_speed_kbps) stats->min_speed_kbps = speed_kbps;
  if (speed_kbps > stats->max_speed_kbps) stats->max_speed_kbps = speed_kbps;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Update timing statistics for delete operation

  Parameters: stats - pointer to statistics structure
              operation_time - total operation time
              file_size - size of deleted file in bytes

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_update_delete_success(T_performance_stats *stats, uint32_t operation_time, uint32_t file_size)
{
  stats->success_count++;
  stats->total_bytes += file_size;

  // Update timing statistics
  if (operation_time < stats->min_time) stats->min_time = operation_time;
  if (operation_time > stats->max_time) stats->max_time = operation_time;
  stats->total_time += operation_time;

  // Calculate speed in KB/s based on operation time (avoid division by zero)
  // Using binary KB (1 KB = 1024 bytes) for speed calculation with floating point precision
  uint32_t speed_kbps;
  if (operation_time > 0)
  {
    speed_kbps = (uint32_t)((float)file_size * 1000000.0f / ((float)operation_time * 1024.0f));
  }
  else
  {
    speed_kbps = 0;
  }

  // Update speed statistics
  if (speed_kbps < stats->min_speed_kbps) stats->min_speed_kbps = speed_kbps;
  if (speed_kbps > stats->max_speed_kbps) stats->max_speed_kbps = speed_kbps;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Increment error counter

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_increment_error(T_performance_stats *stats)
{
  stats->error_count++;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Update error counter (alias for increment_error)

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_update_error(T_performance_stats *stats)
{
  stats->error_count++;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Increment CRC error counter

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_increment_crc_error(T_performance_stats *stats)
{
  stats->crc_errors++;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Increment pattern error counter

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_increment_pattern_error(T_performance_stats *stats)
{
  stats->pattern_errors++;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Increment size error counter

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_increment_size_error(T_performance_stats *stats)
{
  stats->size_errors++;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Calculate final averages and clean up statistics

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_finalize(T_performance_stats *stats)
{
  // Calculate averages
  if (stats->success_count > 0)
  {
    stats->avg_time = stats->total_time / stats->success_count;
    if (stats->total_io_time > 0)
    {
      stats->avg_speed_kbps = (uint32_t)((float)stats->total_bytes * 1000000.0f / ((float)stats->total_io_time * 1024.0f));
    }
    if (stats->min_speed_kbps == UINT32_MAX)
    {
      stats->min_speed_kbps = 0;
    }
    if (stats->min_open_time == UINT32_MAX)
    {
      stats->min_open_time = 0;
    }
    if (stats->min_close_time == UINT32_MAX)
    {
      stats->min_close_time = 0;
    }
  }
  else
  {
    stats->min_time       = 0;
    stats->avg_time       = 0;
    stats->min_speed_kbps = 0;
    stats->min_open_time  = 0;
    stats->max_open_time  = 0;
    stats->min_close_time = 0;
    stats->max_close_time = 0;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print operation statistics (unified format for all filesystem tests)

  Parameters: operation_name - name of operation
              stats - statistics to print
              data_verification_enabled - whether data verification was enabled

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_print(const char *operation_name, T_performance_stats *stats, bool data_verification_enabled)
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
    if (data_verification_enabled)
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
  Description: Print write operation success result with speed and CRC32 info

  Parameters: close_time - file close time in microseconds
              io_time - I/O operation time in microseconds
              operation_time - total operation time in microseconds
              file_size - size of file in bytes
              crc32_value - CRC32 value of written data
              data_verification_enabled - whether data verification was enabled

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_print_write_success(uint32_t close_time, uint32_t io_time, uint32_t operation_time,
                                           uint32_t file_size, uint32_t crc32_value, bool data_verification_enabled)
{
  GET_MCBL;
  uint32_t speed_kbps;

  // Calculate speed in KB/s based on I/O time
  if (io_time > 0)
  {
    speed_kbps = (uint32_t)((float)file_size * 1000000.0f / ((float)io_time * 1024.0f));
  }
  else
  {
    speed_kbps = 0;
  }

  MPRINTF("closed: %5u us, I/O: %6u us, total: %6u us, speed: %5u KB/s", close_time, io_time, operation_time, speed_kbps);

  // Show CRC32 if verification enabled
  if (data_verification_enabled && file_size >= FS_CRC32_SIZE)
  {
    MPRINTF(", CRC32: 0x%08X", crc32_value);
  }
  MPRINTF("\n\r");
}

/*-----------------------------------------------------------------------------------------------------
  Description: Print read operation success result with speed and CRC32 info

  Parameters: close_time - file close time in microseconds
              io_time - I/O operation time in microseconds
              operation_time - total operation time in microseconds
              bytes_read - number of bytes read
              file_size - expected file size in bytes
              crc32_value - CRC32 value from file
              crc_valid - whether CRC32 verification passed
              pattern_valid - whether pattern verification passed (LittleFS only)
              size_valid - whether size verification passed (LittleFS only)
              data_verification_enabled - whether data verification was enabled
              is_littlefs - true for LittleFS, false for FileX

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_print_read_success(uint32_t close_time, uint32_t io_time, uint32_t operation_time,
                                          uint32_t bytes_read, uint32_t file_size, uint32_t crc32_value,
                                          bool crc_valid, bool pattern_valid, bool size_valid,
                                          bool data_verification_enabled, bool is_littlefs)
{
  GET_MCBL;
  uint32_t speed_kbps;

  // Calculate speed in KB/s based on I/O time
  if (io_time > 0)
  {
    speed_kbps = (uint32_t)((float)bytes_read * 1000000.0f / ((float)io_time * 1024.0f));
  }
  else
  {
    speed_kbps = 0;
  }

  if (is_littlefs)
  {
    MPRINTF("closed: %5u us, size: %5u bytes, I/O: %6u us, total: %6u us, speed: %5u KB/s", close_time, bytes_read, io_time, operation_time, speed_kbps);
  }
  else
  {
    MPRINTF("closed: %5u us, I/O: %6u us, total: %6u us, speed: %5u KB/s", close_time, io_time, operation_time, speed_kbps);
  }

  // Show verification results if enabled
  if (data_verification_enabled && file_size >= FS_CRC32_SIZE)
  {
    if (is_littlefs)
    {
      // LittleFS format: CRC, Pattern, Size
      MPRINTF(", CRC: %s", crc_valid ? "OK" : "ERROR");
      MPRINTF(", Pattern: %s", pattern_valid ? "OK" : "ERROR");
      MPRINTF(", Size: %s", size_valid ? "OK" : "ERROR");
      if (crc_valid)
      {
        MPRINTF(" (0x%08X)", crc32_value);
      }
    }
    else
    {
      // FileX format: CRC32 only
      if (crc_valid)
      {
        MPRINTF(", CRC32: OK (0x%08X)", crc32_value);
      }
      else
      {
        MPRINTF(", CRC32: FAILED");
      }
    }
  }
  MPRINTF("\n\r");
}
