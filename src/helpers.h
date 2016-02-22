//
// Created by ic15dev on 10.02.16.
//

#ifndef FIND_HELPERS_H
#define FIND_HELPERS_H

void exit_with_error(int return_code, char *format, ...);
void print_last_error(const char * cmd_name);
void print_error(char *format, ...);
int is_str_equal(const char *s1, const char *s2);

#endif // FIND_HELPERS_H
