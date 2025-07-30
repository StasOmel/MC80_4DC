#include "App.h"
#include "yaffs_osglue.h"
#include "yaffs_trace.h"

// Global trace mask for YAFFS2 debugging
unsigned int yaffs_trace_mask = 0;

// Thread safety - using Azure RTOS mutex
static TX_MUTEX g_yaffs_mutex;
static bool     g_mutex_initialized = false;

// Error storage
static int g_yaffs_last_error = 0;

// Memory allocation statistics
static unsigned int g_malloc_current = 0;
static unsigned int g_malloc_high_water = 0;

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize YAFFS OS glue layer

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
void yaffsfs_OSInitialisation(void)
{
  if (!g_mutex_initialized)
  {
    // Create mutex for thread safety
    UINT status = tx_mutex_create(&g_yaffs_mutex, "YAFFS_Mutex", TX_NO_INHERIT);
    if (status == TX_SUCCESS)
    {
      g_mutex_initialized = true;
    }
  }

  // Initialize trace mask (can be configured as needed)
  yaffs_trace_mask = 0; // Set to YAFFS_TRACE_ALL for full debugging

  // Reset error state
  g_yaffs_last_error = 0;

  // Reset memory statistics
  g_malloc_current = 0;
  g_malloc_high_water = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Lock YAFFS for thread safety

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
void yaffsfs_Lock(void)
{
  if (g_mutex_initialized)
  {
    tx_mutex_get(&g_yaffs_mutex, TX_WAIT_FOREVER);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Unlock YAFFS for thread safety

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
void yaffsfs_Unlock(void)
{
  if (g_mutex_initialized)
  {
    tx_mutex_put(&g_yaffs_mutex);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get current time for YAFFS

  Parameters: none

  Return: current time in seconds since epoch
-----------------------------------------------------------------------------------------------------*/
u32 yaffsfs_CurrentTime(void)
{
  // For embedded systems, you might want to use RTC or tick counter
  // For now, return a simple tick-based time
  return (u32)(tx_time_get() / TX_TIMER_TICKS_PER_SECOND);
}

/*-----------------------------------------------------------------------------------------------------
  Description: Set YAFFS error code

  Parameters: err - error code to set

  Return: none
-----------------------------------------------------------------------------------------------------*/
void yaffsfs_SetError(int err)
{
  g_yaffs_last_error = err;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get last YAFFS error

  Parameters: none

  Return: last error code
-----------------------------------------------------------------------------------------------------*/
int yaffsfs_GetLastError(void)
{
  return g_yaffs_last_error;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Allocate memory for YAFFS

  Parameters: size - size in bytes to allocate

  Return: pointer to allocated memory or NULL if failed
-----------------------------------------------------------------------------------------------------*/
void *yaffsfs_malloc(size_t size)
{
  void *ptr = App_malloc(size);

  if (ptr != NULL)
  {
    g_malloc_current += size;
    if (g_malloc_current > g_malloc_high_water)
    {
      g_malloc_high_water = g_malloc_current;
    }
  }

  return ptr;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Free memory allocated by YAFFS

  Parameters: ptr - pointer to memory to free

  Return: none
-----------------------------------------------------------------------------------------------------*/
void yaffsfs_free(void *ptr)
{
  if (ptr != NULL)
  {
    // Note: We can't easily track the size being freed without additional bookkeeping
    // For now, just free the memory
    App_free(ptr);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get malloc statistics

  Parameters: current - pointer to store current allocated bytes
            high_water - pointer to store peak allocated bytes

  Return: none
-----------------------------------------------------------------------------------------------------*/
void yaffsfs_get_malloc_values(unsigned *current, unsigned *high_water)
{
  if (current != NULL)
  {
    *current = g_malloc_current;
  }

  if (high_water != NULL)
  {
    *high_water = g_malloc_high_water;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Check if memory region is valid for access

  Parameters: addr - memory address to check
            size - size of memory region
            write_request - 1 if write access, 0 if read access

  Return: 1 if valid, 0 if invalid
-----------------------------------------------------------------------------------------------------*/
int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
  // For embedded systems, this function should validate memory regions
  // For simplicity, we'll assume all memory regions are valid
  // In a real implementation, you might check:
  // - Address range validity
  // - Write protection
  // - Memory mapped I/O regions

  FSP_PARAMETER_NOT_USED(write_request);

  // Basic null pointer check
  if (addr == NULL || size == 0)
  {
    return 0; // Invalid
  }

  // For now, assume all non-null addresses are valid
  return 1; // Valid
}
