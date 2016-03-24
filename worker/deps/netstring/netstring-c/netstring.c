/* Streaming API for netstrings. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "netstring.h"

/* Reads a netstring from a `buffer` of length `buffer_length`. Writes
   to `netstring_start` a pointer to the beginning of the string in
   the buffer, and to `netstring_length` the length of the
   string. Does not allocate any memory. If it reads successfully,
   then it returns 0. If there is an error, then the return value will
   be negative. The error values are:

   NETSTRING_ERROR_TOO_LONG      More than 999999999 bytes in a field
   NETSTRING_ERROR_NO_COLON      No colon was found after the number
   NETSTRING_ERROR_TOO_SHORT     Number of bytes greater than buffer length
   NETSTRING_ERROR_NO_COMMA      No comma was found at the end
   NETSTRING_ERROR_LEADING_ZERO  Leading zeros are not allowed
   NETSTRING_ERROR_NO_LENGTH     Length not given at start of netstring

   If you're sending messages with more than 999999999 bytes -- about
   2 GB -- then you probably should not be doing so in the form of a
   single netstring. This restriction is in place partially to protect
   from malicious or erroneous input, and partly to be compatible with
   D. J. Bernstein's reference implementation.

   Example:
      if (netstring_read("3:foo,", 6, &str, &len) < 0) explode_and_die();
 */
int netstring_read(char *buffer, size_t buffer_length,
		   char **netstring_start, size_t *netstring_length) {
  int i;
  size_t len = 0;

  /* Write default values for outputs */
  *netstring_start = NULL; *netstring_length = 0;

  /* Make sure buffer is big enough. Minimum size is 3. */
  if (buffer_length < 3) return NETSTRING_ERROR_TOO_SHORT;

  /* No leading zeros allowed! */
  if (buffer[0] == '0' && isdigit(buffer[1]))
    return NETSTRING_ERROR_LEADING_ZERO;

  /* The netstring must start with a number */
  if (!isdigit(buffer[0])) return NETSTRING_ERROR_NO_LENGTH;

  /* Read the number of bytes */
  for (i = 0; i < buffer_length && isdigit(buffer[i]); i++) {
    /* Error if more than 9 digits */
    if (i >= 9) return NETSTRING_ERROR_TOO_LONG;
    /* Accumulate each digit, assuming ASCII. */
    len = len*10 + (buffer[i] - '0');
  }

  /* Check buffer length once and for all. Specifically, we make sure
     that the buffer is longer than the number we've read, the length
     of the string itself, and the colon and comma. */
  if (i + len + 1 >= buffer_length) return NETSTRING_ERROR_TOO_SHORT;

  /* Read the colon */
  if (buffer[i++] != ':') return NETSTRING_ERROR_NO_COLON;
  
  /* Test for the trailing comma, and set the return values */
  if (buffer[i + len] != ',') return NETSTRING_ERROR_NO_COMMA;
  *netstring_start = &buffer[i]; *netstring_length = len;

  return 0;
}

/* Return the length, in ASCII characters, of a netstring containing
   `data_length` bytes. */
size_t netstring_buffer_size(size_t data_length) {
  if (data_length == 0) return 3;
  return (size_t)ceil(log10((double)data_length + 1)) + data_length + 2;
}

/* Allocate and create a netstring containing the first `len` bytes of
   `data`. This must be manually freed by the client. If `len` is 0
   then no data will be read from `data`, and it may be NULL. */
size_t netstring_encode_new(char **netstring, char *data, size_t len) {
  char *ns;
  size_t num_len = 1;

  if (len == 0) {
    ns = malloc(3);
    ns[0] = '0';
    ns[1] = ':';
    ns[2] = ',';
  } else {
    num_len = (size_t)ceil(log10((double)len + 1));
    ns = malloc(num_len + len + 2);
    sprintf(ns, "%lu:", (unsigned long)len);
    memcpy(ns + num_len + 1, data, len);
    ns[num_len + len + 1] = ',';
  }

  *netstring = ns;
  return num_len + len + 2;
}
  
