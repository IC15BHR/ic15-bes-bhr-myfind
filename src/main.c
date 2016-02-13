#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "helpers.h"

#define ARG_MIN 2

void do_file(const char * file_name, const char * const * parms);
void do_dir(const char * dir_name, const char * const * parms);

void do_print(const char *name);
void do_list(const char * file_name);

void do_help(void);

void do_params(const char *name, const char *const *pString);

static const char ARG_PRINT[] = "-print";
static const char ARG_LIST[] = "-ls";
static const char ARG_USER[] = "-user";
static const char ARG_NAME[] = "-name";
static const char ARG_TYPE[] = "-type";

/*
 * Argument Structure:
 * [0] = exec path
 * [1] = target path (first "file_name")
 * [n] = expressions
 */

int main(int argc, char* argv[])
{
    if(argc < ARG_MIN){
        do_help();
        exit_with_error(1, "Too few arguments given!\n");
    }

    argv[argc] = NULL;
    do_file(argv[1], (const char**)&argv[2]);

    return 0;
}

void do_file(const char * file_name, const char * const * parms)
{
    if(file_name == NULL)
        return;

    DIR *dir = opendir(file_name);
    if(dir != NULL){
        do_dir(file_name, parms);
        closedir(dir);
    }

    do_params(file_name, parms);
}

void do_params(const char *file_name, const char *const *parms) {
    const char * p;
    int i = 0;
    int output_done = 0;

    while((p = parms[i++]) != NULL) {
        if(p[0] != '-')
            exit_with_error(1, "%s invalid argument format\n", p);

        if (is_str_equal(p, ARG_PRINT)) {
            do_print(file_name);
            output_done = 1;
            continue;
        }

        if(is_str_equal(p, ARG_LIST)) {
            do_list(file_name);
            output_done = 1;
            continue;
        }

        if(parms[i] == NULL || parms[i][0] == '-')
            exit_with_error(1, "invalid argument format %s\n", p);

        if(is_str_equal(p, ARG_USER)) {
            //TODO: Implement -user argument
            output_done = 0;
            continue;
        }

        if(is_str_equal(p, ARG_NAME)) {
            //TODO: Implement -name argument
            output_done = 0;
            continue;
        }

        if(is_str_equal(p, ARG_TYPE)) {
            //TODO: Implement -type argument
            output_done = 0;
            continue;
        }

        exit_with_error(1, "invalid argument %s\n", p);
    }

    if(!output_done)
        do_print(file_name);
}

void do_dir(const char * dir_name, const char * const * parms)
{
    struct dirent *dp;
    DIR *dfd = opendir(dir_name);
    if(dfd != NULL) {
        while((dp = readdir(dfd)) != NULL){
            if(is_str_equal(dp->d_name, ".") || is_str_equal(dp->d_name, ".."))
                continue;

            char path[FILENAME_MAX];
            snprintf(path, sizeof path, "%s/%s", dir_name, dp->d_name);
            do_params(dir_name, parms);

            if(dp->d_type == DT_DIR)
                do_dir(path, parms);
            else
                do_file(path, parms);
        }
        closedir(dfd);
    } else {
        do_file(dir_name, parms);
    }
}

void do_print(const char * file_name)
{
    printf("%s\n", file_name);
}

void do_list(const char * file_name)
{
    struct stat s;
    struct tm lt;

    stat(file_name, &s);

    localtime_r(&s.st_mtime, &lt);
    char timetext[80];
    strftime(timetext, 80, "%b %d %H:%M", &lt);

    printf("%8lu %8lu %s %2lu %6d %6d %13s %s\n", s.st_ino, s.st_size, "P", s.st_nlink, s.st_uid, s.st_gid, timetext, file_name); //inode blocksize permissions linkcount owner group lastmoddate dirname
}

void do_help(void)
{
    printf(
            "Usage: find <dir> <expressions>\n\nExpressions:"
            "  -print     returns name of directory\n"
            "  -ls        returns formatted list\n"
    );
}

