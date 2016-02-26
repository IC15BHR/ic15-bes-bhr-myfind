/**
 * @file my_find.c
 * Betriebssysteme my_find file
 * Beispiel 1
 *
 * @author Baliko Markus	    <ic15b001@technikum-wien.at>
 * @author Haubner Alexander    <ic15b033@technikum-wien.at>
 * @author Riedmann Michael     <ic15b054@technikum-wien.at>
 *
 * @date 2015/02/25
 *
 * @version 1
 *
 * @todo comments
 * @todo error handling
 */

/*
 * -------------------------------------------------------------- includes --
 */
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
#include <fnmatch.h>

/*
 * -------------------------------------------------------------- defines --
 */
#define ARG_MIN 2
#define DEBUG 0

/*
 * -------------------------------------------------------------- macros --
 */
#define debug_print(fmt, ...)                                                                                          \
    do {                                                                                                               \
        if (DEBUG)                                                                                                     \
            fprintf(stdout, fmt, __VA_ARGS__);                                                                         \
    } while (0)
/*
 * -------------------------------------------------------------- globals --
 */
/*
 * -------------------------------------------------------------- prototypes --
 */
// TODO: Sort prototypes by call-order!
void do_file(const char *file_name, const char *const *parms);
void do_dir(const char *dir_name, const char *const *parms);

int do_print(const char *file_name);
int do_list(const char *file_name, struct stat s);

void do_help(void);

void do_params(const char *name, const char *const *parms);

void sprintf_permissions(char *buf, int mode);

int do_user(const char *value, struct stat s);
int do_type(const char *value, struct stat s);
int do_name(const char *file_name, const char *value, struct stat s);
int do_nouser(struct stat s);
int do_path(const char *file_name, const char *value, struct stat s);

size_t sprintf_username(char *buf, uid_t uid);
size_t sprintf_groupname(char *buf, gid_t gid);
size_t sprintf_filetime(char *buf, const time_t *time);

/*
 * -------------------------------------------------------------- constants --
 */
enum OPT { UNKNOWN, PRINT, LS, USER, NAME, TYPE, NOUSER, PATH };

const char *const OPTS[] = {"", "-print", "-ls", "-user", "-name", "-type", "-nouser", "-path"};

/*
 * -------------------------------------------------------------- functions --
 */
// TODO: Sort functions by call-order!

/**
 * Main Funktion
 * Dieses Programm ließt die Nutzereingaben:
 * -ls, -user, -name, -type, -nouser, -path
 * aus.
 * Es arbeitet dabei wie das "find" programm von Linux.
 *
 * \param argc ist die Anzahl der Argumente
 * \param argv ist das Argument selbst
 *
 * \func do_help() wird aufgerufen wenn zuwenig Argumente übergeben werden
 * \func do_file() wird aufgerufen wenn eine richtige Anzahl an Argumenten übergeben wurde
 *
 * \return always "success"
 * \return 0 always
 * TODO: beschreiben was die Funktion macht?
 */
int main(int argc, const char *argv[]) {
    if (argc < ARG_MIN) {
        do_help();
        error(1, 0, "Too few arguments given!"); // TODO: maybe errorcode available?
    }

    //&argv[2] because "Anleitung" says so ...
    //"parms ist dabei die Adresse des ersten Arguments (so wie beim argv), das
    // eine Aktion ist"
    do_file(argv[1], &argv[2]);
    return 0;
}
/**
 * do_file Funktion
 * Diese Funktion überprüft ob es sich um ein File oder um ein Directory handelt
 *
 * \param file_name ist der relative Pfard
 * \param params übernimmt das dritte Argument [2]
 *
 * \func do_params() wird aufgerufen um die Paramenter auszulesen.
 * \func do_dir() wird zusätzlich aufgerufen wenn es sich um ein directory handelt
 *
 * \return kein Return-Wert da "void"
 */
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

/**
 * do_dir Funktion
 * Diese Funktion überprüft ob es sich bei dem angegebenen Argument um ein directory
 * oder um ein file handelt.
 * Handelt es sich um ein File, wird in die Funktion do_file() gesprungen.
 *
 * \param dir_name
 * \param params
 *
 * \func opendir() öffnet einen diretory stream um die elemente des directorys zu laden
 * \func readdir() liefert einen pointer zu einem "struct dirent" der den Eintrag beschreibt
 * \func do_file()
 * \func closedir() schließt den diretory stream wieder
 *
 * \return kein Return-Wert da "void"
 */
