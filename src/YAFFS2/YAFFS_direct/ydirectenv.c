/*
 * YAFFS: Yet another Flash File System . A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2018 Aleph One Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 */

#include "App.h"
#include "ydirectenv.h"

/*-----------------------------------------------------------------------------------------------------
  Get current time for YAFFS2 filesystem operations

  Parameters:
    None

  Return:
    Current time as 32-bit timestamp (seconds since epoch)
-----------------------------------------------------------------------------------------------------*/
u32 yaffsfs_CurrentTime(void)
{
  // For embedded systems without RTC, return a fixed timestamp
  // You can implement actual RTC reading here if available
  // This represents a timestamp from ~2021 year
  return 0x60000000;
}
