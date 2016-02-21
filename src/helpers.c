//
// Created by ic15dev on 10.02.16.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include "helpers.h"

void print_error_valist(char *format, va_list argptr);

void exit_with_error(int return_code, char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  print_error_valist(format, argptr);
  va_end(argptr);
  exit(return_code);
}

void print_last_error(const char *const *parms) {
  fprintf(stderr, "%s: %s\n", parms[0], strerror(errno));
}

void print_error(char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  print_error_valist(format, argptr);
  va_end(argptr);
}

void print_error_valist(char *format, va_list argptr) {
  vfprintf(stderr, format, argptr);
  fprintf(stderr, "\n"); // postfix with newline
}

int is_str_equal(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }