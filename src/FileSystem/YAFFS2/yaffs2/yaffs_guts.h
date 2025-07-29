/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2018 Aleph One Ltd.
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __YAFFS_GUTS_H__
#define __YAFFS_GUTS_H__

#include "yaffs_config.h"

/*
 * This is the main YAFFS header file. It contains all the core
 * data structures and function prototypes.
 */

/* Some special object id values */
#define YAFFS_OBJECTID_ROOT             1
#define YAFFS_OBJECTID_LOSTNFOUND       2
#define YAFFS_OBJECTID_UNLINKED         3
#define YAFFS_OBJECTID_DELETED          4

/*
 * YAFFS_MAX_CHUNK_ID is the maximum chunk id value that we can handle.
 * We limit this to 1MB chunks.
 */
#define YAFFS_MAX_CHUNK_ID              0x000FFFFF

#define YAFFS_UNUSED(x)                 ((void)(x))

/* Special structure for passing through to mtd */
struct yaffs_nand_geometry {
    unsigned int data_bytes_per_chunk;
    unsigned int spare_bytes_per_chunk;
    unsigned int chunks_per_block;
    unsigned int blocks_per_lun;
    unsigned int number_of_luns;
};

typedef enum {
    YAFFS_OBJECT_TYPE_UNKNOWN,
    YAFFS_OBJECT_TYPE_FILE,
    YAFFS_OBJECT_TYPE_SYMLINK,
    YAFFS_OBJECT_TYPE_DIRECTORY,
    YAFFS_OBJECT_TYPE_HARDLINK,
    YAFFS_OBJECT_TYPE_SPECIAL
} yaffs_obj_type;

enum yaffs_guts_event {
    YAFFS_LOW_LEVEL_FORMAT,
    YAFFS_REMOVE_OBJECT_HEADER
};

typedef enum yaffs_ecc_result {
    YAFFS_ECC_RESULT_UNKNOWN,
    YAFFS_ECC_RESULT_NO_ERROR,
    YAFFS_ECC_RESULT_FIXED,
    YAFFS_ECC_RESULT_UNFIXED
} yaffs_ecc_result;

typedef enum {
    YAFFS_BLOCK_STATE_UNKNOWN = 0,
    YAFFS_BLOCK_STATE_SCANNING,
    YAFFS_BLOCK_STATE_NEEDS_SCAN,
    YAFFS_BLOCK_STATE_EMPTY,
    YAFFS_BLOCK_STATE_ALLOCATING,
    YAFFS_BLOCK_STATE_FULL,
    YAFFS_BLOCK_STATE_DIRTY,
    YAFFS_BLOCK_STATE_CHECKPOINT,
    YAFFS_BLOCK_STATE_COLLECTING,
    YAFFS_BLOCK_STATE_DEAD
} yaffs_block_state_t;

#define YAFFS_NUMBER_OF_BLOCK_STATES (YAFFS_BLOCK_STATE_DEAD + 1)

/* Block info structure */
struct yaffs_block_info {
    int soft_del_pages:12;      /* number of soft deleted pages */
    int pages_in_use:12;        /* number of pages in use */
    unsigned block_state:4;     /* One of the above block states */
    u32 needs_retiring:1;       /* Data has failed on this block, need to get valid data off */
    u32 skip_erased_check:1;    /* Don't check the erased check as we know the block is empty */
    u32 gc_prioritise:1;        /* An ECC check or blank check has failed.
                                   Block should be prioritised for GC */
    u32 chunk_error_strikes:3;  /* How many times we've had ecc etc failures on this block and tried to reuse it */
    u32 has_shrink_hdr:1;       /* This block has at least one shrink header */
    u32 seq_number;             /* block sequence number for yaffs2 */
};

/* Extended tags structures for YAFFS2 */
enum yaffs_ecc_result;

typedef struct {
    unsigned validity0;
    unsigned chunk_id;
    unsigned obj_id;
    unsigned n_bytes;
    unsigned validity1;
} yaffs_packed_tags1;

typedef struct {
    u32 chunk_id;
    u32 obj_id;
    u32 n_bytes;
    u32 validity;
    u32 block_bad;
    u32 extra_available;
    u32 extra_parent_id;
    u32 extra_is_shrink;
    u32 extra_shadows;
    u32 extra_obj_type;
    yaffs_ecc_result ecc_result;
    int extra_length;
} yaffs_ext_tags;

