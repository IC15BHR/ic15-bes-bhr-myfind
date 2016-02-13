//
// Created by ic15dev on 10.02.16.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "helpers.h"

void exit_with_error(int return_code, char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, strcat("find: ", format), argptr);
    va_end(argptr);
    exit(return_code);
}

void print_error(char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, strcat("find: ", format), argptr);
    va_end(argptr);
}

int is_str_equal(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}