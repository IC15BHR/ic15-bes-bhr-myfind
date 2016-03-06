/**
 * @file main.c
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
 * @todo error handling
 */

/*
 * -------------------------------------------------------------- includes --
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h> //std string functions
#include <error.h>  //errorhandling
#include <errno.h>  //errorhandling

#include <dirent.h>   //directory handling
#include <sys/stat.h> //file stat functions

#include <libgen.h>  //only for basename()
#include <pwd.h>     //user handling
#include <grp.h>     //group handling
#include <time.h>    //for ls time output
#include <fnmatch.h> //patternmatching

/*
 * -------------------------------------------------------------- defines --
 */
#define ARG_MIN 2
#define OPTS_COUNT sizeof(OPTS) / sizeof(OPTS[0])

#ifndef DEBUG // to make -DDEBUG gcc flag possible
#define DEBUG 0
#endif
/*
 * -------------------------------------------------------------- macros --
 */
#define debug_print(fmt, ...)                                                                                          \
    do {                                                                                                               \
        if (DEBUG)                                                                                                     \
            fprintf(stdout, fmt, __VA_ARGS__);                                                                         \
    } while (0)

#define check_flags(mode, flags) (mode & flags) == flags
/*
 * -------------------------------------------------------------- globals --
 */
/*
 * -------------------------------------------------------------- prototypes --
 */
static void do_help(void);

static void do_file(const char *file_name, const char *const *parms);
static void do_dir(const char *dir_name, const char *const *parms);
static void do_params(const char *name, const char *const *parms, struct stat *s);

static int do_print(const char *file_name);

static int do_list(const char *file_name, struct stat *s);
static void snprintf_permissions(char *buf, size_t bufsize, int mode);
static size_t snprintf_username(char *buf, size_t bufsize, uid_t uid);
static size_t snprintf_groupname(char *buf, size_t bufsize, gid_t gid);
static size_t snprintf_filetime(char *buf, size_t bufsize, const time_t *time);

static int do_nouser(struct stat *s);
static int do_user(const char *value, struct stat *s);
static int do_type(const char *value, struct stat *s);
static int do_name(const char *file_name, const char *value);
static int do_path(const char *file_name, const char *value, struct stat *s);

/*
 * -------------------------------------------------------------- constants --
 */
enum OPT { UNKNOWN, PRINT, LS, USER, NAME, TYPE, NOUSER, PATH };

static const char *const OPTS[] = {NULL, "-print", "-ls", "-user", "-name", "-type", "-nouser", "-path"};

/*
 * -------------------------------------------------------------- functions --
 */

/**
 * main Funktion
 * Dieses Programm liest die Nutzereingaben:
 * -ls, -user, -name, -type, -nouser, -path
 * aus.
 * Es arbeitet dabei wie das "find" programm von Linux.
 *
 * \param argc ist die Anzahl der Argumente welche übergeben werden.
 * \param argv ist das Argument selbst.
 *
 * \func do_help() wird aufgerufen, wenn zu wenig Argumente übergeben werden.
 * \func do_file() wird aufgerufen, wenn eine richtige Anzahl an Argumenten übergeben wurde.
 * \func free() gibt den, für das Programm angelegten Speicher, wieder frei.
 *
 * \return always "success"
 * \return 0 always
 */
int main(int argc, char *argv[]) {
    if (argc < ARG_MIN) {
        do_help();
        error(1, 0, "Too few arguments given!"); // TODO: maybe errorcode available?
    }

    // remove tailing slash if present
    size_t len = strlen(argv[1]) - 1;
    if (argv[1][len] == '/') {
        argv[1][len] = '\0';
    }

    // copy args to const parms starting with 2 (as defined in spec)
    const char *parms[argc - 1];
    for (int i = 2; i <= argc; i++)
        parms[i - 2] = argv[i];

    do_file(argv[1], parms);
    return 0;
}