/* Spare structure for YAFFS1 */
typedef struct {
    u8 tb0;
    u8 tb1;
    u8 tb2;
    u8 tb3;
    u8 page_status; /* set to 0 to delete the chunk */
    u8 block_status;
    u8 tb6;
    u8 tb7;
} yaffs_spare;

struct yaffs_nand_geometry;

/* Forward declarations */
struct yaffs_obj;
struct yaffs_dev;

typedef struct yaffs_obj yaffs_obj_t;
typedef struct yaffs_dev yaffs_dev_t;

typedef int (*yaffs_write_chunk_tags_fn) (struct yaffs_dev *dev,
                                         int nand_chunk,
                                         const u8 *data,
                                         const yaffs_ext_tags *tags);

typedef int (*yaffs_read_chunk_tags_fn) (struct yaffs_dev *dev,
                                        int nand_chunk,
                                        u8 *data,
                                        yaffs_ext_tags *tags);

typedef int (*yaffs_erase_fn) (struct yaffs_dev *dev, int block_no);

typedef int (*yaffs_initialise_fn) (struct yaffs_dev *dev);
typedef int (*yaffs_deinitialise_fn) (struct yaffs_dev *dev);

typedef int (*yaffs_bad_block_fn) (struct yaffs_dev *dev, int block_no);
typedef int (*yaffs_query_block_fn) (struct yaffs_dev *dev,
                                     int block_no,
                                     yaffs_block_state_t *state,
                                     u32 *seq_number);

/* Function to manipulate block info */
static inline yaffs_block_state_t yaffs_get_block_state(yaffs_dev_t *dev, int blk)
{
    if (blk < dev->internal_start_block || blk > dev->internal_end_block)
        return YAFFS_BLOCK_STATE_UNKNOWN;
    return dev->block_info[blk - dev->internal_start_block].block_state;
}

static inline void yaffs_set_block_state(yaffs_dev_t *dev, int blk,
                                        yaffs_block_state_t state)
{
    if (blk >= dev->internal_start_block && blk <= dev->internal_end_block)
        dev->block_info[blk - dev->internal_start_block].block_state = state;
}

/* Object structure */
struct yaffs_obj {
    u8 deleted:1;               /* This should only apply to unlinked files. */
    u8 soft_del:1;              /* it has also been soft deleted */
    u8 unlinked:1;              /* An unlinked file.*/
    u8 fake:1;                  /* A fake object has no presence on NAND. */
    u8 rename_allowed:1;        /* Some objects cannot be renamed. */
    u8 unlink_allowed:1;
    u8 dirty:1;                 /* the object needs to be written to flash */
    u8 valid:1;                 /* When the file system is being loaded up, this
                                 * object might be created before the data
                                 * is available
                                 * ie. file data chunks encountered before the header.
                                 */
    u8 lazy_loaded:1;           /* This object has been lazy loaded and is missing some detail */

    u8 defered_free:1;          /* Object is removed from NAND, but is
                                 * still in the inode cache.
                                 * Free of object is defered.
                                 * until the inode is released.
                                 */
    u8 being_created:1;         /* This object is still being created so skip some verification checks. */
    u8 is_shadowed:1;           /* This object is shadowed on the way to being renamed. */

    u8 xattr_known:1;           /* We know if this object has xattribs or not. */
    u8 has_xattr:1;             /* This object has xattribs.
                                 * Only valid if xattr_known. */

    u8 serial;                  /* serial number of chunk in NAND.*/
    u16 sum;                    /* checksum of name. */
    struct yaffs_dev *my_dev;   /* The device I'm on */

    struct list_head hash_link; /* list of objects in hash bucket */

    struct list_head hard_links; /* hard linked object chain*/

    /* directory structure stuff */
    /* also used for linking up the free list */
    struct yaffs_obj *parent;
    struct list_head siblings;
    struct list_head children;

    /* Where's my object header in NAND? */
    int hdr_chunk;
    int n_data_chunks;          /* Number of data chunks attached to the file. */

    u32 obj_id;                 /* the object id value */

    u32 yst_mode;

#ifdef CONFIG_YAFFS_SHORT_NAMES_IN_RAM
    YCHAR short_name[YAFFS_SHORT_NAME_LENGTH + 1];
#endif

#ifdef CONFIG_YAFFS_WINCE
    u32 win_ctime[2];
    u32 win_mtime[2];
    u32 win_atime[2];
#else
    u32 yst_uid;
    u32 yst_gid;
    u32 yst_atime;
    u32 yst_mtime;
    u32 yst_ctime;
#endif

    u32 yst_rdev;

    void *my_inode;

    yaffs_obj_type variant_type;

    union {
        struct yaffs_file_var file_variant;
        struct yaffs_dir_var dir_variant;
        struct yaffs_symlink_var symlink_variant;
        struct yaffs_hardlink_var hardlink_variant;
    } variant;
};

/* Maximum number of NAND chunks allowed for an object's data */
#define YAFFS_MAX_FILE_SIZE     (0x7FFFFFFF)

struct yaffs_file_var {
    loff_t file_size;
    loff_t stored_size;         /* The size of the file stored in the object */
    u32 top_level;
    u32 *file_map;              /* Array of chunk ids */
};

struct yaffs_dir_var {
    struct list_head children;  /* The objects in this directory */
    struct list_head dirty;     /* Entry for List of dirty directories */
    s32 assoc_obj_id;           /* For a directory (YAFFS_OBJECT_TYPE_DIRECTORY), this is the
                                 * obj_id of the associated object
                                 */
};

struct yaffs_symlink_var {
    YCHAR *alias;
};

struct yaffs_hardlink_var {
    struct yaffs_obj *equiv_obj;
    u32 equiv_id;
};

/* The device structure */
struct yaffs_dev {
    struct list_head dev_list;
    const char *name;

    /* Entry parameters set up way early. Yaffs sets up the rest.*/
    int data_bytes_per_chunk;   /* Should be a power of 2 >= 512 */
    int chunks_per_block;       /* does not need to be a power of 2 */
    int spare_bytes_per_chunk;  /* spare area size */
    int start_block;            /* Start block we're allowed to use */
    int end_block;              /* End block we're allowed to use */
    int n_reserved_blocks;      /* Tuneable so that we can reduce reserved blocks on NOR and RAM. */

    int n_caches;               /* If <= 0, then short op caching is disabled,
                                 * else the number of short op caches.
                                 */
    int use_header_file_size;   /* Flag to determine if we should use
                                 * file sizes from the header
                                 */
    int disable_lazy_load;      /* Disable lazy loading on this device */
    int wide_tnodes;            /* Set to disable wide tnodes */
    int disable_soft_del;       /* yaffs 1 only: Set to disable the use of softdeletion. */

    int defered_dir_update;     /* Set to defer directory updates */

#ifdef CONFIG_YAFFS_XATTR
    int max_xattr_size;
#endif

    /* Stuff used by the shared space checkpointing mechanism */
    /* If this value is zero, then this mechanism is disabled */

    int checkpoint_blocks;
    int checkpoint_max_blocks;

    /* Stuff used by the wide tnodes mechanism */
    u32 tnodes_created;

    /* Stuff for background deletion and unlinked files.*/
    int defer_dir_update;       /* Set to defer directory updates */
    int unlinked_deletion;      /* Background deletion of unlinked files */
    int delete_dir;             /* Background deletion of whole directories */

    u32 *gc_cleanup_list;       /* objects to delete at the end of a GC. */
    u32 n_clean_ups;

    unsigned has_pending_prioritised_gc; /* We think this device might
                                         have pending prioritised gcs */
    unsigned gc_disable;
    unsigned gc_block_finder;
    unsigned gc_dirtiest;
    unsigned gc_pages_in_use;
    unsigned gc_not_done;
    unsigned gc_block;          /* Next block to look at for GC */
    unsigned gc_chunk;          /* Next chunk to look at for GC */
    unsigned gc_skip;

    struct yaffs_block_info *block_info;
    u8 *chunk_bits;             /* bitmap of chunks in use */
    unsigned chunk_bit_stride;  /* Number of bytes of chunk_bits per block.
                                 * Must be consistent with chunks_per_block.
                                 */

    int n_erased_blocks;
    int alloc_block;            /* Current block being allocated from */
    u32 alloc_page;
    int alloc_block_finder;     /* Used to search for next allocation block */

    /* Object and Tnode memory management */
    void *allocator;
    int n_obj;
    int n_tnodes;

    int n_hardlinks;

