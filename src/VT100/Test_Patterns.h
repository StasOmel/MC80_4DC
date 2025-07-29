#ifndef TEST_PATTERNS_H
#define TEST_PATTERNS_H

// Test data patterns
#define DATA_PATTERN_CONSTANT    0     // Fill with constant value
#define DATA_PATTERN_COUNTER     1     // Fill with 32-bit counter
#define DATA_PATTERN_RANDOM      2     // Fill with pseudo-random data
#define DEFAULT_FILL_CONSTANT    0xAA  // Default constant for pattern fill

/*-----------------------------------------------------------------------------------------------------
  Description: Fill buffer with specified test pattern

  Parameters: buffer - buffer to fill
              size - buffer size in bytes
              pattern - pattern type (DATA_PATTERN_*)
              fill_constant - constant value for constant pattern
              start_offset - starting offset for counter/random patterns

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Test_patterns_fill_buffer(uint8_t *buffer, uint32_t size, uint32_t pattern, uint32_t fill_constant, uint32_t start_offset);

/*-----------------------------------------------------------------------------------------------------
  Description: Verify buffer data matches expected pattern

  Parameters: buffer - buffer to verify
              size - buffer size in bytes
              pattern - expected pattern type (DATA_PATTERN_*)
              fill_constant - constant value for constant pattern
              start_offset - starting offset for counter/random patterns

  Return: true if data matches expected pattern, false otherwise
-----------------------------------------------------------------------------------------------------*/
bool Test_patterns_verify_buffer(const uint8_t *buffer, uint32_t size, uint32_t pattern, uint32_t fill_constant, uint32_t start_offset);

/*-----------------------------------------------------------------------------------------------------
  Description: Get pattern name string

  Parameters: pattern - pattern type (DATA_PATTERN_*)

  Return: pointer to pattern name string
-----------------------------------------------------------------------------------------------------*/
const char *Test_patterns_get_name(uint32_t pattern);

#endif // TEST_PATTERNS_H
