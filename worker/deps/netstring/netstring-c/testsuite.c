#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "netstring.h"


/* Good examples */
char ex1[] = "12:hello world!,";
char ex2[] = "3:foo,0:,3:bar,";

/* Bad examples */
char ex3[] = "12:hello world! "; /* No comma */
char ex4[] = "15:hello world!,"; /* Too short */
char ex5[] = "03:foo,";		 /* Leading zeros are forbidden */
char ex6[] = "999999999999999:haha lol,"; /* Too long */
char ex7[] = "3fool,";			  /* No colon */
char ex8[] = "what's up";		  /* No number or colon */
char ex9[] = ":what's up";		  /* No number */


void test_netstring_read(void) {
  char *netstring;
  size_t netstring_len;
  int retval;

  /* ex1: hello world */
  retval = netstring_read(ex1, strlen(ex1), &netstring, &netstring_len);
  assert(netstring_len == 12); assert(strncmp(netstring, "hello world!", 12) == 0);
  assert(retval == 0);


  /* ex2: three netstrings, concatenated. */

  retval = netstring_read(ex2, strlen(ex2), &netstring, &netstring_len);
  assert(netstring_len == 3); assert(strncmp(netstring, "foo", 3) == 0);
  assert(retval == 0);

  retval = netstring_read(netstring+netstring_len+1, 9, &netstring, &netstring_len);
  assert(netstring_len == 0); assert(retval == 0);

  retval = netstring_read(netstring+netstring_len+1, 6, &netstring, &netstring_len);
  assert(netstring_len == 3); assert(strncmp(netstring, "bar", 3) == 0);
  assert(retval == 0);


  /* ex3: no comma */
  retval = netstring_read(ex3, strlen(ex3), &netstring, &netstring_len);
  assert(netstring_len == 0); assert(netstring == NULL);
  assert(retval == NETSTRING_ERROR_NO_COMMA);

  /* ex4: too short */
  retval = netstring_read(ex4, strlen(ex4), &netstring, &netstring_len);
  assert(netstring_len == 0); assert(netstring == NULL);
  assert(retval == NETSTRING_ERROR_TOO_SHORT);

  /* ex5: leading zero */
  retval = netstring_read(ex5, strlen(ex5), &netstring, &netstring_len);
  assert(netstring_len == 0); assert(netstring == NULL);
  assert(retval == NETSTRING_ERROR_LEADING_ZERO);

  /* ex6: too long */
  retval = netstring_read(ex6, strlen(ex6), &netstring, &netstring_len);
  assert(netstring_len == 0); assert(netstring == NULL);
  assert(retval == NETSTRING_ERROR_TOO_LONG);

  /* ex7: no colon */
  retval = netstring_read(ex7, strlen(ex7), &netstring, &netstring_len);
  assert(netstring_len == 0); assert(netstring == NULL);
  assert(retval == NETSTRING_ERROR_NO_COLON);

  /* ex8: no number or colon */
  retval = netstring_read(ex8, strlen(ex8), &netstring, &netstring_len);
  assert(netstring_len == 0); assert(netstring == NULL);
  assert(retval == NETSTRING_ERROR_NO_LENGTH);

  /* ex9: no number */
  retval = netstring_read(ex9, strlen(ex9), &netstring, &netstring_len);
  assert(netstring_len == 0); assert(netstring == NULL);
  assert(retval == NETSTRING_ERROR_NO_LENGTH);
}

void test_netstring_buffer_size(void) {
  assert(netstring_buffer_size(0) == 3);
  assert(netstring_buffer_size(1) == 4);
  assert(netstring_buffer_size(2) == 5);
  assert(netstring_buffer_size(9) == 12);
  assert(netstring_buffer_size(10) == 14);
  assert(netstring_buffer_size(12345) == 12345 + 5 + 2);
}

void test_netstring_encode_new(void) {
  char *ns; size_t bytes;

  bytes = netstring_encode_new(&ns, "foo", 3);
  assert(ns != NULL); assert(strncmp(ns, "3:foo,", 6) == 0); assert(bytes == 6);
  free(ns);

  bytes = netstring_encode_new(&ns, NULL, 0);
  assert(ns != NULL); assert(strncmp(ns, "0:,", 3) == 0); assert(bytes == 3);
  free(ns);

  bytes = netstring_encode_new(&ns, "hello world!", 12); assert(bytes == 16);
  assert(ns != NULL); assert(strncmp(ns, "12:hello world!,", 16) == 0);
  free(ns);
}  

int main(void) {
  printf("Running test suite...\n");
  test_netstring_read();
  test_netstring_buffer_size();
  test_netstring_encode_new();
  printf("All tests passed!\n");
  return 0;
}