    struct yaffs_obj_bucket obj_bucket[YAFFS_NOBJECT_BUCKETS];
    u32 bucket_finder;

    int n_free_chunks;

    /* Stuff for deciding when to do background GC */
    unsigned background_gcs;    /* Count of background GCs performed */
    unsigned oldest_dirty_gc_count;
    unsigned oldest_dirty_block;

    /* Temporary buffer management */
    struct yaffs_buffer temp_buffer[YAFFS_N_TEMP_BUFFERS];
    int max_temp;
    int temp_in_use;
    int unmanaged_buffer_allocs;
    int unmanaged_buffer_deallocs;

    /* yaffs2 runtime stuff */
    unsigned seq_number;        /* Sequence number of currently
                                 * allocating block */
    unsigned oldest_dirty_seq;
    unsigned highest_seq;

    /* Block refreshing */
    int refresh_skip;           /* A skip down counter.
                                 * Refresh happens when this gets to zero. */

    /* Dirty directory handling */
    struct list_head dirty_dirs; /* List of dirty directories */

    /* Summary */
    int chunks_per_summary;
    struct yaffs_summary_tags *sum_tags;

    /* Statistics */
    u32 n_page_writes;
    u32 n_page_reads;
    u32 n_erasures;
    u32 n_bad_queries;
    u32 n_bad_markings;
    u32 n_erase_failures;
    u32 n_gc_copies;
    u32 n_gc_retries;
    u32 n_retired_writes;
    u32 n_retired_blocks;

    u32 n_ecc_fixed;
    u32 n_ecc_unfixed;
    u32 n_tags_ecc_fixed;
    u32 n_tags_ecc_unfixed;
    u32 n_deletions;
    u32 n_unmarked_deletions;
    u32 refresh_count;
    u32 cache_hits;

    /* The remove object callback function must be supplied by OS glue */
    void (*remove_obj_fn) (struct yaffs_obj *obj);

    /* Callback to mark the superblock dirty */
    void (*sb_dirty_fn) (struct yaffs_dev *dev);

    /*  Callback to control wear levelling. */
    unsigned (*select_gc_fn) (struct yaffs_dev *dev,
                              unsigned urgency,
                              unsigned *prioritised);

    int is_mounted;
    int read_only;
    int is_checkpointed;

    /* Stuff to support wide tnodes */
    u32 tnode_width;
    u32 tnode_mask;
    u32 tnode_size;

    /* Stuff for figuring out file offset to chunk conversions */
    u32 chunk_shift; /* Shift value */
    u32 chunk_div;   /* Divisor after shifting: 1 for 2^n sizes */
    u32 chunk_mod;   /* Remainder after shifting */

    /* Stuff to handle inband tags */
    int inband_tags;
    u32 tags_9bytes;

    struct mtd_info *mtd;
    void *os_context;

    struct yaffs_param param;

    /* Context for the generic device to pass back to device specific code */
    void *driver_context;

    struct list_head search_contexts;

    void (*put_super_fn) (struct yaffs_dev *dev);

    int is_super_dirty;
    int gc_in_progress;
    unsigned gc_max_copies;

    /* Non-wide tnode stuff */
    struct yaffs_tnode *tn_map[YAFFS_NTNODES_LEVEL0];
    struct yaffs_obj *root_dir;
    struct yaffs_obj *lost_n_found;

    int ll_init;
    /* The checkpointBuffer is used to hold spare pages loaded during
     * checkpoint read and some header pages when writing a checkpoint.
     */
    void *checkpt_buffer;
    int checkpt_page_seq;
    int checkpt_byte_count;
    int checkpt_byte_offs;
    u8 checkpt_sum;
    int checkpt_xor;

    int checkpoint_blocks_required; /* Number of blocks needed to store
                                     * current checkpoint set
                                     */

    /* Block Info */
    struct yaffs_block_info *block_info;
    u8 *chunk_bits;         /* bitmap of chunks in use */
    unsigned chunk_bit_stride;  /* Number of bytes of chunk_bits per block.
                                 * Must be consistent with chunks_per_block.
                                 */

    int n_erased_blocks;
    int alloc_block;        /* Current block being allocated from */
    u32 alloc_page;
    int alloc_block_finder; /* Used to search for next allocation block */

    /* Runtime state */
    int n_tweaks;
    int is_mounted;
    int read_only;
    int is_checkpointed;

