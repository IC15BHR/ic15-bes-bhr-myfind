#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "helpers.h"

#define ARG_MIN 2

void do_file(const char * file_name, const char * const * parms);
void do_dir(const char * dir_name, const char * const * parms);

void do_print(const char *name);
void do_help(void);

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
        exit_with_error(1, "Too few arguments given!");
    }

    argv[argc] = NULL;
    if(!is_str_equal(argv[argc-1], ARG_PRINT)) //add default print to end if none is given
        strcpy(argv[argc-1], ARG_PRINT);

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

    const char * p;
    int i = 0;
    while((p = parms[i++]) != NULL) {
        if (is_str_equal(parms[0], ARG_PRINT))
            do_print(file_name);
    }
}

void do_dir(const char * dir_name, const char * const * parms)
{
    struct dirent *dp;
    DIR *dfd = opendir(dir_name);
    if(dfd != NULL) {
        while((dp = readdir(dfd)) != NULL){
            if(is_str_equal(dp->d_name, ".") || is_str_equal(dp->d_name, ".."))
                continue;

            char buf[FILENAME_MAX];
            snprintf(buf, sizeof buf, "%s/%s", dir_name, dp->d_name);
            printf("%s\n", buf);

            if(dp->d_type == DT_DIR)
                do_dir(buf, parms);
            else
                do_file(buf, parms);
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

void do_help(void)
{
    printf(
            "Usage: find <dir> <expressions>\n\nExpressions:"
            "  -print     returns name of directory\n"
    );
}

