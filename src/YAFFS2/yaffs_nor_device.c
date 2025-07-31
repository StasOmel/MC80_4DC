#include "yaffs_nor_device.h"
#include "yaffs_nor_adapter.h"
#include "yaffs_nor_config.h"

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 device configuration for deterministic NOR Flash operation

  This configuration prioritizes deterministic behavior and real-time performance
  over maximum storage efficiency. All timing-sensitive operations are carefully
  controlled to meet hard real-time requirements.
-----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 device instance for NOR Flash

  This structure defines the complete configuration for a YAFFS2 filesystem
  running on NOR Flash with deterministic timing guarantees.
-----------------------------------------------------------------------------------------------------*/
static struct yaffs_dev g_yaffs_nor_device = {
  .param = {
   // Device identification
   .name                  = "/",
   .total_bytes_per_chunk = YAFFS_NOR_PAGE_DATA_SIZE,
   .chunks_per_block      = YAFFS_NOR_PAGES_PER_BLOCK,
   .spare_bytes_per_chunk = YAFFS_NOR_PAGE_OOB_SIZE,
   .start_block           = YAFFS_NOR_START_BLOCK,
   .end_block             = YAFFS_NOR_END_BLOCK,
   .n_reserved_blocks     = YAFFS_NOR_RESERVED_BLOCKS,

   // YAFFS2 feature configuration
   .is_yaffs2             = 1,  // Enable YAFFS2 features
   .use_header_file_size  = YAFFS_NOR_USE_HEADER_FILE_SIZE,
   .refresh_period        = YAFFS_NOR_REFRESH_PERIOD,
   .n_caches              = YAFFS_NOR_CACHE_SIZE,

   // Deterministic behavior settings
   .empty_lost_n_found    = YAFFS_NOR_EMPTY_LOST_AND_FOUND,

   // ECC and reliability settings
   .no_tags_ecc           = YAFFS_NOR_NO_TAGS_ECC,  // Disable ECC for metadata
   .inband_tags           = YAFFS_NOR_INBAND_TAGS,  // Store tags inside data area
  },

  // Driver functions for hardware interface
  .drv = {
   .drv_write_chunk_fn  = Yaffs_nor_write_chunk,  // Basic write function (required)
   .drv_read_chunk_fn   = Yaffs_nor_read_chunk,   // Basic read function (required)
   .drv_erase_fn        = Yaffs_nor_erase_block,
   .drv_mark_bad_fn     = Yaffs_nor_mark_bad_block,
   .drv_check_bad_fn    = Yaffs_nor_check_bad_block,
   .drv_initialise_fn   = Yaffs_nor_initialise,
   .drv_deinitialise_fn = Yaffs_nor_deinitialise,
  },

  // Tags handler for metadata operations (NOT USED in inband tags mode)
  .tagger = {
   .write_chunk_tags_fn = NULL,  // Not used in inband tags mode
   .read_chunk_tags_fn  = NULL,  // Not used in inband tags mode
   .query_block_fn      = NULL,  // Optional
   .mark_bad_fn         = NULL,  // Not used in inband tags mode
  },

  // Runtime state (initialized by YAFFS2)
  .is_mounted      = 0,
  .read_only       = 0,
  .is_checkpointed = 0,
};

