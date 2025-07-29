#include "App.h"
#include "FS_Test_Config.h"
#include "Test_Patterns.h"

// Global test configuration instance
T_fs_test_config g_fs_test_config;

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize filesystem test configuration with default values

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Fs_test_config_init(void)
{
  g_fs_test_config.files_count       = FS_TEST_FILES_COUNT_DEFAULT;
  g_fs_test_config.file_size         = FS_TEST_FILE_SIZE_DEFAULT;
  g_fs_test_config.block_size        = FS_TEST_BLOCK_SIZE_DEFAULT;
  g_fs_test_config.data_pattern      = DATA_PATTERN_CONSTANT;
  g_fs_test_config.fill_constant     = DEFAULT_FILL_CONSTANT;
  g_fs_test_config.data_verification = true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Reset filesystem test configuration to default values

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Fs_test_config_reset_to_defaults(void)
{
  Fs_test_config_init();
}