    /* Stuff to handle inband tags */
    int inband_tags;
    u32 tags_9bytes;

    /* Callback functions */
    yaffs_write_chunk_tags_fn write_chunk_tags_fn;
    yaffs_read_chunk_tags_fn read_chunk_tags_fn;
    yaffs_erase_fn erase_fn;
    yaffs_initialise_fn initialise_fn;
    yaffs_deinitialise_fn deinitialise_fn;
    yaffs_bad_block_fn bad_block_fn;
    yaffs_query_block_fn query_block_fn;
};

typedef struct yaffs_param {
    const YCHAR *name;

    /*
     * Entry parameters set up way early. Yaffs sets up the rest.
     * The structure should be zeroed out before use so that unused
     * and backward compatibility fields are zero.
     */

    /*
     * Data geometry parameters: These parameters that relate to the NAND
     * and must be set up by the caller.
     */
    u32 total_bytes_per_chunk;      /* Should be >= 512, does not need to be a power of 2 */
    u32 chunks_per_block;           /* does not need to be a power of 2 */
    u32 spare_bytes_per_chunk;      /* spare area size */
    u32 start_block;                /* Start block we're allowed to use */
    u32 end_block;                  /* End block we're allowed to use */
    u32 n_reserved_blocks;          /* Tuneable so that we can reduce reserved blocks on NOR and RAM. */

    /*
     * Device control parameters.
     */
    u32 n_caches;                   /* If <= 0, then short op caching is disabled, else
                                     * the number of short op caches (don't use too many).
                                     * 10 to 20 is a good bet.
                                     */
    u32 use_header_file_size;       /* Flag to determine if we should use file sizes from the header */
    u32 disable_lazy_load;          /* Disable lazy loading on this device */
    u32 wide_tnodes;                /* Set to disable wide tnodes */
    u32 disable_soft_del;           /* yaffs 1 only: Set to disable the use of softdeletion. */

    u32 no_tags_ecc;                /* Flag to decide whether or not to do ECC on tags */

    u32 refresh_period;             /* How often to check for a block refresh */

    /*
     * Checkpoint control parameters
     */
    u32 disable_checkpt;            /* Set to disable checkpointing on this device */

    /*
     * Directory update handling
     */
    u32 empty_lost_n_found;         /* Auto-empty lost+found directory on mount */
    u32 disable_summary;

    /*
     * Runtime control parameters (callback functions)
     */
    yaffs_write_chunk_tags_fn write_chunk_tags_fn;
    yaffs_read_chunk_tags_fn read_chunk_tags_fn;
    yaffs_erase_fn erase_fn;
    yaffs_bad_block_fn bad_block_fn;
    yaffs_initialise_fn initialise_fn;
    yaffs_deinitialise_fn deinitialise_fn;

    /*
     * Runtime control parameters (callback functions)
     * Also hooks for ECC handling.
     */
    int (*ecc_fix_fn)(struct yaffs_dev *dev, int chunk_id, u8 *data, u8 *spare);
    void (*verifycac_fn)(struct yaffs_obj *obj);
    void (*sb_dirty_fn)(struct yaffs_dev *dev);
    void (*remove_obj_fn)(struct yaffs_obj *obj);

    /*
     * Device context storage. This is device specific.
     * Linux uses this for mtd  stuff. Direct uses it for the
     * memory extent stuff.
     */
    void *driver_context;

    void *os_context;

    /* Spare context callback stuff. If set then these are called when the
     * spare data needs to be packed or unpacked. The caller will allocate
     * and free the spare memory.
     * NB Due to the way the tags are packed you cannot completely pack and
     * unpack if you round trip.
     */
    struct yaffs_nand_geometry geometry;
    int (*pack_spare_fn)(const yaffs_ext_tags *tags, void *spare_ptr);
    int (*unpack_spare_fn)(yaffs_ext_tags *tags, const void *spare_ptr);

    int is_yaffs2;

    /* The remove object callback function must be supplied by OS glue */
    void (*remove_obj_fn)(struct yaffs_obj *obj);

    /* Callback to mark the superblock dirty */
    void (*sb_dirty_fn)(struct yaffs_dev *dev);

    /* Callback to control wear levelling. */
    u32 (*select_gc_fn)(struct yaffs_dev *dev, u32 urgency, u32 *prioritised);

    /* Debug control flags. Don't use unless you know what you're doing */
    int use_asserts;
    int disable_summary;
    int disable_bad_block_marking;
    int disable_background_gc;
    int enable_xattr;

    /*
     * The following is for aborts during formatting.
     */
    int (*check_formatting_fn)(struct yaffs_dev *dev);

    int gc_control;
    int use_header_file_size;
    int disable_lazy_load;
    int wide_tnodes;
    int disable_soft_del;
    int defer_dir_update;
    int auto_unicode;
    int always_check_checkpt;
    int disable_summary;
    int disable_bad_block_marking;
    int disable_background_gc;
    int oldest_dirty_gc_count;
    int disable_soft_del;
    int refresh_period;
    int enable_xattr;
    int stored_endian;
    int disable_checkpt;
    int empty_lost_n_found;

} yaffs_param;

