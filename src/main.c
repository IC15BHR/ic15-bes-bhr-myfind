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
int is_str_equal(char *a, char *b);

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

    do_dir(file_name);
}

void do_file(const char * file_name)
{
    printf("Call do_file with %s\n", file_name);

    do_print(file_name);
}

void do_dir(const char * dir_name)
{
    printf("Call do_dir with %s\n", dir_name);

    struct dirent *dp;
    DIR *dfd = opendir(dir_name);
    if(dfd != NULL) {
        while((dp = readdir(dfd)) != NULL){
            if(is_str_equal(dp->d_name, "."))
                continue;
            if(is_str_equal(dp->d_name, ".."))
                continue;

            char buf[FILENAME_MAX];
            snprintf(buf, sizeof buf, "%s/%s", dir_name, dp->d_name);
            printf("%s\n", buf);

            if(dp->d_type == DT_DIR)
                do_dir(buf);
            else
                do_file(buf);
        }
        closedir(dfd);
    } else {
        do_file(dir_name);
    }
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

int is_str_equal(char *a, char *b)
{
    return strcmp(a,b) == 0;
}