/**
 * do_help Funktion
 * Diese Funktion wird aufgerufen, wenn der User zu wenig Argumente übergibt.
 *
 * \param 'void' Es werden keine Paramenter übergeben.
 *
 */
static void do_help(void) {
    printf("Usage: find <dir> <expressions>\n\nExpressions:\n"
           "  -print              returns formatted list\n"
           "  -ls                 returns formatted list\n"
           "  -user   <name/uid>  file-owners filter\n"
           "  -name   <pattern>   file-name filter\n"
           "  -type   [bcdpfls]   node-type filter\n"
           "  -nouser             filter nonexisting owners\n"
           "  -path   <pattern>   path filter\n");
}

/**
 * do_file Funktion
 * Diese Funktion überprüft ob es sich um ein directory handelt oder nicht.
 * Handelt es sich um ein directory, wird in die Funktion do_dir gesprungen.
 * Handelt es sich um kein directory, wird mit einer Fehlermeldung das Programm beendet.
 *
 * \param file_name ist der relative Pfard
 * \param params übernimmt das dritte Argument [2]
 *
 * \func lstat() ließt die file-Attribute aus und speichert sie in BUF
 * \func do_params() wird aufgerufen um die Parameter auszulesen.
 * \func do_dir() wird zusätzlich aufgerufen wenn es sich um ein directory handelt.
 *
 */
static void do_file(const char *file_name, const char *const *parms) {
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
    }

    do_params(file_name, parms, &status);

    if (S_ISDIR(status.st_mode)) {
        do_dir(file_name, parms);
    }
}

/**
 * do_dir Funktion
 * Diese Funktion überprüft ob es sich bei dem angegebenen Argument um ein directory
 * oder um ein File handelt.
 * Handelt es sich um ein File, wird in die Funktion do_file() gesprungen.
 *
 * \param dir_name
 * \param params
 *
 * \func opendir() öffnet einen diretory stream um die elemente des directorys zu laden.
 * \func readdir() liefert einen pointer zu einem "struct dirent" der den Eintrag beschreibt.
 * \func do_file() springt zurück (rekussive Aufruf) in die do_file Funktion.
 * \func closedir() schließt den diretory stream wieder.
 */
static void do_dir(const char *dir_name, const char *const *parms) {
    struct dirent *dp;
    size_t pathsize;

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

            pathsize = strlen(dir_name) + strlen(dp->d_name) + 2; // lengths + '/' + \0
            char path[pathsize];
            snprintf(path, pathsize, "%s/%s", dir_name, dp->d_name);

            if (errno != 0) {
                error(0, errno, "can't read dir '%s'", path);
                errno = 0;
                continue;
            }

            debug_print("DEBUG: readdir '%s' '%s'\n", dp->d_name, path);
            do_file(path, parms);
        }

        if (closedir(dirp) == -1) {
            error(0, errno, "faild to close dir '%s'", dir_name);
            errno = 0;
        }
    } else {
        error(0, errno, "can't open dir '%s'", dir_name);
        errno = 0;
    }
}

/**
 * do_params Funktion
 * Diese Funktion überprüft die eigegebenen Parameter,
 * geht diese in einer While-Schleife durch und führt bei
 * Treffer das jeweilige If-Statement / Funktion aus.
 *
 * \param file_name
 * \param params
 *
 * \func do_print() gibt den Filenamen auf der Konsole aus.
 * \func do_list() gibt username, groupname und permissions aus.

 * \return kein Return-Wert da "void"
 */
