//
// Created by MR Admin on 08.02.2016.
//

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum filter {
    NO_FILTER = 0,
    FILTER_USER = 1,
    FILTER_NAME = 2,
    FILTER_TYPE = 3
} filter;

void exit_with_error(int return_code, char *text);
void print_error(char *text);
void do_print(void);
filter check_filter_arg(char* arg);

void do_file(const char * file_name, const char * const * parms);
void do_dir(const char * dir_name, const char * const * parms);

//argv: -printf
int main(int argc, char *argv[])
{
    int argi = 2;

    if(argc < 2)
        exit_with_error(1, "Too few arguments given!");

    char* file_name = argv[1];
    printf("Got target dir: %s\n", file_name);

    //check on filters (user, name, type)
    filter f;
    while((f = check_filter_arg(argv[argi++])))
    {
        char* value = argv[argi++];
        if(value[0] == '-')
            exit_with_error(1,"Invalid in an argument-value\n");

        printf("Got Filter %d with Value: %s\n", f, value);
        //TODO: implement Filtering/Globbing
    }

    do_file(file_name, NULL);
}

filter check_filter_arg(char* arg) {
    if(strcmp(arg, "-user") == 0)
        return FILTER_USER;
    if(strcmp(arg, "-name") == 0)
        return FILTER_NAME;
    if(strcmp(arg, "-type") == 0)
        return FILTER_TYPE;

    return NO_FILTER;
}

void do_file(const char * file_name, const char * const * parms)
{
    printf("Call do_file with %s\n", file_name);

    for(int i = 0; parms[i] != NULL; i++)
        printf("  %s\n", parms[i]);
}

void do_dir(const char * dir_name, const char * const * parms)
{
    printf("Call do_dir with %s\n", dir_name);

    for(int i = 0; parms[i] != NULL; i++)
        printf("  %s\n", parms[i]);
}

void do_print(void)
{

}

void exit_with_error(int return_code, char *text) {
    print_error(text);
    exit(return_code);
}

void print_error(char *text) {
    fprintf(stderr, "ERROR: %s\n", text);
}