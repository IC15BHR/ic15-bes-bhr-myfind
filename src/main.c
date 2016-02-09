//
// Created by Michael Riedmann on 08.02.2016.
//

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

void exit_with_error(int return_code, char *text);
void print_error(char *text);

void do_file(const char * file_name);
void do_dir(const char * dir_name);
void do_print(const char * file_name);

//argv: -printf
int main(int argc, char *argv[])
{
    if(argc < 2)
        exit_with_error(1, "Too few arguments given!");

    char* file_name = argv[1];
    printf("Got target dir: %s\n", file_name);

    do_file(file_name);
}

void do_file(const char * file_name)
{
    printf("Call do_file with %s\n", file_name);

    DIR *directory = opendir(file_name);
    if(directory != NULL) {
        do_dir(file_name);
        closedir(directory);
    }

    do_print(file_name);
}

void do_dir(const char * dir_name)
{
    printf("Call do_dir with %s\n", dir_name);
}

void do_print(const char * file_name)
{
    printf("%s\n", file_name);
}

void exit_with_error(int return_code, char *text) {
    print_error(text);
    exit(return_code);
}

void print_error(char *text) {
    fprintf(stderr, "ERROR: %s\n", text);
}