static void do_params(const char *file_name, const char *const *parms, struct stat *s) {
    const char *command = NULL, *value = NULL;
    int i = 0;
    bool printed = false, stop = false, has_value;

    while ((command = parms[i++]) != NULL) {
        enum OPT opt = UNKNOWN;
        for (size_t j = 0; j < OPTS_COUNT && opt == UNKNOWN; j++) {
            const char *copt = OPTS[j];
            if (copt == NULL || strcmp(copt, command) != 0)
                continue;
            debug_print("DEBUG: found arg '%s' as OPT:'%lu'\n", command, (long)j);
            opt = (enum OPT)j;
        }

        switch (opt) {
        case UNKNOWN:
            error(4, 0, "invalid argument '%s'", command);
            return;
        case PRINT:
        case LS:
        case NOUSER:
            has_value = false;
            break;
        default:
            has_value = true;
            break;
        }

        // if value is needed save it and inc iteration-counter
        if (has_value && (value = parms[i++]) == NULL) {
            error(6, 0, "missing value on '%s'", command);
            return;
        }

        switch (opt) {
        case PRINT:
            stop = !do_print(file_name);
            printed = true;
            break;
        case LS:
            stop = !do_list(file_name, s);
            printed = true;
            break;
        case NOUSER:
            stop = !do_nouser(s);
            break;
        case USER:
            stop = !do_user(value, s);
            break;
        case TYPE:
            stop = !do_type(value, s);
            break;
        case NAME:
            stop = !do_name(file_name, value);
            break;
        case PATH:
            stop = !do_path(file_name, value, s);
            break;
        default:
            error(100, 0, "NOT IMPLEMENTED: can't unhandle command '%s'!", command);
            break;
        }

        if (stop) {
            printed = true; // don't print if stopping
            break;
        }
    }

    if (!printed)
        do_print(file_name);
}

/**
 * do_print Funktion
 * Diese Funktion gibt den Namen des Files zurück.
 *
 * \param file_name
 *
 * \func printf()
 *
 * \return Anzahl der charakter
 */
static int do_print(const char *file_name) {
    errno = 0;
    if (printf("%s\n", file_name) < 0) {
        error(0, errno, "can't write to stdout");
        return false;
    }
    return true;
}

/**
 * TODO: Comment
 */
static int do_list(const char *file_name, struct stat *s) {
    // Get Last Modified Time
    char timetext[80];
    snprintf_filetime(timetext, sizeof(timetext), &(s->st_mtim.tv_sec));

    // Get User Name
    char user_name[255]; // TODO: read buffer-size with sysconf()
    if (snprintf_username(user_name, sizeof(user_name), s->st_uid) == 0)
        sprintf(user_name, "%d", s->st_uid);

    // Get Group Name
    char group_name[255]; // TODO: read buffer-size with sysconf()
    snprintf_groupname(group_name, sizeof(group_name), s->st_gid);

    // Get Permissions
    char permissions[11];
    snprintf_permissions(permissions, sizeof(permissions), s->st_mode);

    printf("%lu %4lu", s->st_ino, s->st_blocks / 2); // stat calculates with 512bytes blocksize ... 1024 should be used
    printf(" %s ", permissions);
    printf("%3d %-8s %-8s %8lu %s %s\n", (int)s->st_nlink, user_name, group_name, s->st_size, timetext, file_name);

    return true;
}

/**
 * TODO: Comment
 */
static size_t snprintf_filetime(char *buf, size_t bufsize, const time_t *time) {
    struct tm lt;

    localtime_r(time, &lt);
    debug_print("DEBUG: print_filetime time: '%ld'\n", (long)time);
    return strftime(buf, bufsize, "%b %e %H:%M", &lt);
}

/**
 * TODO: Comment
 */
static size_t snprintf_username(char *buf, size_t bufsize, uid_t uid) {
    errno = 0;
    const struct passwd *usr = getpwuid(uid);

    if (usr == NULL) {
        if (buf != NULL)
            buf[0] = '\0';
        return 0;
    } else {
        if (buf != NULL)
            strncpy(buf, usr->pw_name, bufsize);
        return strlen(usr->pw_name);
    }
}

/**
 * TODO: Comment
 */