/* Function prototypes */
int yaffs_guts_initialise(struct yaffs_dev *dev);
void yaffs_deinitialise(struct yaffs_dev *dev);

int yaffs_get_n_free_chunks(struct yaffs_dev *dev);

int yaffs_rename_obj(struct yaffs_obj *old_dir, const YCHAR *old_name,
                    struct yaffs_obj *new_dir, const YCHAR *new_name);

int yaffs_unlinker(struct yaffs_obj *dir, const YCHAR *name);
int yaffs_del_obj(struct yaffs_obj *obj);

struct yaffs_obj *yaffs_get_equivalent_obj(struct yaffs_obj *obj);

void yaffs_guts_test(struct yaffs_dev *dev);

/* A few useful functions to be used within the core files*/
void yaffs_chunk_del(struct yaffs_dev *dev, int chunk_id, int mark_flash,
                    int lyn);
int yaffs_check_ff(u8 *buffer, int n_bytes);
void yaffs_handle_chunk_error(struct yaffs_dev *dev,
                             struct yaffs_block_info *bi);

u8 *yaffs_get_temp_buffer(struct yaffs_dev *dev);
void yaffs_release_temp_buffer(struct yaffs_dev *dev, u8 *buffer);

struct yaffs_obj *yaffs_find_by_name(struct yaffs_obj *the_dir,
                                     const YCHAR *name);
struct yaffs_obj *yaffs_find_by_number(struct yaffs_dev *dev, u32 number);

/* Link operations */
struct yaffs_obj *yaffs_link_obj(struct yaffs_obj *parent, const YCHAR *name,
                                 struct yaffs_obj *equiv_obj);

struct yaffs_obj *yaffs_get_hardlink_target(struct yaffs_obj *hard_link);

/* Symlink operations */
struct yaffs_obj *yaffs_create_symlink(struct yaffs_obj *parent,
                                       const YCHAR *name, u32 mode, u32 uid,
                                       u32 gid, const YCHAR *alias);
YCHAR *yaffs_get_symlink_alias(struct yaffs_obj *obj);

/* Special directories */
struct yaffs_obj *yaffs_create_special(struct yaffs_obj *parent,
                                       const YCHAR *name, u32 mode, u32 uid,
                                       u32 gid, u32 rdev);

int yaffs_set_attribs(struct yaffs_obj *obj, struct iattr *attr);
int yaffs_get_attribs(struct yaffs_obj *obj, struct iattr *attr);

/* File operations */
int yaffs_file_rd(struct yaffs_obj *obj, u8 *buffer, loff_t offset,
                 int n_bytes);
int yaffs_wr_file(struct yaffs_obj *obj, const u8 *buffer, loff_t offset,
                 int n_bytes, int write_through);
int yaffs_resize_file(struct yaffs_obj *obj, loff_t new_size);

struct yaffs_obj *yaffs_create_file(struct yaffs_obj *parent,
                                    const YCHAR *name, u32 mode, u32 uid,
                                    u32 gid);

int yaffs_flush_file(struct yaffs_obj *obj, int update_time, int data_sync);

/* Flushing and checkpointing */
void yaffs_flush_whole_cache(struct yaffs_dev *dev);

int yaffs_checkpoint_save(struct yaffs_dev *dev);
int yaffs_checkpoint_restore(struct yaffs_dev *dev);

/* Directory operations */
struct yaffs_obj *yaffs_create_dir(struct yaffs_obj *parent, const YCHAR *name,
                                   u32 mode, u32 uid, u32 gid);
