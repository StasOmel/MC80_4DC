// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2024-07-02
// 15:17:32
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"

const char *days_abbrev[]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months_abbrev[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/*-------------------------------------------------------------------------------------------------------------
  Clear monitor screen
-------------------------------------------------------------------------------------------------------------*/
void VT100_clr_screen(void)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
}

/*-------------------------------------------------------------------------------------------------------------
  Set cursor to specified position
  Column and row counting starts from zero
-------------------------------------------------------------------------------------------------------------*/
void VT100_set_cursor_pos(uint8_t row, uint8_t col)
{
  GET_MCBL;
  MPRINTF("\033[%.2d;%.2dH", row, col);
}

/*-------------------------------------------------------------------------------------------------------------
  Output string to specified position
-------------------------------------------------------------------------------------------------------------*/
void VT100_send_str_to_pos(uint8_t *str, uint8_t row, uint8_t col)
{
  GET_MCBL;
  MPRINTF("\033[%.2d;%.2dH", row, col);
  SEND_BUF(str, strlen((char *)str));
}

/*-------------------------------------------------------------------------------------------------------------
  Find string start position to center it on screen
-------------------------------------------------------------------------------------------------------------*/
uint8_t VT100_find_str_center(uint8_t *str)
{
  int16_t l = 0;
  while (*(str + l) != 0) l++;  // Find string length
  return (COLCOUNT - l) / 2;
}

/*-----------------------------------------------------------------------------------------------------
  Get string from input

  Parameters:
    lp - pointer to string buffer
    n  - maximum string length

  Return:
    Number of characters read
-----------------------------------------------------------------------------------------------------*/
int VT100_get_string(char *lp, int n)
{
  int  cnt = 0;
  char c;
  GET_MCBL;

  do
  {
    if (WAIT_CHAR((unsigned char *)&c, ms_to_ticks(1000)) == RES_OK)
    {
      switch (c)
      {
        case VT100_CNTLQ:  // ignore Control S/Q
        case VT100_CNTLS:
          break;

        case VT100_BCKSP:
        case VT100_DEL:
          if (cnt == 0)
          {
            break;
          }
          cnt--;  // decrement count
          lp--;   // and line VOID*
                  // echo backspace
          MPRINTF("\x008 \x008");
          break;
        case VT100_ESC:
          *lp = 0;  // ESC - stop editing line
          return (RES_ERROR);
        default:
          MPRINTF("*");
          *lp = c;  // echo and store character
          lp++;     // increment line VOID*
          cnt++;    // and count
          break;
      }
    }
  } while (cnt < n - 1 && c != 0x0d);  // check limit and line feed
  *lp = 0;  // mark end of string
  return (RES_OK);
}

/*-----------------------------------------------------------------------------------------------------
  String input

  Parameters:
    buf      - buffer for input characters
    buf_len  - buffer size including null byte. Buffer should also accommodate initial string
    row      - row where input will be performed
    instr    - string with initial value

  Return:
    RES_OK if input was successful
-----------------------------------------------------------------------------------------------------*/
int32_t VT100_edit_string_in_pos(char *buf, int buf_len, int row, char *instr)
{
  int     indx = 0;
  uint8_t b;
  int     res;
  uint8_t bs_seq[] = {VT100_BCKSP, ' ', VT100_BCKSP, 0};
  GET_MCBL;

  indx = 0;
  VT100_set_cursor_pos(row, 0);
  MPRINTF(VT100_CLL_FM_CRSR);
  MPRINTF(">");

  if (instr != 0)
  {
    indx = strlen(instr);
    if (indx >= (buf_len - 1)) indx = buf_len - 1;
    SEND_BUF(instr, indx);
    for (uint32_t n = 0; n < indx; n++)
    {
      buf[n] = instr[n];
    }
  }

  do
  {
    if (WAIT_CHAR(&b, ms_to_ticks(100000)) != RES_OK)
    {
      res = RES_ERROR;
      goto exit_;
    };

    if (b == VT100_BCKSP)
    {
      if (indx > 0)
      {
        indx--;
        SEND_BUF(bs_seq, sizeof(bs_seq));
      }
    }
    else if (b == VT100_ESC)
    {
      res = RES_ERROR;
      goto exit_;
    }
    else if (b != VT100_CR && b != VT100_LF && b != 0)
    {
      SEND_BUF(&b, 1);
      buf[indx] = b; /* String[i] value set to alpha */
      indx++;
      if (indx >= buf_len)
      {
        res = RES_ERROR;
        goto exit_;
      };
    }
  } while ((b != VT100_CR) && (indx < COL));

  res       = RES_OK;
  buf[indx] = 0; /* End of string set to NUL */
exit_:

  VT100_set_cursor_pos(row, 0);
  MPRINTF(VT100_CLL_FM_CRSR);

  return (res);
}

