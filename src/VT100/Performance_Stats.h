#ifndef PERFORMANCE_STATS_H
#define PERFORMANCE_STATS_H

#include "App.h"

/*-----------------------------------------------------------------------------------------------------
  Description: Common performance statistics structure for filesystem tests

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
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
} T_performance_stats;

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize performance statistics structure

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_init(T_performance_stats *stats);

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize performance statistics for delete operations

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_init_delete(T_performance_stats *stats);

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
                                       uint32_t operation_time,
                                       uint32_t open_time,
                                       uint32_t close_time,
                                       uint32_t io_time,
                                       uint32_t bytes_processed,
                                       uint32_t speed_kbps);

/*-----------------------------------------------------------------------------------------------------
  Description: Update timing statistics for delete operation

  Parameters: stats - pointer to statistics structure
              operation_time - total operation time

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_update_delete_success(T_performance_stats *stats, uint32_t operation_time);

/*-----------------------------------------------------------------------------------------------------
  Description: Increment error counter

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_increment_error(T_performance_stats *stats);

/*-----------------------------------------------------------------------------------------------------
  Description: Update error counter (alias for increment_error)

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_update_error(T_performance_stats *stats);

/*-----------------------------------------------------------------------------------------------------
  Description: Increment CRC error counter

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_increment_crc_error(T_performance_stats *stats);

/*-----------------------------------------------------------------------------------------------------
  Description: Increment pattern error counter

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_increment_pattern_error(T_performance_stats *stats);

/*-----------------------------------------------------------------------------------------------------
  Description: Increment size error counter

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_increment_size_error(T_performance_stats *stats);

/*-----------------------------------------------------------------------------------------------------
  Description: Calculate final averages and clean up statistics

  Parameters: stats - pointer to statistics structure

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_finalize(T_performance_stats *stats);

/*-----------------------------------------------------------------------------------------------------
  Description: Print operation statistics (unified format for all filesystem tests)

  Parameters: operation_name - name of operation
              stats - statistics to print
              data_verification_enabled - whether data verification was enabled

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Performance_stats_print(const char *operation_name, T_performance_stats *stats, bool data_verification_enabled);

#endif // PERFORMANCE_STATS_H
