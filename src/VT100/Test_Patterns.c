#include "App.h"
#include "Test_Patterns.h"

/*-----------------------------------------------------------------------------------------------------
  Description: Fill buffer with specified test pattern

  Parameters: buffer - buffer to fill
              size - buffer size in bytes
              pattern - pattern type (DATA_PATTERN_*)
              fill_constant - constant value for constant pattern
              start_offset - starting offset for counter/random patterns

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Test_patterns_fill_buffer(uint8_t *buffer, uint32_t size, uint32_t pattern, uint32_t fill_constant, uint32_t start_offset)
{
  switch (pattern)
  {
    case DATA_PATTERN_CONSTANT:
      memset(buffer, fill_constant, size);
      break;

    case DATA_PATTERN_COUNTER:
    {
      uint32_t *word_ptr   = (uint32_t *)buffer;
      uint32_t  counter    = start_offset / 4;
      uint32_t  word_count = size / 4;

      // Fill with 32-bit counter values
      for (uint32_t i = 0; i < word_count; i++)
      {
        word_ptr[i] = counter + i;
      }

      // Fill remaining bytes
      uint32_t remaining_bytes = size % 4;
      if (remaining_bytes > 0)
      {
        uint32_t last_value = counter + word_count;
        uint8_t *byte_ptr   = &buffer[word_count * 4];
        for (uint32_t i = 0; i < remaining_bytes; i++)
        {
          byte_ptr[i] = (uint8_t)((last_value >> (i * 8)) & 0xFF);
        }
      }
    }
    break;

    case DATA_PATTERN_RANDOM:
    {
      uint32_t seed = 0x12345678 + start_offset;
      for (uint32_t i = 0; i < size; i++)
      {
        seed      = seed * 1103515245 + 12345;  // Simple LCG
        buffer[i] = (uint8_t)(seed >> 16);
      }
    }
    break;

    default:
      memset(buffer, 0x00, size);
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Verify buffer data matches expected pattern

  Parameters: buffer - buffer to verify
              size - buffer size in bytes
              pattern - expected pattern type (DATA_PATTERN_*)
              fill_constant - constant value for constant pattern
              start_offset - starting offset for counter/random patterns

  Return: true if data matches expected pattern, false otherwise
-----------------------------------------------------------------------------------------------------*/
bool Test_patterns_verify_buffer(const uint8_t *buffer, uint32_t size, uint32_t pattern, uint32_t fill_constant, uint32_t start_offset)
{
  uint8_t *expected_buffer = App_malloc(size);
  if (expected_buffer == NULL)
  {
    return false;  // Cannot verify without memory
  }

  Test_patterns_fill_buffer(expected_buffer, size, pattern, fill_constant, start_offset);

  bool result = (memcmp(buffer, expected_buffer, size) == 0);

  App_free(expected_buffer);
  return result;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get pattern name string

  Parameters: pattern - pattern type (DATA_PATTERN_*)

  Return: pointer to pattern name string
-----------------------------------------------------------------------------------------------------*/
const char *Test_patterns_get_name(uint32_t pattern)
{
  switch (pattern)
  {
    case DATA_PATTERN_CONSTANT:
      return "Constant";
    case DATA_PATTERN_COUNTER:
      return "Counter";
    case DATA_PATTERN_RANDOM:
      return "Random";
    default:
      return "Unknown";
  }
}
