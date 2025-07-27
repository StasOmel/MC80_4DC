#include "App.h"

#define MAX_PATH_LENGTH 256
#define MAX_DIRS_IN_STACK 32

// Structure for directory stack (non-recursive tree traversal)
typedef struct
{
  char path[MAX_PATH_LENGTH];
  uint8_t depth;
} T_dir_stack_item;

static T_dir_stack_item g_dir_stack[MAX_DIRS_IN_STACK];
static uint8_t g_stack_top;

// External global LittleFS context
extern T_littlefs_context g_littlefs_context;

const T_VT100_Menu_item MENU_LittleFS_items[] = {
  { '1', Do_LittleFS_init, NULL },
  { '2', Do_LittleFS_list_files, NULL },
  { 'R', NULL, NULL },
  { 0 }  // End of menu
};

const T_VT100_Menu MENU_LittleFS = {
  "LittleFS Manager",
  "\033[5C LittleFS file system management menu\r\n"
  "\033[5C <1> - Initialize LittleFS (auto-format if needed)\r\n"
  "\033[5C <2> - List files\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_LittleFS_items
};

/*-----------------------------------------------------------------------------------------------------
  Description: Helper function to push directory to stack

  Parameters: path - directory path, depth - depth level

  Return: true if pushed successfully, false if stack full
-----------------------------------------------------------------------------------------------------*/
static bool _Push_dir_to_stack(const char *path, uint8_t depth)
{
  if (g_stack_top >= MAX_DIRS_IN_STACK)
  {
    return false; // Stack full
  }

  strncpy(g_dir_stack[g_stack_top].path, path, MAX_PATH_LENGTH - 1);
  g_dir_stack[g_stack_top].path[MAX_PATH_LENGTH - 1] = '\0';
  g_dir_stack[g_stack_top].depth = depth;
  g_stack_top++;
  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Helper function to pop directory from stack

  Parameters: path - buffer for path, depth - pointer to depth variable

  Return: true if popped successfully, false if stack empty
-----------------------------------------------------------------------------------------------------*/
static bool _Pop_dir_from_stack(char *path, uint8_t *depth)
{
  if (g_stack_top == 0)
  {
    return false; // Stack empty
  }

  g_stack_top--;
  strcpy(path, g_dir_stack[g_stack_top].path);
  *depth = g_dir_stack[g_stack_top].depth;
  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Helper function to print tree indentation

  Parameters: depth - depth level

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Print_tree_indent(uint8_t depth)
{
  GET_MCBL;

  for (uint8_t i = 0; i < depth; i++)
  {
    if (i == depth - 1)
    {
      MPRINTF("├── ");
    }
    else
    {
      MPRINTF("│   ");
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Convert LittleFS error code to text description

  Parameters: error - LittleFS error code

  Return: pointer to error description string
-----------------------------------------------------------------------------------------------------*/
static const char *_Littlefs_error_to_string(int error)
{
  switch (error)
  {
    case LFS_ERR_OK:
      return "LFS_ERR_OK: No error";
    case LFS_ERR_IO:
      return "LFS_ERR_IO: Error during device operation";
    case LFS_ERR_CORRUPT:
      return "LFS_ERR_CORRUPT: Corrupted";
    case LFS_ERR_NOENT:
      return "LFS_ERR_NOENT: No directory entry";
    case LFS_ERR_EXIST:
      return "LFS_ERR_EXIST: Entry already exists";
    case LFS_ERR_NOTDIR:
      return "LFS_ERR_NOTDIR: Entry is not a dir";
    case LFS_ERR_ISDIR:
      return "LFS_ERR_ISDIR: Entry is a dir";
    case LFS_ERR_NOTEMPTY:
      return "LFS_ERR_NOTEMPTY: Dir is not empty";
    case LFS_ERR_BADF:
      return "LFS_ERR_BADF: Bad file number";
    case LFS_ERR_FBIG:
      return "LFS_ERR_FBIG: File too large";
    case LFS_ERR_INVAL:
      return "LFS_ERR_INVAL: Invalid parameter";
    case LFS_ERR_NOSPC:
      return "LFS_ERR_NOSPC: No space left on device";
    case LFS_ERR_NOMEM:
      return "LFS_ERR_NOMEM: No more memory available";
    case LFS_ERR_NOATTR:
      return "LFS_ERR_NOATTR: No data/attr available";
    case LFS_ERR_NAMETOOLONG:
      return "LFS_ERR_NAMETOOLONG: File name too long";
    default:
      return "Unknown LittleFS error";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize LittleFS filesystem (performs automatic formatting if mount fails)

  Parameters: keycode - input key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_init(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS Initialization ===\n\r");

  // Initialize LittleFS configuration
  int result = Littlefs_initialize();
  if (result != 0)
  {
    MPRINTF("LittleFS initialization failed: %s\n\r", _Littlefs_error_to_string(result));
    goto exit;
  }

  // Try to mount the filesystem
  result = Littlefs_mount();
  if (result != 0)
  {
    MPRINTF("Mount failed: %s\n\r", _Littlefs_error_to_string(result));
    MPRINTF("Trying to format...\n\r");

    // Format the filesystem if mount fails
    result = Littlefs_format();
    if (result != 0)
    {
      MPRINTF("LittleFS format failed: %s\n\r", _Littlefs_error_to_string(result));
      goto exit;
    }

    // Try to mount again after format
    result = Littlefs_mount();
    if (result != 0)
    {
      MPRINTF("LittleFS mount failed after format: %s\n\r", _Littlefs_error_to_string(result));
      goto exit;
    }

    MPRINTF("Filesystem formatted and mounted successfully\n\r");
  }
  else
  {
    MPRINTF("LittleFS mounted successfully\n\r");
  }

exit:
  if (result == 0)
  {
    MPRINTF("LittleFS initialized successfully\n\r");
  }
  else
  {
    MPRINTF("LittleFS initialization failed: %s\n\r", _Littlefs_error_to_string(result));
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: List all files in the LittleFS filesystem in tree format

  Parameters: keycode - input key code

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Do_LittleFS_list_files(uint8_t keycode)
{
  GET_MCBL;
  lfs_dir_t       dir;
  struct lfs_info info;
  int             result;
  int             total_files = 0;
  int             total_dirs = 0;
  char            current_path[MAX_PATH_LENGTH];
  uint8_t         current_depth;

  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF("=== LittleFS Directory Tree ===\n\r");

  // Check if filesystem is mounted
  if (!Littlefs_is_mounted())
  {
    MPRINTF("Filesystem not mounted. Please initialize first.\n\r");
    goto exit;
  }

  // Initialize stack and start with root directory
  g_stack_top = 0;
  _Push_dir_to_stack("/", 0);

  MPRINTF("Root directory tree:\n\r");
  MPRINTF("/\n\r");

  // Process directories using stack (non-recursive)
  while (_Pop_dir_from_stack(current_path, &current_depth))
  {
    // Open current directory
    result = lfs_dir_open(&g_littlefs_context.lfs, &dir, current_path);
    if (result < 0)
    {
      MPRINTF("Failed to open directory %s: %s\n\r", current_path, _Littlefs_error_to_string(result));
      continue;
    }

    // Read directory entries
    while (true)
    {
      result = lfs_dir_read(&g_littlefs_context.lfs, &dir, &info);
      if (result < 0)
      {
        MPRINTF("Failed to read directory %s: %s\n\r", current_path, _Littlefs_error_to_string(result));
        break;
      }

      // End of directory
      if (result == 0)
      {
        break;
      }

      // Skip "." and ".." entries
      if (info.name[0] == '.')
      {
        continue;
      }

      // Print tree indentation
      _Print_tree_indent(current_depth + 1);

      if (info.type == LFS_TYPE_REG)
      {
        // Format file size with appropriate units (bytes and kilobytes only)
        if (info.size >= 1024)
        {
          MPRINTF("%s (%.2f KB)\n\r", info.name, (float)info.size / 1024.0f);
        }
        else
        {
          MPRINTF("%s (%u B)\n\r", info.name, info.size);
        }
        total_files++;
      }
      else if (info.type == LFS_TYPE_DIR)
      {
        MPRINTF("%s/\n\r", info.name);
        total_dirs++;

        // Add subdirectory to stack if not too deep and stack not full
        if (current_depth < 8 && g_stack_top < MAX_DIRS_IN_STACK - 1)
        {
          char subdir_path[MAX_PATH_LENGTH];
          if (strcmp(current_path, "/") == 0)
          {
            snprintf(subdir_path, MAX_PATH_LENGTH, "/%s", info.name);
          }
          else
          {
            snprintf(subdir_path, MAX_PATH_LENGTH, "%s/%s", current_path, info.name);
          }
          _Push_dir_to_stack(subdir_path, current_depth + 1);
        }
      }
      else
      {
        MPRINTF("%s (UNKNOWN)\n\r", info.name);
      }
    }

    // Close directory
    lfs_dir_close(&g_littlefs_context.lfs, &dir);
  }

  MPRINTF("\nSummary: %d files, %d directories\n\r", total_files, total_dirs);

  // Show filesystem statistics
  struct lfs_fsinfo fsinfo;
  result = lfs_fs_stat(&g_littlefs_context.lfs, &fsinfo);
  if (result == 0)
  {
    uint32_t total_size = fsinfo.block_count * fsinfo.block_size;
    uint32_t used_size = fsinfo.block_count * fsinfo.block_size;
    uint32_t free_size = total_size - used_size;

    MPRINTF("\nFilesystem statistics:\n\r");
    MPRINTF("  Total space: %u KB (%u blocks x %u bytes)\n\r",
            total_size / 1024, fsinfo.block_count, fsinfo.block_size);
    MPRINTF("  Free space:  %u KB\n\r", free_size / 1024);
  }

exit:
  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}
