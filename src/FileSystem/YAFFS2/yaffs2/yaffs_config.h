/*-----------------------------------------------------------------------------------------------------
  YAFFS2 Configuration Header

  This file configures YAFFS2 for operation with NOR Flash and ThreadX RTOS
-----------------------------------------------------------------------------------------------------*/

#ifndef YAFFS_CONFIG_H
#define YAFFS_CONFIG_H

#include "App.h"

/*
 * YAFFS2 Compile-time Configuration
 */

/* Basic YAFFS configuration */
#define CONFIG_YAFFS_YAFFS2                  1    // Enable YAFFS2 mode
#define CONFIG_YAFFS_DOES_ECC                0    // We handle ECC externally
#define CONFIG_YAFFS_ECC_WRONG_ORDER         0    // ECC byte order
#define CONFIG_YAFFS_DIRECT                  1    // Direct interface mode

/* Threading and locking */
#define CONFIG_YAFFS_WINCE                   0    // Not WinCE
#define CONFIG_YAFFS_PROVIDE_DEFS            1    // Provide basic definitions
#define CONFIG_YAFFS_DEFINES_TYPES           1    // Define basic types

/* Memory allocation configuration */
#define CONFIG_YAFFS_DIRECT_USE_OS_MALLOC    0    // Use our custom malloc
#define YMALLOC(x)                           App_malloc(x)
#define YFREE(x)                             App_free(x)

/* File system features */
#define CONFIG_YAFFS_SHORT_NAMES_IN_RAM      1    // Keep short names in RAM
#define CONFIG_YAFFS_CASE_INSENSITIVE        0    // Case sensitive names
#define CONFIG_YAFFS_UNICODE                 0    // ASCII only
#define CONFIG_YAFFS_XATTR                   0    // No extended attributes

/* Checkpointing and summaries */
#define CONFIG_YAFFS_AUTO_CHECKPT            1    // Enable auto checkpointing
#define CONFIG_YAFFS_DISABLE_LAZY_LOAD       0    // Allow lazy loading
#define CONFIG_YAFFS_DISABLE_WIDE_TNODES     0    // Use wide tnodes for large files

/* Background operations */
#define CONFIG_YAFFS_BACKGROUND_DELETE       1    // Background deletion
#define CONFIG_YAFFS_DISABLE_BACKGROUND_GC   0    // Allow background GC

/* NOR Flash specific */
#define CONFIG_YAFFS_ALWAYS_CHECK_CHUNK_ERASED 1  // Always verify erased chunks
#define CONFIG_YAFFS_DISABLE_CHUNK_ERASED_CHECK 0 // Don't skip erase check

/* Debugging and verification */
#define CONFIG_YAFFS_ENABLE_TRACING          1    // Enable trace output
#define CONFIG_YAFFS_PARANOID                1    // Extra paranoid checking
#define CONFIG_YAFFS_DISABLE_VERIFY          0    // Enable verification

/* Platform specific */
#define YCHAR                                char  // Character type
#define YUCHAR                               unsigned char

/* Basic type definitions */
#ifndef u8
#define u8                                   uint8_t
#endif

#ifndef u16
#define u16                                  uint16_t
#endif

#ifndef u32
#define u32                                  uint32_t
#endif

#ifndef s8
#define s8                                   int8_t
#endif

#ifndef s16
#define s16                                  int16_t
#endif

#ifndef s32
#define s32                                  int32_t
#endif

#ifndef loff_t
#define loff_t                               long long
#endif

/* Include YAFFS2 headers with correct paths */
#include "../../YAFFS2/YAFFS_direct/yaffs_guts.h"
#include "../../YAFFS2/YAFFS_direct/yaffs_trace.h"

/* Trace configuration */
#define T(mask, p)                           do { if (yaffs_trace_mask & (mask)) printf p; } while (0)
#define TSTR(x)                              x
#define TENDSTR                              "\n"

/* Trace masks */
extern unsigned int yaffs_trace_mask;

#define YAFFS_TRACE_OS                       0x00000002
#define YAFFS_TRACE_ALLOCATE                 0x00000004
#define YAFFS_TRACE_SCAN                     0x00000008
#define YAFFS_TRACE_BAD_BLOCKS               0x00000010
#define YAFFS_TRACE_ERASE                    0x00000020
#define YAFFS_TRACE_GC                       0x00000040
#define YAFFS_TRACE_WRITE                    0x00000080
#define YAFFS_TRACE_TRACING                  0x00000100
#define YAFFS_TRACE_DELETION                 0x00000200
#define YAFFS_TRACE_BUFFERS                  0x00000400
#define YAFFS_TRACE_NANDACCESS               0x00000800
#define YAFFS_TRACE_GC_DETAIL                0x00001000
#define YAFFS_TRACE_SCAN_DEBUG               0x00002000
#define YAFFS_TRACE_MTD                      0x00004000
#define YAFFS_TRACE_CHECKPOINT               0x00008000
#define YAFFS_TRACE_VERIFY                   0x00010000
#define YAFFS_TRACE_VERIFY_NAND              0x00020000
#define YAFFS_TRACE_VERIFY_FULL              0x00040000
#define YAFFS_TRACE_VERIFY_ALL               0x000F0000
#define YAFFS_TRACE_SYNC                     0x00100000
#define YAFFS_TRACE_BACKGROUND               0x00200000
#define YAFFS_TRACE_LOCK                     0x00400000
#define YAFFS_TRACE_MOUNT                    0x00800000
#define YAFFS_TRACE_ERROR                    0x40000000
#define YAFFS_TRACE_BUG                      0x80000000
#define YAFFS_TRACE_ALWAYS                   0xF0000000

#endif // YAFFS_CONFIG_H