/*-----------------------------------------------------------------------------------------------------
  String editing

  Parameters:
    buf          - buffer for string editing
    buf_len      - buffer size for editing not including terminating 0
    instr        - initial string value

  Return:
    int32_t
-----------------------------------------------------------------------------------------------------*/
int32_t VT100_edit_string(char *buf, uint32_t buf_len, char *instr)
{
  int     indx = 0;
  uint8_t b;
  int     res;
  uint8_t bs_seq[] = {VT100_BCKSP, ' ', VT100_BCKSP, 0};
  GET_MCBL;

  indx = 0;
  MPRINTF(">");

  if (instr != 0)
  {
    indx = strlen(instr);
    if (indx >= (buf_len - 1)) indx = buf_len - 1;
    SEND_BUF(instr, indx);
    for (uint32_t n = 0; n < indx; n++)
    {
      buf[n] = instr[n];
    }
  }

  do
  {
    if (WAIT_CHAR(&b, ms_to_ticks(100000)) != RES_OK)
    {
      res = RES_ERROR;
      goto exit_;
    };

    if (b == VT100_BCKSP)
    {
      if (indx > 0)
      {
        indx--;
        SEND_BUF(bs_seq, sizeof(bs_seq));
      }
    }
    else if (b == VT100_ESC)
    {
      res = RES_ERROR;
      goto exit_;
    }
    else if (b != VT100_CR && b != VT100_LF && b != 0)
    {
      if (indx < (buf_len - 1))
      {
        SEND_BUF(&b, 1);
        buf[indx] = b;
        indx++;
      };
    }
  } while ((b != VT100_CR) && (indx < COL));

  res       = RES_OK;
  buf[indx] = 0;
exit_:

  return (res);
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void VT100_edit_uinteger_val(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv)
{
  char     str[32];
  char     buf[32];
  uint32_t tmpv;
  sprintf(str, "%d", *value);
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (sscanf(buf, "%d", &tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void VT100_edit_integer_val(uint32_t row, int32_t *value, int32_t minv, int32_t maxv)
{
  char    str[32];
  char    buf[32];
  int32_t tmpv;
  sprintf(str, "%d", *value);
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (sscanf(buf, "%d", &tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void VT100_edit_float_val(uint32_t row, float *value, float minv, float maxv)
{
  char  str[32];
  char  buf[32];
  float tmpv;
  sprintf(str, "%f", (double)*value);
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (sscanf(buf, "%f", &tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Edit unsigned integer value in hexadecimal format

  Parameters:
    row   - row for input (input will be at this row, prompt should be printed separately)
    value - pointer to value
    minv  - minimum allowed value
    maxv  - maximum allowed value

  Return:
    int32_t - RES_OK if input successful, RES_ERROR if cancelled (ESC)
-----------------------------------------------------------------------------------------------------*/
int32_t VT100_edit_uinteger_hex_val(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv)
{
  char     str[32];
  char     buf[32];
  uint32_t tmpv;
  sprintf(str, "%08X", *value);  // Print as 8-digit hex
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (sscanf(buf, "%x", &tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
      return RES_OK;
    }
  }
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Edit unsigned integer value with selectable format (hex or decimal)

  Parameters:
    row      - row for input (input will be at this row, prompt should be printed separately)
    value    - pointer to value
    minv     - minimum allowed value
    maxv     - maximum allowed value
    hex_mode - if true, input is hex, else decimal

  Return:
    int32_t - RES_OK if input successful, RES_ERROR if cancelled (ESC)
-----------------------------------------------------------------------------------------------------*/
int32_t VT100_edit_uinteger_val_mode(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv, bool hex_mode)
{
  char     str[32];
  char     buf[32];
  uint32_t tmpv;
  if (hex_mode)
  {
    sprintf(str, "%08X", *value);
  }
  else
  {
    sprintf(str, "%u", *value);
  }
  if (VT100_edit_string_in_pos(buf, 31, row, str) == RES_OK)
  {
    if (hex_mode)
    {
      if (sscanf(buf, "%x", &tmpv) == 1)
      {
        if (tmpv > maxv) tmpv = maxv;
        if (tmpv < minv) tmpv = minv;
        *value = tmpv;
        return RES_OK;
      }
    }
    else
    {
      if (sscanf(buf, "%u", &tmpv) == 1)
      {
        if (tmpv > maxv) tmpv = maxv;
        if (tmpv < minv) tmpv = minv;
        *value = tmpv;
        return RES_OK;
      }
    }
  }
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Interactive integer input with validation (supports decimal and hex)

  This function provides a unified interface for inputting integers with:
  - Support for both decimal (123) and hexadecimal (0x7B) formats
  - Automatic range validation with min/max limits
  - ESC to cancel input
  - Backspace for editing
  - Empty input uses current/default value
  - Timeout protection (30 seconds)

  Parameters:
    result        - pointer to store the entered value
    min_value     - minimum allowed value
    max_value     - maximum allowed value
    current_value - current/default value (used if input is empty)

  Return:
    true if input was successful and value was changed
    false if input was cancelled (ESC) or timeout occurred
-----------------------------------------------------------------------------------------------------*/
bool VT100_input_uint32(uint32_t *result, uint32_t min_value, uint32_t max_value, uint32_t current_value)
{
  GET_MCBL;
  char input_buffer[16];
  memset(input_buffer, 0, sizeof(input_buffer));
  uint32_t value = 0;

  uint8_t pos = 0;
  while (pos < 15)
  {
    uint8_t key;
    if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
    {
      MPRINTF("TIMEOUT\n\r");
      return false;  // Exit on timeout
    }

    if (key == '\r' || key == '\n')
    {
      break;
    }
    else if (key == VT100_ESC)
    {
      MPRINTF("ESC - cancelled\n\r");
      return false;
    }
    else if (key == '\b' || key == 0x7F)  // Backspace
    {
      if (pos > 0)
      {
        pos--;
        input_buffer[pos] = 0;
        MPRINTF("\b \b");
      }
    }
    else if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F') || (key >= 'a' && key <= 'f') || key == 'x' || key == 'X')
    {
      input_buffer[pos] = key;
      pos++;
      MPRINTF("%c", key);
    }
  }

  MPRINTF("\n\r");

  // If no input, use current value
  if (pos == 0)
  {
    *result = current_value;
    return true;
  }

  // Check if input is hexadecimal (starts with 0x or 0X)
  bool    is_hex       = false;
  char   *parse_start  = input_buffer;
  uint8_t parse_length = pos;

  if (pos >= 2 && (input_buffer[0] == '0') && (input_buffer[1] == 'x' || input_buffer[1] == 'X'))
  {
    is_hex       = true;
    parse_start  = &input_buffer[2];
    parse_length = pos - 2;
  }

  // Check if we have any valid digits to process
  if (parse_length == 0)
  {
    MPRINTF("No valid digits entered, using current value\n\r");
    *result = current_value;
    return true;
  }

  // Convert string to number
  if (is_hex)
  {
    // Convert hex string to number with overflow check
    for (uint8_t i = 0; i < parse_length; i++)
    {
      char c = parse_start[i];
      if (c == 0) break;  // End of string

      // Check for potential overflow
      if (value > (UINT32_MAX / 16))
      {
        MPRINTF("WARNING: Value too large, using maximum allowed\n\r");
        value = max_value;
        break;
      }

      value = value * 16;
      if (c >= '0' && c <= '9')
      {
        value += c - '0';
      }
      else if (c >= 'A' && c <= 'F')
      {
        value += c - 'A' + 10;
      }
      else if (c >= 'a' && c <= 'f')
      {
        value += c - 'a' + 10;
      }
    }
  }
  else
  {
    // Convert decimal string to number
    for (uint8_t i = 0; i < parse_length; i++)
    {
      char c = parse_start[i];
      if (c < '0' || c > '9') break;  // Only process decimal digits

      // Check for potential overflow
      if (value > (UINT32_MAX / 10))
      {
        MPRINTF("WARNING: Value too large, using maximum allowed\n\r");
        value = max_value;
        break;
      }

      value = value * 10 + (c - '0');
    }
  }

  // Validate range
  if (value < min_value)
  {
    MPRINTF("Value %u is below minimum %u, using minimum\n\r", value, min_value);
    value = min_value;
  }
  else if (value > max_value)
  {
    MPRINTF("Value %u exceeds maximum %u, using maximum\n\r", value, max_value);
    value = max_value;
  }

  *result = value;
  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get memory address input from user with quick presets

  This function provides convenient address input with predefined quick options
  and supports both decimal and hexadecimal formats.

  Parameters:
    max_address - maximum allowed address value

  Return:
    Address value entered by user
-----------------------------------------------------------------------------------------------------*/
uint32_t VT100_input_address(uint32_t max_address)
{
  GET_MCBL;
  uint32_t address = 0;

  MPRINTF("Quick addresses:\n\r");
  MPRINTF("  <1> - 0x00000000 (Start)\n\r");
  MPRINTF("  <2> - 0x00001000 (4KB offset)\n\r");
  MPRINTF("  <3> - 0x00010000 (64KB offset)\n\r");
  MPRINTF("  <4> - 0x00100000 (1MB offset)\n\r");
  MPRINTF("  <5> - 0x01000000 (16MB offset)\n\r");
  MPRINTF("  <C> - Custom address\n\r");
  MPRINTF("Choice: ");

  uint8_t choice = 0;
  if (WAIT_CHAR(&choice, ms_to_ticks(30000)) != RES_OK)
  {
    MPRINTF("TIMEOUT - using custom input\n\r");
    choice = 'c';  // Default to custom
  }
  MPRINTF("%c\n\r", choice);

  switch (choice)
  {
    case '1':
      return 0x00000000;
    case '2':
      return (0x00001000 <= max_address) ? 0x00001000 : max_address;
    case '3':
      return (0x00010000 <= max_address) ? 0x00010000 : max_address;
    case '4':
      return (0x00100000 <= max_address) ? 0x00100000 : max_address;
    case '5':
      return (0x01000000 <= max_address) ? 0x01000000 : max_address;
    case 'C':
    case 'c':
      break;
    default:
      MPRINTF("Invalid choice, using custom input\n\r");
      break;
  }

  MPRINTF("Enter address (decimal or hex with 0x prefix, max 0x%08X): ", max_address);
  if (VT100_input_uint32(&address, 0, max_address, 0))
  {
    MPRINTF("Selected address: 0x%08X\n\r", address);
  }
  else
  {
    MPRINTF("Input cancelled, using 0x00000000\n\r");
    address = 0;
  }

  return address;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get size input from user with quick presets

  This function provides convenient size input with predefined quick options
  and supports both decimal and hexadecimal formats.

  Parameters:
    max_size - maximum allowed size value

  Return:
    Size value entered by user
-----------------------------------------------------------------------------------------------------*/
uint32_t VT100_input_size(uint32_t max_size)
{
  GET_MCBL;
  uint32_t size = 0;

  MPRINTF("Quick sizes:\n\r");
  MPRINTF("  <1> - 256 bytes (Page size)\n\r");
  MPRINTF("  <2> - 4096 bytes (Sector size)\n\r");
  MPRINTF("  <3> - 65536 bytes (Block size)\n\r");
  MPRINTF("  <4> - 1048576 bytes (1MB)\n\r");
  MPRINTF("  <5> - 16777216 bytes (16MB)\n\r");
  MPRINTF("  <C> - Custom size\n\r");
  MPRINTF("Choice: ");

  uint8_t choice = 0;
  if (WAIT_CHAR(&choice, ms_to_ticks(30000)) != RES_OK)
  {
    MPRINTF("TIMEOUT - using custom input\n\r");
    choice = 'c';  // Default to custom
  }
  MPRINTF("%c\n\r", choice);

  switch (choice)
  {
    case '1':
      return (256 <= max_size) ? 256 : max_size;
    case '2':
      return (4096 <= max_size) ? 4096 : max_size;
    case '3':
      return (65536 <= max_size) ? 65536 : max_size;
    case '4':
      return (1048576 <= max_size) ? 1048576 : max_size;
    case '5':
      return (16777216 <= max_size) ? 16777216 : max_size;
    case 'C':
    case 'c':
      break;
    default:
      MPRINTF("Invalid choice, using custom input\n\r");
      break;
  }

  MPRINTF("Enter size in bytes (decimal or hex with 0x prefix, max %u): ", max_size);
  if (VT100_input_uint32(&size, 1, max_size, 1024))
  {
    MPRINTF("Selected size: %u bytes\n\r", size);
  }
  else
  {
    MPRINTF("Input cancelled, using 1024 bytes\n\r");
    size = 1024;
  }

  return size;
}

/*------------------------------------------------------------------------------
  Memory dump output

  Parameters:
    addr       - displayed starting address of dump
    buf        - pointer to memory
    buf_len    - number of bytes
    sym_in_str - number of bytes displayed per dump line

  Return:
    int32_t
 ------------------------------------------------------------------------------*/
void VT100_print_dump(uint32_t addr, void *buf, uint32_t buf_len, uint8_t sym_in_str)
{
  uint32_t i;
  uint32_t scnt;
  uint8_t *pbuf;
  GET_MCBL;

  pbuf = (uint8_t *)buf;
  scnt = 0;
  for (i = 0; i < buf_len; i++)
  {
    if (scnt == 0)
    {
      MPRINTF("%08X: ", addr);
    }

    MPRINTF("%02X ", pbuf[i]);

    addr++;
    scnt++;
    if (scnt >= sym_in_str)
    {
      scnt = 0;
      MPRINTF("\r\n");
    }
  }

  if (scnt != 0)
  {
    MPRINTF("\r\n");
  }
}

/*-----------------------------------------------------------------------------------------------------
  Input waiting function with VT100 special keys handling

  Processes ESC sequences for arrow keys and returns
  special codes defined in Monitor_utilites.h

  Parameters:
    key     - pointer to variable for storing received character
    timeout - wait time in ticks

  Return:
    RES_OK if character received, otherwise RES_ERROR
-----------------------------------------------------------------------------------------------------*/
int VT100_wait_special_key(uint8_t *key, int timeout)
{
  GET_MCBL;
  uint8_t b;
  int     res;
  res = WAIT_CHAR(&b, timeout);

  if (res != RES_OK)
  {
    return res;  // Timeout expired or error
  }

  // Check if character is the start of ESC sequence
  if (b == VT100_ESC)
  {
    // Wait for second character (should be '[', but skip check for speed)
    if (WAIT_CHAR(&b, 50) != RES_OK)
    {
      *key = VT100_ESC;  // If second character didn't arrive, return ESC
      return RES_OK;
    }

    if (b != '[')
    {
      *key = b;  // If second character is not '[', return it
      return RES_OK;
    }

    // Wait for third character (key code)
    if (WAIT_CHAR(&b, 50) != RES_OK)
    {
      *key = '[';  // If third character didn't arrive, return '['
      return RES_OK;
    }

    // Process arrow key codes
    switch (b)
    {
      case 'A':  // Up arrow
        *key = VT100_UP_ARROW;
        break;
      case 'B':  // Down arrow
        *key = VT100_DOWN_ARROW;
        break;
      case 'C':  // Right arrow
        *key = VT100_RIGHT_ARROW;
        break;
      case 'D':  // Left arrow
        *key = VT100_LEFT_ARROW;
        break;
      default:
        *key = b;  // If sequence not recognized, return last character
        break;
    }
  }
  else
  {
    *key = b;  // Regular character, just return it
  }

  return RES_OK;
}
