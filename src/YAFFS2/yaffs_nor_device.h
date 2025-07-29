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
  Convenience macros for common operations
-----------------------------------------------------------------------------------------------------*/

// Default mount point for NOR Flash filesystem
#define YAFFS_NOR_MOUNT_POINT    "/norflash"

// Garbage collection urgency levels
#define YAFFS_GC_PASSIVE         (0)    // Collect only when necessary
#define YAFFS_GC_NORMAL          (1)    // Normal garbage collection
#define YAFFS_GC_AGGRESSIVE      (2)    // Aggressive garbage collection

/*-----------------------------------------------------------------------------------------------------
  Example usage pattern:

  // Initialize and mount filesystem
  if (Yaffs_nor_device_init() == 0)
  {
    if (Yaffs_nor_device_mount(YAFFS_NOR_MOUNT_POINT) == 0)
    {
      // Filesystem ready for use
      // Use standard POSIX file operations with paths like "/norflash/myfile.txt"

      // Periodic maintenance (call from low-priority task)
      Yaffs_nor_device_garbage_collect(YAFFS_NOR_MOUNT_POINT, YAFFS_GC_NORMAL);
    }
  }

  // At shutdown
  Yaffs_nor_device_unmount(YAFFS_NOR_MOUNT_POINT);
-----------------------------------------------------------------------------------------------------*/

#endif // YAFFS_NOR_DEVICE_H