struct yaffs_obj *yaffs_find_by_name(struct yaffs_obj *the_dir,
                                     const YCHAR *name);
struct yaffs_obj *yaffs_find_by_number(struct yaffs_dev *dev, u32 number);

/* Buffer management */
void yaffs_handle_defered_free(struct yaffs_obj *obj);
void yaffs_update_dirty_dirs(struct yaffs_dev *dev);

int yaffs_bg_gc(struct yaffs_dev *dev, unsigned urgency);

/* Tags and ECC functions */
int yaffs_tags_compat_wr(struct yaffs_dev *dev,
                        int nand_chunk,
                        const u8 *data, const yaffs_ext_tags *ext_tags);

int yaffs_tags_compat_rd(struct yaffs_dev *dev,
                        int nand_chunk,
                        u8 *data, yaffs_ext_tags *ext_tags);

int yaffs_block_bad(struct yaffs_dev *dev, int blk);

/* Robustification */
void yaffs_handle_chunk_error(struct yaffs_dev *dev, struct yaffs_block_info *bi);

/* BUG fn */
#define BUG() do { T(YAFFS_TRACE_BUG, (TSTR("==>> yaffs bug: " __FILE__ " %d" TENDSTR), __LINE__)); } while (0)

#ifndef YCHAR
#define YCHAR char
#endif

#ifndef YAFFS_LOSTNFOUND_NAME
#define YAFFS_LOSTNFOUND_NAME		"lost+found"
#endif

#ifndef YAFFS_LOSTNFOUND_PREFIX
#define YAFFS_LOSTNFOUND_PREFIX		"obj"
#endif

/* Constants */
#define YAFFS_OBJECT_SPACE      0x40000
#define YAFFS_MAX_NAME_LENGTH   255
#define YAFFS_MAX_ALIAS_LENGTH  159

#ifndef YAFFS_SHORT_NAME_LENGTH
#define YAFFS_SHORT_NAME_LENGTH 15
#endif

/* Some special object ids for pseudo objects */
#define YAFFS_OBJECTID_ROOT     1
#define YAFFS_OBJECTID_LOSTNFOUND  2
#define YAFFS_OBJECTID_UNLINKED 3
#define YAFFS_OBJECTID_DELETED  4

/* Pseudo object ids for checkpointing */
#define YAFFS_OBJECTID_SB_HEADER   0x10
#define YAFFS_OBJECTID_CHECKPOINT_DATA  0x20
#define YAFFS_OBJECTID_DATA     0x10000000

#define YAFFS_MAX_SHORT_OP_CACHES  20

#define YAFFS_N_TEMP_BUFFERS    6

#define YAFFS_SMALL_HOLE_THRESHOLD  4

/* Mode constants */
#define YAFFS_TNODES_LEVEL0      16
#define YAFFS_TNODES_LEVEL0_BITS 4
#define YAFFS_TNODES_LEVEL0_MASK 0xf

#define YAFFS_NTNODES_LEVEL0    (1<<YAFFS_TNODES_LEVEL0_BITS)

/* Constants for wide tnodes */
#define YAFFS_WIDE_TNODES_LEVEL0_BITS  16
#define YAFFS_WIDE_TNODES_LEVEL0_MASK  0xffff

/* Bucket constants */
#define YAFFS_NOBJECT_BUCKETS    256

/* Sequence numbers */
#define YAFFS_LOWEST_SEQUENCE_NUMBER 0x00001000
#define YAFFS_HIGHEST_SEQUENCE_NUMBER 0xefffff00

/* Special chunk ids for header and spare */
#define YAFFS_CHUNK_ID_HEADER    0

#define YAFFS_ALLOCATION_NOBJECTS 100
#define YAFFS_ALLOCATION_NTNODES  100
#define YAFFS_ALLOCATION_NLINKS   100

#define YAFFS_NOBJECT_BUCKETS    256

#define YAFFS_CHECKPT_MAX_BLOCKS 50

#define YAFFS_CHUNK_SIZE_SHIFT 10

#define YAFFS_MAX_TAGS_SIZE  530

#ifdef CONFIG_YAFFS_UNICODE
#define YCHAR wchar_t
#else
#define YCHAR char
#endif

#endif /* __YAFFS_GUTS_H__ */
