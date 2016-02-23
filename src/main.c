#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

////// DEFINES
#define ARG_MIN 2
#define DEBUG 0

////// MACROS
#define debug_print(fmt, ...)                                                                                          \
    do {                                                                                                               \
        if (DEBUG)                                                                                                     \
            fprintf(stdout, fmt, __VA_ARGS__);                                                                         \
    } while (0)

////// PROTOTYPES TODO: Sort prototypes by call-order!
void do_file(const char *file_name, const char *const *parms);
void do_dir(const char *dir_name, const char *const *parms);

void do_print(const char *name);
void do_list(const char *file_name);

void do_help(void);

void do_params(const char *name, const char *const *parms);

void sprintf_permissions(const struct stat *fileStat, char *bufp);

int do_user(const char *file_name, const char *value);
int do_type(const char *file_name, const char *value);
int do_name(const char *file_name, const char *value);
int do_nouser(const char *file_name);
int do_path(const char *file_name, const char *value);

size_t sprintf_username(char *buf, uid_t uid);
size_t sprintf_groupname(char *buf, gid_t gid);
size_t sprintf_filetime(char *buf, const time_t *time);

////// GLOBALS

////// CONSTANTS TODO: Sort!
static const char ARG_PRINT[] = "-print";
static const char ARG_LIST[] = "-ls";
static const char ARG_USER[] = "-user";
static const char ARG_NAME[] = "-name";
static const char ARG_TYPE[] = "-type";
static const char ARG_NOUSER[] = "-nouser";
static const char ARG_PATH[] = "-path";

////// FUNCTIONS TODO: Sort functions by call-order!

int main(int argc, const char *argv[]) {
    if (argc < ARG_MIN) {
        do_help();
        error(1, 0, "Too few arguments given!"); // TODO: maybe errorcode available?
    }

    char *targetpath = realpath(argv[1], NULL); // TODO: Memoryleak on error! Better way?
    debug_print("DEBUG: targetpath = %s\n", targetpath);

    //&argv[2] because "Anleitung" says so:
    //"parms ist dabei die Adresse des ersten Arguments (so wie beim argv), das
    // eine Aktion ist"
    do_file(targetpath, &argv[2]);

    free(targetpath);
    return 0;
}

void do_dir(const char *dir_name, const char *const *parms) {
    struct dirent *dp;

    if (dir_name == NULL) {
        debug_print("DEBUG: NULL dirname in do_dir%s", "\n");
        return;
    }

    debug_print("DEBUG: do_dir '%s'\n", dir_name);

    DIR *dirp = opendir(dir_name);
    if (dirp != NULL) {
        // readdir returns NULL && errno=0 on EOF, errno != 0 is not EOF!
        while (((dp = readdir(dirp)) != NULL) || errno != 0) {
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                debug_print("DEBUG: skip '%s' '%s' (%d)\n", dir_name, dp->d_name, errno);
                continue;
            }

            char path[PATH_MAX]; // TODO: Not efficient - Malloc/Realloc?
            snprintf(path, PATH_MAX, "%s/%s", dir_name, dp->d_name);

            if (errno != 0) {
                error(0, errno, "can't read dir '%s'", path);
                errno = 0;
                continue;
            }

            debug_print("DEBUG: readdir '%s' '%s'\n", dp->d_name, path);
            do_file(path, parms);
        }

        closedir(dirp);
    } else {
        error(0, errno, "can't open dir '%s'", dir_name);
        errno = 0;
    }
}

void do_file(const char *file_name, const char *const *parms) {
    int result = 0;
    struct stat status;

    if (file_name == NULL) {
        debug_print("DEBUG: NULL filename in do_file %s", "\n");
        return;
    }

    debug_print("DEBUG: do_file '%s'\n", file_name);

    errno = 0;
    result = lstat(file_name, &status);
    debug_print("DEBUG: lstat (%d:%d)\n", result, errno);

    if (result == -1) {
        error(0, errno, "can't get stat of '%s'", file_name);
        errno = 0;
    } else if (S_ISDIR(status.st_mode)) {
        do_params(file_name, parms);
        do_dir(file_name, parms);
    } else {
        do_params(file_name, parms);
    }
}

void do_params(const char *file_name, const char *const *parms) {
    const char *command, *value;
    int i = 0;
    int output_done = 0;

    while ((command = parms[i++]) != NULL) {
        //== Arguments Without Values ==
        if (command[0] != '-')
            error(12, 1, "invalid argument format '%s'", command);

        if (strcmp(command, ARG_PRINT) == 0) {
            do_print(file_name);
            output_done = 1;
            continue;
        }

        if (strcmp(command, ARG_LIST) == 0) {
            do_list(file_name);
            output_done = 1;
            continue;
        }

        if (strcmp(command, ARG_NOUSER) == 0) {
            if (!do_nouser(file_name))
                break;
            output_done = 0;
            continue;
        }

        //== Arguments With Values ==
        if ((value = parms[i++]) == NULL)
            error(11, 1, "invalid argument format (value) '%s'", command);

        if (strcmp(command, ARG_USER) == 0) {
            if (!do_user(file_name, value))
                break;
            output_done = 1;
            continue;
        }

        if (strcmp(command, ARG_NAME) == 0) {
            if (!do_name(file_name, value))
                break;
            output_done = 1;
            continue;
        }

        if (strcmp(command, ARG_TYPE) == 0) {
            if (!do_type(file_name, value))
                break;
            output_done = 1;
            continue;
        }

        if (strcmp(command, ARG_PATH) == 0) {
            if (!do_path(file_name, value))
                break;
            output_done = 1;
            continue;
        }

        error(10, 1, "invalid argument '%s'", command);
    }

    if (!output_done) // outputs print if no last print/ls is given in args
        do_print(file_name);
}