static size_t snprintf_groupname(char *buf, size_t bufsize, gid_t gid) {
    errno = 0;
    const struct group *grp = getgrgid(gid);

    if (grp == NULL) {
        error(0, errno, "Failed to get group from id '%d'", gid);
        buf[0] = '\0';
        return 0;
    } else {
        if (buf != NULL)
            strncpy(buf, grp->gr_name, bufsize);
        return strlen(grp->gr_name);
    }
}

/**
 * TODO: Comment
 */
static void snprintf_permissions(char *buf, size_t bufsize, int mode) {
    char lbuf[11] = "----------";

    // print node-type
    if (S_ISBLK(mode)) {
        lbuf[0] = 'b';
    } else if (S_ISCHR(mode)) {
        lbuf[0] = 'c';
    } else if (S_ISDIR(mode)) {
        lbuf[0] = 'd';
    } else if (S_ISFIFO(mode)) {
        lbuf[0] = 'p';
    } else if (S_ISLNK(mode)) {
        lbuf[0] = 'l';
    } else if (S_ISSOCK(mode)) {
        lbuf[0] = 's';
    }

    // print access mask
    if (mode & S_IRUSR) {
        lbuf[1] = 'r';
    } // user   read
    if (mode & S_IWUSR) {
        lbuf[2] = 'w';
    } // user   write

    // user   execute/sticky
    if (check_flags(mode, (S_IXUSR | S_ISUID))) {
        lbuf[3] = 's';
    } else if (mode & S_IXUSR) {
        lbuf[3] = 'x';
    } else if (mode & S_ISUID) {
        lbuf[3] = 'S';
    }

    if (mode & S_IRGRP) {
        lbuf[4] = 'r';
    } // group  read
    if (mode & S_IWGRP) {
        lbuf[5] = 'w';
    } // group  write

    // group  execute/sticky
    if (check_flags(mode, (S_IXGRP | S_ISGID))) {
        lbuf[6] = 's';
    } else if (mode & S_IXGRP) {
        lbuf[6] = 'x';
    } else if (mode & S_ISGID) {
        lbuf[6] = 'S';
    }

    if (mode & S_IROTH) {
        lbuf[7] = 'r';
    } // other  read
    if (mode & S_IWOTH) {
        lbuf[8] = 'w';
    } // other  write

    // other  execute/sticky
    if (check_flags(mode, (S_IXOTH | S_ISVTX))) {
        lbuf[9] = 's';
    } else if (mode & S_IXOTH) {
        lbuf[9] = 'x';
    } else if (mode & S_ISVTX) {
        lbuf[9] = 'S';
    }

    strncpy(buf, lbuf, bufsize);
}

/**
 * TODO: Comment
 */
static int do_nouser(struct stat *s) { return snprintf_username(NULL, 0, s->st_uid) == 0; }

/**
 * TODO: Comment
 */
static int do_user(const char *value, struct stat *s) {
    struct passwd *pass;
    char *tmp;
    unsigned long uid;

    pass = getpwnam(value);
    if (pass == NULL) {
        uid = strtoul(value, &tmp, 0);

        if (*tmp != '\0') {
            error(1, errno, "Can't find user '%s'", value);
            return false;
        }
    } else {
        uid = pass->pw_uid;
    }
    return uid == s->st_uid;
}

/**
 * TODO: Comment
 */
static int do_type(const char *value, struct stat *s) {
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

    return (s->st_mode & mask) == mask;
}

/**
 * TODO: Comment
 */
static int do_name(const char *file_name, const char *value) {
    int result = 0;

    char *name = basename((char *)file_name);
    result = fnmatch(value, name, 0);
    debug_print("DEBUG: do_name for '%s' with '%s' => %d\n", file_name, value, result);

    if (result == 0)
        return true;
    else if (result == FNM_NOMATCH)
        return false;

    error(23, errno, "Pattern '%s' failed on '%s'", value, file_name);
    return false;
}

/**
 * TODO: Comment
 */
static int do_path(const char *file_name, const char *value, struct stat *s) {
    int result = 0;

    if (S_ISDIR(s->st_mode))
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