/*-----------------------------------------------------------------------------------------------------
  Initialize YAFFS2 device for NOR Flash

  This function prepares the YAFFS2 device structure and registers it with the YAFFS2 system.
  Must be called before mounting the filesystem.

  Return:
    0 on success, -1 on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_device_init(void)
{
  // Add device to YAFFS2 system
  yaffs_add_device(&g_yaffs_nor_device);

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Mount YAFFS2 filesystem on NOR Flash

  Attempts to mount the filesystem. If mounting fails (e.g., unformatted flash),
  the function will format the flash and retry mounting.

  Parameters:
    mount_point - Mount point path (e.g., "/norflash")

  Return:
    0 on success, -1 on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_device_mount(const char *mount_point)
{
  int result;

  if (NULL == mount_point)
  {
    return -1;
  }

  // Attempt to mount existing filesystem
  result = yaffs_mount(mount_point);

  if (result < 0)
  {
    // Format the flash and try mounting again
    result = yaffs_format(mount_point, 0, 0, 0);
    if (result < 0)
    {
      return -1;
    }

    result = yaffs_mount(mount_point);
  }

  if (result >= 0)
  {
    g_yaffs_nor_device.is_mounted = 1;
  }

  return result;
}

/*-----------------------------------------------------------------------------------------------------
  Unmount YAFFS2 filesystem

  Cleanly unmounts the filesystem, ensuring all cached data is written to flash.

  Parameters:
    mount_point - Mount point path

  Return:
    0 on success, -1 on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_device_unmount(const char *mount_point)
{
  int result;

  if (NULL == mount_point)
  {
    return -1;
  }

  // Synchronize any pending writes
  yaffs_sync(mount_point);

  // Unmount filesystem
  result = yaffs_unmount(mount_point);

  if (result >= 0)
  {
    g_yaffs_nor_device.is_mounted = 0;
  }

  return result;
}

/*-----------------------------------------------------------------------------------------------------
  Force garbage collection cycle

  Manually triggers garbage collection to reclaim space from deleted files.
  This allows deterministic control over when GC occurs, avoiding unpredictable
  delays during normal file operations.

  Parameters:
    mount_point - Mount point path
    urgency     - GC urgency level (0=passive, 1=normal, 2=aggressive)

  Return:
    Number of blocks collected, -1 on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_device_garbage_collect(const char *mount_point, int urgency)
{
  struct yaffs_dev *dev;
  int               blocks_collected = 0;
  int               total_collected  = 0;
  int               iterations       = 0;
  int               max_iterations;

  if (NULL == mount_point)
  {
    return -1;
  }

  dev = yaffs_getdev(mount_point);
  if (NULL == dev)
  {
    return -1;
  }

  // Force filesystem sync before GC
  yaffs_sync(mount_point);

  // Set maximum iterations based on urgency
  switch (urgency)
  {
    case YAFFS_GC_PASSIVE:
      max_iterations = 1;
      break;
    case YAFFS_GC_NORMAL:
      max_iterations = 3;
      break;
    case YAFFS_GC_AGGRESSIVE:
      max_iterations = 10;
      break;
    default:
      max_iterations = 1;
      break;
  }

  // Perform garbage collection in iterations
  do
  {
    blocks_collected = yaffs_do_background_gc_reldev(dev, urgency);

    if (blocks_collected > 0)
    {
      total_collected += blocks_collected;
    }

    iterations++;

    // Continue if we collected blocks and haven't reached max iterations
  } while (blocks_collected > 0 && iterations < max_iterations);

  return total_collected;
}

/*-----------------------------------------------------------------------------------------------------
  Get YAFFS2 device pointer

  Returns the device structure for advanced operations.

  Return:
    Pointer to YAFFS2 device structure
-----------------------------------------------------------------------------------------------------*/
struct yaffs_dev *Yaffs_nor_get_device(void)
{
  return &g_yaffs_nor_device;
}

/*-----------------------------------------------------------------------------------------------------
  Check if filesystem is mounted

  Return:
    1 if mounted, 0 if not mounted
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_is_mounted(void)
{
  return g_yaffs_nor_device.is_mounted;
}

/*-----------------------------------------------------------------------------------------------------
  Automatic periodic garbage collection

  Performs intelligent garbage collection based on filesystem usage patterns.
  Recommended to call this function periodically from a low-priority task.

  Parameters:
    mount_point - Mount point path
    usage_threshold - Usage percentage threshold (0-100) above which GC is triggered

  Return:
    Number of blocks collected, -1 on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_device_auto_gc(const char *mount_point, uint32_t usage_threshold)
{
  Y_LOFF_T free_space;
  uint64_t total_space;
  uint32_t usage_percent;
  int      urgency;
  int      blocks_collected = 0;

  if (NULL == mount_point || usage_threshold > 100)
  {
    return -1;
  }

  // Check if filesystem is mounted
  if (!Yaffs_nor_is_mounted())
  {
    return -1;
  }

  // Get current free space
  free_space = yaffs_freespace(mount_point);
  if (free_space < 0)
  {
    return -1;
  }

  // Calculate usage percentage (using estimated total space)
  total_space = 32 * 1024 * 1024;  // 32MB estimated (adjust based on your NOR Flash size)
  if (total_space > 0)
  {
    uint64_t used_space = total_space - free_space;
    usage_percent       = (uint32_t)((used_space * 100) / total_space);
  }
  else
  {
    return -1;
  }

  // Determine GC urgency based on usage and threshold
  if (usage_percent < usage_threshold)
  {
    // Below threshold - no GC needed
    return 0;
  }
  else if (usage_percent < usage_threshold + 10)
  {
    urgency = YAFFS_GC_PASSIVE;
  }
  else if (usage_percent < usage_threshold + 20)
  {
    urgency = YAFFS_GC_NORMAL;
  }
  else
  {
    urgency = YAFFS_GC_AGGRESSIVE;
  }

  // Perform garbage collection
  blocks_collected = Yaffs_nor_device_garbage_collect(mount_point, urgency);

  return blocks_collected;
}
