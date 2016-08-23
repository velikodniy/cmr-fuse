#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "utils.h"

int asprintf(char **strp, const char *fmt, ...) {
  char* buffer;
  va_list ap, ap2;
  va_start(ap, fmt);
  va_copy(ap2, ap);
  size_t n = 1 + vsnprintf(NULL, 0, fmt, ap);
  buffer = malloc(n);
  if (buffer == NULL) return -1;
  n = vsnprintf(buffer, n, fmt, ap2);
  *strp = buffer;
  va_end(ap2);
  va_end(ap);
  return n;
}