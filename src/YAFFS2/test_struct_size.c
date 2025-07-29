#include "App.h"
#include "yaffs_nor_config.h"
#include "yaffs_guts.h"

/*-----------------------------------------------------------------------------------------------------
  Test function to determine exact size of yaffs_ext_tags structure

  This function will output the size during compilation or runtime
-----------------------------------------------------------------------------------------------------*/
void Test_yaffs_ext_tags_size(void)
{
  // Force compiler to show size in error message
  char size_check[sizeof(struct yaffs_ext_tags)];

  // This will never execute but helps us see the size
  (void)size_check;
}

// Alternative: use pragma to show message during compilation
#pragma message("Size of yaffs_ext_tags: " STRINGIFY(sizeof(struct yaffs_ext_tags)))

// Helper macro for stringification
#define STRINGIFY(x) #x