void do_dir(const char *dir_name, const char *const *parms) {
    struct dirent *dp;

    if (dir_name == NULL) {
        debug_print("DEBUG: NULL dirname in do_dir%s", "\n");
        return;
    }

    debug_print("DEBUG: do_dir '%s'\n", dir_name);

    DIR *dirp = opendir(dir_name);
    if (dirp != NULL) {
        // readdir returns (NULL && errno=0) on EOF,
        // (NULL && errno != 0) is not EOF!
        while (((dp = readdir(dirp)) != NULL) || errno != 0) {
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                debug_print("DEBUG: skip '%s' '%s' (%d)\n", dir_name, dp->d_name, errno);
                continue;
            }

            char path[PATH_MAX]; // TODO: Not standardized! - Malloc/Realloc?
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
/**
 * do_params Funktion
 * Diese Funktion überprüft die eigegebenen Parameter,
 * geht diese in einer while-Schleife durch und führt bei
 * treffer das jeweilige if-statement / Funktion aus.
 *
 * \param file_name
 * \param params
 *
 * \func do_print() print
 * \func do_list()
 * \func do_nouser()
 * \func do_user()
 * \func do_type()
 * \func do_name()
 * \func do_path()
 *
 * \return kein Return-Wert da "void"
 */
void do_params(const char *file_name, const char *const *parms) {
    const char *command, *value = "";
    int i = 0;
    bool printed = false;
    struct stat s;
    int rets = 0;

    while ((command = parms[i++]) != NULL) {
        enum OPT opt = UNKNOWN;
        for (size_t j = 0; j < sizeof(OPTS) && opt == UNKNOWN; j++) {
            if (strcmp(OPTS[j], command) != 0)
                continue;
            opt = (enum OPT)j;
        }


        if (opt == UNKNOWN) {
            error(4, 0, "invalid argument '%s'", command);
            return;
        }

        //print, ls, nouser can't have a value
        if(opt == PRINT || opt == LS || opt == NOUSER) {
            if(parms[i] != NULL && parms[i][0] != '-') {
                error(6, 0, "missing value on '%s'", command);
                printed = true;
                break;
            }
        }

        if (opt == PRINT) {
            do_print(file_name);
            printed = true;
            continue;
        }

        //only run on first hit and not for print
        if (rets == 0 && (rets = lstat(file_name, &s)) == -1) {
            error(0, errno, "failed to read stat of '%s'", file_name);
            printed = true;
            break;
        }

        if (opt == LS) {
            do_list(file_name, s);
            printed = true;
            continue;
        }

        if (opt == NOUSER) {
            if (do_nouser(s))
                continue;
            printed = true;
            break;
        }

        //check for value and invalid (if present) next argument
        if ((value = parms[i++]) == NULL) {
            error(6, 0, "missing value on '%s'", command);
            return;
        } else if(parms[i] != NULL && parms[i][0] != '-') {
            error(7, 0, "invalid arg '%s'", parms[i]);
            return;
        }

        if (opt == USER) {
            if (do_user(value, s))
                continue;
            printed = true;
            break;
        }

        if (opt == TYPE) {
            if (do_type(value, s))
                continue;
            printed = true;
            break;
        }

        if (opt == NAME) {
            if (do_name(file_name, value, s))
                continue;
            printed = true;
            break;
        }

        if (opt == PATH) {
            if (do_path(file_name, value, s))
                continue;
            printed = true;
            break;
        }

        error(100, 0, "NOT IMPLEMENTED: can't unhandle command '%s'!", command);
    }

    if (!printed)
        do_print(file_name);
}

int do_nouser(struct stat s) {
    return sprintf_username(NULL, s.st_uid) == 0;
}

int do_type(const char *value, struct stat s) {
    mode_t mask = 0;
    if (strlen(value) != 1) {
        error(32, 0, "invalid type value '%s'", value);
        return false;
    }

    switch (value[0]) {
    case 'b':
        mask = S_IFBLK;
        break;
    case 'c':
        mask = S_IFCHR;
        break;
    case 'd':
        mask = S_IFDIR;
        break;
    case 'p':
        mask = S_IFIFO;
        break;
    case 'f':
        mask = S_IFREG;
        break;
    case 'l':
        mask = S_IFLNK;
        break;
    case 's':
        mask = S_IFSOCK;
        break;
    default:
        error(18, 0, "invalid flag on type '%c'", value[0]);
        break;
    }

    return (s.st_mode & mask) == mask;
}

int do_name(const char *file_name, const char *value, struct stat s) {
    int result = 0;

    if (S_ISDIR(s.st_mode))
        return false;

    char *name = basename((char *)file_name);
    result = fnmatch(value, name, 0);

    if (result == 0)
        return true;
    else if (result == FNM_NOMATCH)
        return false;

    error(23, 0, "Pattern '%s' failed on '%s'", value, file_name);
    return false;
}

int do_user(const char *value, struct stat s) {
    struct passwd *pass;
    char *tmp;
    unsigned long uid;

    pass = getpwnam(value);
    if (pass == NULL) {
        uid = strtol(value, &tmp, 0);

        if (*tmp != '\0') {
            error(1, errno, "Can't find user '%s'", value);
            return false;
        }
    } else {
        uid = pass->pw_uid;
    }
    return uid == s.st_uid;
}

int do_path(const char *file_name, const char *value, struct stat s) {
    int result = 0;

    if (S_ISDIR(s.st_mode))
        return false;

    result = fnmatch(value, file_name, 0);

    if (result == 0)
        return true;
    else if (result == FNM_NOMATCH)
        return false;

    error(23, 0, "Pattern '%s' failed on '%s'", value, file_name);
    return false;
    return true;
}

int do_print(const char *file_name) { return printf("%s\n", file_name); }

int do_list(const char *file_name, struct stat s) {
    // Get Last Modified Time
    char timetext[80];
    sprintf_filetime(timetext, &(s.st_mtim.tv_sec));

    // Get User Name
    char user_name[255]; // TODO: read buffer-size with sysconf()
    if (sprintf_username(user_name, s.st_uid) == 0)
        sprintf(user_name, "%d", s.st_uid);

    // Get Group Name
    char group_name[255]; // TODO: read buffer-size with sysconf()
    sprintf_groupname(group_name, s.st_gid);

    // Get Permissions
    char permissions[11];
    sprintf_permissions(permissions, s.st_mode);

    printf("%lu %4lu", s.st_ino, s.st_blocks / 2); // stat calculates with 512bytes blocksize ... 1024 should be used
    printf(" %s ", permissions);
    printf("%3d %-8s %-8s %8lu %s %s\n", s.st_nlink, user_name, group_name, s.st_size, timetext, file_name);

    return true;
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
        if (buf != NULL)
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

void sprintf_permissions(char *buf, int mode) {
    // TODO: Comment which is which
    // TODO: Check buffer size!
    // print node-type
    if (S_ISBLK(mode)) {
        sprintf(buf++, "%c", 'b');
    } else if (S_ISCHR(mode)) {
        sprintf(buf++, "%c", 'c');
    } else if (S_ISDIR(mode)) {
        sprintf(buf++, "%c", 'd');
    } else if (S_ISFIFO(mode)) {
        sprintf(buf++, "%c", 'p');
    } else if (S_ISLNK(mode)) {
        sprintf(buf++, "%c", 'l');
    } else if (S_ISSOCK(mode)) {
        sprintf(buf++, "%c", 's');
    } else {
        sprintf(buf++, "%c", '-');
    }

    // print access mask
    sprintf(buf++, "%c", (mode & S_IRUSR) ? 'r' : '-'); // user   read
    sprintf(buf++, "%c", (mode & S_IWUSR) ? 'w' : '-'); // user   write

    if (mode & S_IXUSR) { // user   execute/sticky
        sprintf(buf++, "%c", (mode & S_ISUID) ? 's' : 'x');
    } else {
        sprintf(buf++, "%c", (mode & S_ISUID) ? 'S' : '-');
    }

    sprintf(buf++, "%c", (mode & S_IRGRP) ? 'r' : '-'); // group  read
    sprintf(buf++, "%c", (mode & S_IWGRP) ? 'w' : '-'); // group  write

    if (mode & S_IXGRP) { // group  execute/sticky
        sprintf(buf++, "%c", (mode & S_ISGID) ? 's' : 'x');
    } else {
        sprintf(buf++, "%c", (mode & S_ISGID) ? 'S' : '-');
    }

    sprintf(buf++, "%c", (mode & S_IROTH) ? 'r' : '-'); // other  read
    sprintf(buf++, "%c", (mode & S_IWOTH) ? 'w' : '-'); // other  write
    if (mode & S_IXOTH) {                               // other  execute/sticky
        sprintf(buf++, "%c", (mode & S_ISVTX) ? 's' : 'x');
    } else {
        sprintf(buf++, "%c", (mode & S_ISVTX) ? 'S' : '-');
    }
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
