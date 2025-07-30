#ifndef YAFFS_NOR_DEVICE_H
#define YAFFS_NOR_DEVICE_H

#include "App.h"

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 Device Management Interface for NOR Flash

  This header provides high-level device management functions for initializing,
  mounting, and controlling a YAFFS2 filesystem on NOR Flash memory.
-----------------------------------------------------------------------------------------------------*/

// Forward declaration
struct yaffs_dev;

/*-----------------------------------------------------------------------------------------------------
  Device Initialization and Management
-----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------------
  Initialize YAFFS2 device for NOR Flash

  Prepares the YAFFS2 device structure and registers it with the YAFFS2 system.
  Must be called before mounting the filesystem.

  Return:
    0 on success, -1 on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_device_init(void);

/*-----------------------------------------------------------------------------------------------------
  Mount YAFFS2 filesystem on NOR Flash

  Attempts to mount the filesystem. If mounting fails (e.g., unformatted flash),
  the function will format the flash and retry mounting.

  Parameters:
    mount_point - Mount point path (e.g., "/norflash")

  Return:
    0 on success, -1 on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_device_mount(const char *mount_point);

/*-----------------------------------------------------------------------------------------------------
  Unmount YAFFS2 filesystem

  Cleanly unmounts the filesystem, ensuring all cached data is written to flash.

  Parameters:
    mount_point - Mount point path

  Return:
    0 on success, -1 on error
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_device_unmount(const char *mount_point);

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
int Yaffs_nor_device_garbage_collect(const char *mount_point, int urgency);

/*-----------------------------------------------------------------------------------------------------
  Get YAFFS2 device pointer

  Returns the device structure for advanced operations.

  Return:
    Pointer to YAFFS2 device structure
-----------------------------------------------------------------------------------------------------*/
struct yaffs_dev* Yaffs_nor_get_device(void);

/*-----------------------------------------------------------------------------------------------------
  Check if filesystem is mounted

  Return:
    1 if mounted, 0 if not mounted
-----------------------------------------------------------------------------------------------------*/
int Yaffs_nor_is_mounted(void);

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
int Yaffs_nor_device_auto_gc(const char *mount_point, uint32_t usage_threshold);

/*-----------------------------------------------------------------------------------------------------
  Convenience macros for common operations
-----------------------------------------------------------------------------------------------------*/

// Default mount point for NOR Flash filesystem
#define YAFFS_NOR_MOUNT_POINT    "/norflash"

// Garbage collection urgency levels
#define YAFFS_GC_PASSIVE         (0)    // Collect only when necessary
#define YAFFS_GC_NORMAL          (1)    // Normal garbage collection (3 iterations max)
#define YAFFS_GC_AGGRESSIVE      (2)    // Aggressive garbage collection (10 iterations max)

/*-----------------------------------------------------------------------------------------------------
  Important Notes about YAFFS2 Garbage Collection:

  1. YAFFS2 GC works at block level, not file level
  2. A block can only be erased if ALL chunks in the block are deleted
  3. If even one file chunk remains in a block, the entire block stays allocated
  4. GC moves live data from partially filled blocks to consolidate free space
  5. Deleted files may not immediately free space until GC runs
  6. Use yaffs_sync() before GC to ensure all pending writes are committed

  Recommended usage patterns:
  - Run NORMAL GC after deleting many files
  - Run AGGRESSIVE GC when free space is critically low
  - Use AUTO GC for periodic maintenance in background tasks
-----------------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------------
  Example usage pattern:

  // Initialize and mount filesystem
  if (Yaffs_nor_device_init() == 0)
  {
    if (Yaffs_nor_device_mount(YAFFS_NOR_MOUNT_POINT) == 0)
    {
      // Filesystem ready for use
      // Use standard POSIX file operations with paths like "/norflash/myfile.txt"

      // Manual garbage collection (call when needed)
      Yaffs_nor_device_garbage_collect(YAFFS_NOR_MOUNT_POINT, YAFFS_GC_NORMAL);

      // Automatic periodic maintenance (call from low-priority task)
      // Triggers GC only when usage exceeds 70%
      Yaffs_nor_device_auto_gc(YAFFS_NOR_MOUNT_POINT, 70);
    }
  }

  // At shutdown
  Yaffs_nor_device_unmount(YAFFS_NOR_MOUNT_POINT);
-----------------------------------------------------------------------------------------------------*/

#endif // YAFFS_NOR_DEVICE_H