int do_nouser(const char *file_name) {
    struct stat s;

    lstat(file_name, &s);
    return sprintf_username(NULL, s.st_uid) == 0;
}

int do_type(const char *file_name, const char *value) {
    struct stat s;
    mode_t mask = 0;

    lstat(file_name, &s);
    for (size_t i = 0; i < strlen(value); i++) {
        switch (value[i]) {
        case 'b':
            mask = mask | S_IFBLK;
            break;
        case 'c':
            mask = mask | S_IFCHR;
            break;
        case 'd':
            mask = mask | S_IFDIR;
            break;
        case 'p':
            mask = mask | S_IFREG;
            break;
        case 'f':
            mask = mask | S_IFIFO;
            break;
        case 'l':
            mask = mask | S_IFLNK;
            break;
        case 's':
            mask = mask | S_IFSOCK;
            break;
        default:
            break;
        }
    }

    return (s.st_mode & mask) == mask;
}

int do_name(const char *file_name, const char *value) {
    // TODO: Implement name-check
    error(0, 0, "NOT IMPLEMENTED: check name '%s' on '%s'", value, file_name);
    return true;
}

int do_user(const char *file_name, const char *value) {
    struct stat s;
    struct passwd *pass;
    char *tmp;
    long uid;

    lstat(file_name, &s);

    uid = strtol(value, &tmp, 0);

    if (*tmp != '\0') {
        pass = getpwnam(value);
        if (pass == NULL) {
            error(1, errno, "Can't find user '%s'", value);
            return false;
        }
        uid = pass->pw_uid;
    }
    return uid == s.st_uid;
}

int do_path(const char *file_name, const char *value) {
    // TODO: Implement path
    printf("check path '%s' on '%s'\n", value, file_name); // only for testing
    return true;
}

void do_print(const char *file_name) { printf("%s\n", file_name); }

void do_list(const char *file_name) {
    struct stat s;

    lstat(file_name, &s);

    // Get Last Modified Time
    char timetext[80];
    sprintf_filetime(timetext, &(s.st_mtim.tv_sec));

    // Get User Name
    char user_name[255]; // TODO: read buffer-size with sysconf()
    sprintf_username(user_name, s.st_uid);

    // Get Group Name
    char group_name[255]; // TODO: read buffer-size with sysconf()
    sprintf_groupname(group_name, s.st_gid);

    // Get Permissions
    char permissions[11];
    sprintf_permissions(&s, permissions);

    printf("%8lu %8lu", s.st_ino, s.st_size);
    printf(" %s ", permissions);
    printf("%2lu %10s %10s %13s %s\n", s.st_nlink, user_name, group_name, timetext, file_name);
}

size_t sprintf_filetime(char *buf, const time_t *time) {
    struct tm lt;

    localtime_r(time, &lt);
    return strftime(buf, 80, "%b %d %H:%M", &lt);
}

size_t sprintf_username(char *buf, uid_t uid) {
    errno = 0;
    const struct passwd *usr = getpwuid(uid);

    if (usr == NULL) {
        error(0, errno, "Failed to get user from id '%d'", uid);
        buf[0] = '\0';
        return 0;
    } else {
        if (buf != NULL)
            strcpy(buf, usr->pw_name);
        return strlen(usr->pw_name);
    }
}

size_t sprintf_groupname(char *buf, gid_t gid) {
    errno = 0;
    const struct group *grp = getgrgid(gid);

    if (grp == NULL) {
        error(0, errno, "Failed to get group from id '%d'", gid);
        buf[0] = '\0';
        return 0;
    } else {
        if (buf != NULL)
            strcpy(buf, grp->gr_name);
        return strlen(grp->gr_name);
    }
}

void sprintf_permissions(const struct stat *fileStat, char *bufp) {
    sprintf(bufp++, (S_ISDIR(fileStat->st_mode)) ? "d" : "-");
    sprintf(bufp++, (fileStat->st_mode & S_IRUSR) ? "r" : "-");
    sprintf(bufp++, (fileStat->st_mode & S_IWUSR) ? "w" : "-");
    sprintf(bufp++, (fileStat->st_mode & S_IXUSR) ? "x" : "-");
    sprintf(bufp++, (fileStat->st_mode & S_IRGRP) ? "r" : "-");
    sprintf(bufp++, (fileStat->st_mode & S_IWGRP) ? "w" : "-");
    sprintf(bufp++, (fileStat->st_mode & S_IXGRP) ? "x" : "-");
    sprintf(bufp++, (fileStat->st_mode & S_IROTH) ? "r" : "-");
    sprintf(bufp++, (fileStat->st_mode & S_IWOTH) ? "w" : "-");
    sprintf(bufp, (fileStat->st_mode & S_IXOTH) ? "x" : "-");
}

void do_help(void) {
    printf("Usage: find <dir> <expressions>\n\nExpressions:"
           "  -print              returns formatted list\n"
           "  -ls                 returns formatted list\n"
           "  -user   <name/uid>  file-owners filter\n"
           "  -name   <pattern>   file-name filter\n"
           "  -type   [bcdpfls]   node-type filter\n"
           "  -nouser             filter nonexisting owners\n"
           "  -path   <pattern>   path filter\n");
}
