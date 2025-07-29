/*-----------------------------------------------------------------------------------------------------
  YAFFS2 NOR Flash Adapter Header

  Adapter interface between YAFFS2 and MC80 OSPI NOR Flash driver
-----------------------------------------------------------------------------------------------------*/

#ifndef YAFFS_NOR_ADAPTER_H
#define YAFFS_NOR_ADAPTER_H

#include "App.h"
#include "yaffs_nor_config.h"
#include "yaffs_guts.h"
#include "yaffsfs.h"

/*-----------------------------------------------------------------------------------------------------
  YAFFS2 callback function prototypes for NOR Flash operations
-----------------------------------------------------------------------------------------------------*/

// Write chunk with tags to NOR Flash
int Yaffs_nor_write_chunk_tags(struct yaffs_dev *dev, int chunk_id, const unsigned char *data, const struct yaffs_ext_tags *tags);

// Read chunk with tags from NOR Flash
int Yaffs_nor_read_chunk_tags(struct yaffs_dev *dev, int chunk_id, unsigned char *data, struct yaffs_ext_tags *tags);

// Erase block in NOR Flash
int Yaffs_nor_erase_block(struct yaffs_dev *dev, int block_no);

// Mark block as bad (for wear leveling)
int Yaffs_nor_mark_bad_block(struct yaffs_dev *dev, int block_no);

// Check if block is bad
int Yaffs_nor_check_bad_block(struct yaffs_dev *dev, int block_no);

// Initialize NOR Flash driver
int Yaffs_nor_initialise(struct yaffs_dev *dev);

// Deinitialize NOR Flash driver
int Yaffs_nor_deinitialise(struct yaffs_dev *dev);

#endif  // YAFFS_NOR_ADAPTER_H
