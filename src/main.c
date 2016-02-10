#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

void exit_with_error(int return_code, char *text);
void print_error(char *text);
int is_str_equal(const char *a, const char *b);

void do_file(const char * file_name, const char * const * parms);
void do_dir(const char * dir_name, const char * const * parms);

void do_printf(const char *name, const char *const *parms);

const char* ARG_PRINTF = "-printf";
const char* ARG_PRINTF_DEFAULT = "%s\n";

int main(int argc, const char * const * argv)
{
    if(argc < 2)
        exit_with_error(1, "Too few arguments given!");

    do_file(argv[1], &argv[2]);
}

void do_file(const char * file_name, const char * const * parms)
{
    if(parms == NULL || parms[0] == NULL || parms[1] == NULL)
        do_printf(file_name, NULL);
    else if(is_str_equal(parms[0], ARG_PRINTF))
        do_printf(file_name, &parms[1]);

    DIR *dir = opendir(file_name);
    if(dir != NULL){
        do_dir(file_name, parms);
        closedir(dir);
    }
}

void do_printf(const char * file_name, const char * const * parms) {
    const char* format;

    if(parms == NULL || parms[1] == NULL)
        format = ARG_PRINTF_DEFAULT;
    else
        format = parms[1];

    printf(format, file_name);
}

void do_dir(const char * dir_name, const char * const * parms)
{
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
                do_dir(buf, parms);
            else
                do_file(buf, parms);
        }
        closedir(dfd);
    } else {
        do_file(dir_name, parms);
    }
}

void exit_with_error(int return_code, char *text) {
    print_error(text);
    exit(return_code);
}

void print_error(char *text) {
    fprintf(stderr, "ERROR: %s\n", text);
}

int is_str_equal(const char *a, const char *b)
{
    return strcmp(a,b) == 0;
}