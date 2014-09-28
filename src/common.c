#include "common.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

bool string_to_unsigned_integer(unsigned int *val, const char *str) {
  unsigned long int res;
  char *err;

  if( strlen(str) == 0 )
    return false;
  res = strtoul(str, &err, 10);
  if( err != NULL && *err != '\0' )
    return false;
  if( res > UINT_MAX )
    return false;
  *val = res;
  return true;
}

bool string_to_integer(int *val, const char *str) {
  long int res;
  char *err;

  if( strlen(str) == 0 )
    return false;
  res = strtol(str, &err, 10);
  if( err != NULL && *err != '\0' )
    return false;
  if( res < INT_MIN || res > INT_MAX )
    return false;
  *val = res;
  return true;
}
