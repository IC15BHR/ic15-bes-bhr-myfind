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
 * @version 1.1
 *
 * @todo error handling
 */

/*
 * -------------------------------------------------------------- includes --
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

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
#define OPTS_COUNT sizeof(OPT_NAME) / sizeof(OPT_NAME[0])

#ifndef DEBUG // to make -DDEBUG gcc flag possible
#define DEBUG 0
#endif

#define USERNAME_MAX 255
#define GROUPNAME_MAX 255
#define PERMISSIONS_TEXT_SIZE 11
#define TIME_TEXT_SIZE 80

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
 * -------------------------------------------------------------- types --
 */

/**
 * /enum OPT
 * TODO: Erklärung der Parameter (copy&paste) und Verweis hierher
 *
 */
typedef enum OPT { UNKNOWN, PRINT, LS, USER, NAME, TYPE, NOUSER, PATH } opt_t;

typedef struct PARAM_CONTEXT {
    const char *file_name;
    const struct stat *file_stat;
    const char *value;
} param_context_t;

typedef int retval_t;

/*
 * -------------------------------------------------------------- prototypes --
 */
static void do_help(void);

static retval_t do_file(const char *file_name, const char *const *parms);
static retval_t do_dir(const char *dir_name, const char *const *parms);

static retval_t do_params(const char *name, const char *const *parms, struct stat *s);
static retval_t strtoopt(const char *command, opt_t *opt);
static retval_t check_value(opt_t opt, const char *next_parm);
static retval_t handle_param(opt_t opt, param_context_t *paramc);

static retval_t do_param_print(const param_context_t *paramc);

static retval_t do_param_list(const param_context_t *paramc);
char get_file_type(int mode);
static size_t snprintf_permissions(char *buf, size_t bufsize, int mode);
static size_t snprintf_username(char *buf, size_t bufsize, uid_t uid);
static size_t snprintf_groupname(char *buf, size_t bufsize, gid_t gid);
static size_t snprintf_filetime(char *buf, size_t bufsize, const time_t *time);

static retval_t do_param_nouser(const param_context_t *paramc);
static retval_t do_param_user(const param_context_t *paramc);
static retval_t do_param_type(const param_context_t *paramc);
static retval_t do_param_name(const param_context_t *paramc);
static retval_t do_param_path(const param_context_t *paramc);

static void handle_error(const char *file_name, const char *command, const char *value, int result);

/*
* -------------------------------------------------------------- constants --
*/
static const char *const OPT_NAME[] = {NULL, "-print", "-ls", "-user", "-name", "-type", "-nouser", "-path"};

// Error/Exit Codes
static const retval_t ERR_NOERROR = 0x0000;
static const retval_t ERR_NONCRITICAL = 0x0000;
static const retval_t ERR_TO0_FEW_ARGUMENTS = 0x0011;
static const retval_t ERR_INVALID_ARGUMENT = 0x0012;
static const retval_t ERR_VALUE_UNEXPECTED = 0x101;
static const retval_t ERR_VALUE_MISSING = 0x102;
static const retval_t ERR_NO_USER_FOUND = 0x0111;
static const retval_t ERR_OUTPUT_BROKEN = 0x0141;
static const retval_t ERR_INVALID_TYPE_ARGUMENT = 0x0161;
static const retval_t ERR_INVALID_PATTERN = 0x0171;
static const retval_t ERR_NOT_IMPLEMENTED = 0xFFFF;

// Param Check-Values
static const retval_t VALUE_NOTNEEDED = 2;
static const retval_t VALUE_OK = 3;

// Param Command-Values
static const retval_t OK_PROCEED = 0;
static const retval_t OK_STOP = 1;
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
 *
 * \return 0 on "success"
 */
int main(int argc, char *argv[]) {
    int result = ERR_NOERROR;

    if (argc < ARG_MIN) {
        do_help();
        error(ERR_TO0_FEW_ARGUMENTS, 0, "Too few arguments given!"); // TODO: maybe errorcode available?
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

    result = do_file(argv[1], parms);
    debug_print("DEBUG: Finished execution! Exitcode: '%d'", result);

    return result;
}

/**
 * do_help Funktion
 * Diese Funktion wird aufgerufen, wenn der User zu wenig Argumente übergibt.
 * Dem User wird vorgeschlagen, welche Argumente er nutzen kann.
 */
static void do_help(void) {
    fprintf(stdout, "Usage: find <dir> <expressions>\n\nExpressions:\n"
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
 * Diese Funktion überprüft ob es sich um ein directory handelt oder nicht und ruft
 * in jedem fall do_params() auf.
 * Wird ein Directory erkannt, wird zusätzlich do_dir aufgerufen.
 * Wird ein Fehler beim auslesen der Attribute erkannt wird die Verarbeitung abgebrochen.
 *
 * \param file_name ist der relative Pfad
 * \param params übernimmt die angegebenen Parameter
 *
 * \func lstat() ließt die file-Attribute aus und speichert sie in einen Buffer
 * \func do_params() wird aufgerufen um die Parameter zu verarbeiten.
 * \func do_dir() wird zusätzlich aufgerufen wenn es sich um ein directory handelt.
 *
 */
static retval_t do_file(const char *file_name, const char *const *parms) {
    int result;
    struct stat status;

    debug_print("DEBUG: do_file '%s'\n", file_name);

    errno = 0;
    result = lstat(file_name, &status);
    debug_print("DEBUG: lstat (%d:%d)\n", result, errno);

    if (result == -1) {
        error(ERR_NONCRITICAL, errno, "can't get stat of '%s'", file_name);
        errno = 0;
        result = ERR_NOERROR; // do not panic on unreadable stat
    } else {
        result = do_params(file_name, parms, &status);

        // do not go deeper if an error has happend
        if (result == ERR_NOERROR && S_ISDIR(status.st_mode))
            result = do_dir(file_name, parms);
    }

    debug_print("DEBUG: ended do_file with '%d' \n", result);
    return result;
}

/**
 * do_dir Funktion
 * Diese Funktion verarbeitet ein Directory indem es alle Einträge durchgeht und an do_file() übergibt.
 *
 * \param dir_name
 * \param params
 *
 * \func opendir() öffnet einen diretory stream um die elemente des directorys zu laden.
 * \func readdir() liefert einen pointer zu einem "struct dirent" der den Eintrag beschreibt.
 * \func do_file() springt zurück (rekussive Aufruf) in die do_file Funktion.
 * \func closedir() schließt den diretory stream wieder.
 */
static retval_t do_dir(const char *dir_name, const char *const *parms) {
    struct dirent *dp;
    size_t pathsize;
    int result = 0;

    debug_print("DEBUG: do_dir '%s'\n", dir_name);

    DIR *dirp = opendir(dir_name);
    if (dirp != NULL) {
        // readdir returns (NULL && errno=0) on EOF,
        // (NULL && errno != 0) is not EOF!
        while (((dp = readdir(dirp)) != NULL) || errno != 0) {
            // leave "." and ".." links alone
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                debug_print("DEBUG: skip '%s' '%s' (%d)\n", dir_name, dp->d_name, errno);
                continue;
            }

            // build path
            pathsize = strlen(dir_name) + strlen(dp->d_name) + 2; // lengths + '/' + \0
            char path[pathsize];
            snprintf(path, pathsize, "%s/%s", dir_name, dp->d_name);

            // if readpath throws an error, print it and continue
            if (errno != 0) {
                error(ERR_NONCRITICAL, errno, "can't read dir '%s'", path);
                errno = 0;
                continue;
            }

            debug_print("DEBUG: readdir '%s' '%s'\n", dp->d_name, path);

            result = do_file(path, parms);
            if (result != ERR_NOERROR)
                break;
        }

        if (closedir(dirp) == -1) {
            error(ERR_NONCRITICAL, errno, "faild to close dir '%s'", dir_name);
            errno = 0;
        }
    } else {
        error(ERR_NONCRITICAL, errno, "can't open dir '%s'", dir_name);
        errno = 0;
    }

    debug_print("DEBUG: ended do_dir with '%d' \n", result);
    return result;
}

/**
 * do_params Funktion
 * Diese Funktion überprüft die eigegebenen Parameter,
 * geht diese in einer While-Schleife durch und führt bei
 * Treffer den jeweiligen 'case' aus.
 *
 * \param file_name
 * \param params
 *
 * \func strtoopt() gibt als Ergebnis einen Wert des Enums OPT zurück.
 * \func check_value() gibt als Ergebnis einen Wert des Enums RETVAL_CV zurück.
 * \func handle_param() Ruft die einzelnen unterfunktionen basierend auf der OPT auf.
 * \func handle_error() Errorhandling.
 * \func do_param_print() gibt den Filenamen auf der Konsole aus.
 */
static retval_t do_params(const char *file_name, const char *const *parms, struct stat *s) {
    const char *command = NULL, *value = NULL;
    int i = 0;
    int result = OK_PROCEED;
    bool printed = false;
    param_context_t paramc = {file_name, s, NULL};
    opt_t opt;

    // save current param to command and increment counter
    while ((command = parms[i++]) != NULL) {
        value = parms[i];

        result = strtoopt(command, &opt);
        debug_print("DEBUG: found arg '%s' on '%d' as OPT:'%lu'\n", command, i - 1, (long)opt);

        if (result != ERR_NOERROR) {
            break;
        }

        result = check_value(opt, value);

        // handle positive results
        if (result == VALUE_OK) {
            debug_print("DEBUG: using value '%s'\n", value);
            i++; // increment to skip value and continue with next argument
        } else if (result == VALUE_NOTNEEDED) {
            debug_print("DEBUG: no value needed%s\n", "");
        } else {
            break;
        }

        paramc.value = value;

        result = handle_param(opt, &paramc);

        if (result != OK_PROCEED)
            break;

        switch (opt) {
        case PRINT:
        case LS:
            printed = true;
            break;
        default:
            break;
        }
    }

    // print some error-info
    handle_error(file_name, command, value, result);

    // if no error or stop happend and still not printed, print line
    if (!printed && result == OK_PROCEED)
        result = do_param_print(&paramc);

    // normalize positive return for better upstream handling
    if (result == OK_PROCEED || result == OK_STOP)
        result = ERR_NOERROR;

    debug_print("DEBUG: ended do_params with '%d' \n", result);
    return result;
}

/**
 * strtoopt Funktion
 * Diese Funktion überprüft ob der gegebene Command-String einer gültigen Option entspricht.
 *
 * \param command zu überprüfender Command-String
 *
 * \return opt_t gibt als Ergebnis einen Wert des Enums OPT zurück.
 * \relates OPT
 */
static retval_t strtoopt(const char *command, opt_t *opt) {
    // start on index 1 because 0 is UNKNOWN = NULL
    for (size_t j = 1; j < OPTS_COUNT; j++) {
        const char *current_opt = OPT_NAME[j];
        if (strcmp(current_opt, command) == 0) {
            *opt = (opt_t)j;
            return ERR_NOERROR;
        }
    };

    return ERR_INVALID_ARGUMENT; // default value if no valid param is found
}

/**
 * check_value Funktion
 * Diese Funktion überprüft ob ein "value"-Zusatz zum Argument notwendig ist und überprüft die Gültigkeit.
 *
 * \param opt ausgewertetes Argument als Option-Wert
 * \param next_param unausgewerteter parameter der dem momentanen argument folgt (kann NULL sein)
 *
 * \return retval_cv_t gibt als Ergebnis einen Wert des Enums RETVAL_CV zurück.
 * \relates RETVAL_CV
 */
static retval_t check_value(opt_t opt, const char *next_parm) {
    switch (opt) {
    case PRINT:
    case LS:
    case NOUSER:
        // if no value is expected check if next param is null or a valid arg (start with '-')
        if (next_parm != NULL && next_parm[0] != '-')
            return ERR_VALUE_MISSING;
        return VALUE_NOTNEEDED;
    case USER:
    case TYPE:
    case NAME:
    case PATH:
        // if value is needed check if not null
        if (next_parm == NULL)
            return ERR_VALUE_UNEXPECTED;
        return VALUE_OK;
    default:
        return ERR_NOT_IMPLEMENTED;
    }
}

/**
 * handle_param Funktion
 * Diese Funktion bekommt OPT übergeben und die einzelnen Unterfunktionen,
 * basierend auf OPT auf.
 * Wurde ein falscher Parameter übergeben, wird ein Error übergeben.
 */
static retval_t handle_param(opt_t opt, param_context_t *paramc) {
    debug_print("DEBUG: handle param '%d'\n", opt);

    // call param method
    switch (opt) {
    case PRINT:
        return do_param_print(paramc);
    case LS:
        return do_param_list(paramc);
    case NOUSER:
        return do_param_nouser(paramc);
    case USER:
        return do_param_user(paramc);
    case TYPE:
        return do_param_type(paramc);
    case NAME:
        return do_param_name(paramc);
    case PATH:
        return do_param_path(paramc);
    default:
        return ERR_NOT_IMPLEMENTED;
    }
}

/**
 * do_param_print Funktion
 * Diese Funktion gibt den Namen des Files zurück.
 *
 * \param file_name
 *
 * \func fprintf()
 *
 * \return Anzahl der charakter
 */
static retval_t do_param_print(const param_context_t *paramc) {
    errno = 0;
    if (fprintf(stdout, "%s\n", paramc->file_name) < 0)
        return ERR_OUTPUT_BROKEN;
    
    return OK_PROCEED;
}

/**
 * do_param_list Funktion
 * Diese Funktion ruft die Funktionen 'snprintf_filetime()', 'snprintf_username', 'snprintf_groupname'
 * und 'snprintf_permissions' auf, übergibt diesen die jeweiligen Parameter
 * und gibt anschließend die Werte mittels 'printf()' aus.
 *
 * \param file_name
 * \param s
 *
 * \func snprintf_filetime() bekommt eine Zahl != 0 zurück wenn erfolgreich und '0' im Fehlerfall.
 * \func snprintf_username() bekommt strln zurück wenn erfolgreich und '0' im Fehlerfall.
 * \func snprintf_groupname() bekommt strln zurück wenn erfolgreich und '0' im Fehlerfall.
 * \func snprintf_permissions() übergibt über strncpy() den Array für die Permissions.
 *
 * \return always true
 * \return 1 always
 */
static retval_t do_param_list(const param_context_t *paramc) {
    const struct stat *s = paramc->file_stat;

    // Get Last Modified Time
    char timetext[TIME_TEXT_SIZE];
    snprintf_filetime(timetext, sizeof(timetext), &(s->st_mtim.tv_sec));

    // Get User Name
    char user_name[USERNAME_MAX]; // TODO: read buffer-size with sysconf()
    if (snprintf_username(user_name, sizeof(user_name), paramc->file_stat->st_uid) == 0)
        sprintf(user_name, "%d", s->st_uid);

    // Get Group Name
    char group_name[GROUPNAME_MAX]; // TODO: read buffer-size with sysconf()
    snprintf_groupname(group_name, sizeof(group_name), paramc->file_stat->st_gid);

    // Get Permissions
    char permissions[PERMISSIONS_TEXT_SIZE];
    snprintf_permissions(permissions, sizeof(permissions), paramc->file_stat->st_mode);

    // stat calculates with 512bytes blocksize ... 1024 should be used
    fprintf(stdout, "%6lu %4lu", s->st_ino, s->st_blocks / 2);
    fprintf(stdout, " %s", permissions);
    fprintf(stdout, " %3d %-8s %-8s", (int)s->st_nlink, user_name, group_name);
    fprintf(stdout, " %8lu %s %s\n", s->st_size, timetext, paramc->file_name);
    
    return OK_PROCEED;
}

/**
 * snprintf_filetime Funktion
 * Diese Funktion gibt die Zeit der letzten Änderung des Files zurück.
 *
 * \param buf
 * \param buffsize
 * \param time
 *
 * \func localtime_r() gibt die lokale Zeit im struct tm zurück und nutzt lt zum speichern des Ergebnisses.
 * \func strftime() gibt die Anzahl der geschriebenen Charakter zurück, oder 0 wenn das Maximum überschritten
 *      wird.
 *
 * \return Anzahl der geschriebenen Charakter, wenn erfolgreich.
 * \return 0 im Fehlerfall.
 */
static size_t snprintf_filetime(char *buf, size_t bufsize, const time_t *time) {
    struct tm lt;

    localtime_r(time, &lt);
    debug_print("DEBUG: print_filetime time: '%ld'\n", (long)time);
    return strftime(buf, bufsize, "%b %e %H:%M", &lt);
}

/**
 * snprintf_username Funktion
 * Diese Funktion sucht nach einer passenden User-ID, speichert diese in '*usr' zwischen,
 * überprüft danach ob es sich um einen NULL-Pointer oder um einen Eintrag handelt und gibt, im Falle eines
 * Eintrages, den Namen zurück.
 *
 * \param buf
 * \param bufsize
 * \param uid
 *
 * \func getpwuid() Sucht einen Eintrag mit der passenden User-ID. Gibt 'NULL' im Fehlerfall zurück.
 * \func strncpy() speichert 'usr->pwname' (Zeiger auf Quell-Array) in 'buf' (Zeiger auf Ziel-Array).
 * \func strln() gibt die Länge des String zurück.
 *
 * \return 0 Wenn kein User in passwd gefunden wurde oder  NULL übergeben wurde
 * \return >0 Gibt länge des Usernamen zurück
 */
static size_t snprintf_username(char *buf, size_t bufsize, uid_t uid) {
    struct passwd *usr = getpwuid(uid);
    size_t len;

    if (usr == NULL)
        return 0;

    len = strlen(usr->pw_name);

    if (buf != NULL)
        strncpy(buf, usr->pw_name, bufsize);

    return len;
}

/**
 * snprintf_groupname Funktion
 * Diese Funktion speichert Gruppennamen in len und gibt diesen zurück
 * Wenn der Pointer == 'NULL', dann konnte die Group-ID nicht gefunden werden.
 * Wenn der Pointer != 'NULL', dann wird der Gruppenname zurück gegeben.
 *
 * \param buf
 * \param bufsize
 * \param gid
 *
 * \func getgrgid() Sucht nach einen Eintrag mit der passenden Group-ID. Gibt 'NULL' im Fehlerfall zurück.
 * \func strncpy() speichert 'grp->gr_name' (Zeiger auf Quell-Array) in 'buf' (Zeiger auf Ziel-Array).
 * \func strln() gibt die Länge des String zurück.
 *
 * \return 0 im Fehlerfall
 * \return len wenn erfolgreich
 */
static size_t snprintf_groupname(char *buf, size_t bufsize, gid_t gid) {
    struct group *grp = getgrgid(gid);
    size_t len;

    if (grp == NULL)
        return 0;

    len = strlen(grp->gr_name);

    if (buf != NULL)
        strncpy(buf, grp->gr_name, bufsize);

    return len;
}

/**
 * snprintf_permissions Funktion
 * Diese Funktion gibt die Permissions aus.
 *
 * \param buf
 * \param bufsize
 * \param mode
 *
 * \func get_file_type()
 * \func check_flags() Makro für Bitmask Flag-Checking
 * TODO: Verlinken?
 * \func strncpy() speichert 'lbuf' (Zeiger auf Quell-Array) in 'buf' (Zeiger auf Ziel-Array).
 */
static size_t snprintf_permissions(char *buf, size_t bufsize, int mode) {
    char lbuf[PERMISSIONS_TEXT_SIZE] = "----------";

    // print node-type
    lbuf[0] = get_file_type(mode);

    // regular files are displayed as '-'
    if (lbuf[0] == 'f')
        lbuf[0] = '-';

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
    return 10; // number im characters that should be written to buffer
}
/**
 * get_file_type Funktion
 *
 * 'b' = 'S_ISBLK' => block Device
 * 'c' = 'S_ISCHR' => char Device
 * 'd' = 'S_ISDIR' => Verzeichnis
 * 'p' = 'S_ISFIFO' => FiFo
 * 'l' = 'S_ISLNK' => symbolische Verknüpfung
 * 's' = 'S_ISOCK' => Socket
 */
char get_file_type(int mode) {
    if (S_ISBLK(mode)) {
        return 'b';
    } else if (S_ISCHR(mode)) {
        return 'c';
    } else if (S_ISDIR(mode)) {
        return 'd';
    } else if (S_ISFIFO(mode)) {
        return 'p';
    } else if (S_ISLNK(mode)) {
        return 'l';
    } else if (S_ISSOCK(mode)) {
        return 's';
    } else if (S_ISREG(mode)) {
        return 'f';
    } else {
        return '-';
    }
}

/**
 * do_param_nouser Funktion
 * Diese Funktion ließt den nouser Parameter aus.
 */
static retval_t do_param_nouser(const param_context_t *paramc) {
    size_t name_len = snprintf_username(NULL, 0, paramc->file_stat->st_uid);
    return (name_len == 0) ? OK_PROCEED : OK_STOP;
}

/**
 * do_param_user Funktion
 * finde Directoryeinträge eines Users
 *
 * \param value
 * \param s
 *
 * \func getpwnam() Sucht einen Eintrag mit einem passenden Usernamen
 * \func strtoul() Konvertiert einen String zu einen unsigned long integer
 *
 * \return 'uid' always
 */
static retval_t do_param_user(const param_context_t *paramc) {
    char *tmp;
    ulong uid;

    debug_print("DEBUG: resolving user '%s'\n", paramc->value);

    errno = 0;
    struct passwd *usr = getpwnam(paramc->value);

    // no user in passwd, check if userid is given
    if (usr == NULL) {
        uid = strtoul(paramc->value, &tmp, 0);

        // no username nor userid => error
        if (*tmp != '\0')
            return ERR_NO_USER_FOUND;
    } else {
        // user found in passwd -> prepare uid for check
        uid = usr->pw_uid;
    }

    debug_print("DEBUG: is uid '%lu' == '%d'?\n", uid, paramc->file_stat->st_uid);
    return (uid == paramc->file_stat->st_uid) ? OK_PROCEED : OK_STOP;
}

/**
 * do_param_type Funktion
 * Finde Directoryeinträge mit passendem Typ
 * TODO: Check die Funktion nicht ganz.
 *
 * \param value
 * \param s
 *
 * \func strlen() gibt die Länge des Strings zurück.
 *
 * \return '0' wenn (s->st_mode & mask) == mask
 */
static retval_t do_param_type(const param_context_t *paramc) {
    const char *valid_flags = "bcdpfls";

    if (strlen(paramc->value) != 1)
        return ERR_INVALID_TYPE_ARGUMENT;

    char type_flag = paramc->value[0];

    // check validity
    while (*valid_flags != type_flag && *valid_flags != '\0')
        valid_flags++;

    if ((*valid_flags) == '\0')
        return ERR_INVALID_TYPE_ARGUMENT;

    return (get_file_type(paramc->file_stat->st_mode) == type_flag) ? OK_PROCEED : OK_STOP;
}

/**
 * do_param_name Funktion
 * Finde Directoryeinträge mit passendem Namen
 *
 * \param file_name
 * \param value
 *
 * \func basename() gibt die letzte Komponente des Pfardes zurück.
 * \func fnmatch() überprüft ob 'name' mit 'value' übereinstimmt.
 *      Wenn Übereinstimmung, dann '0'.
 *      Wenn keine Übereinstimmung, dann 'FNM_NOMATCH'
 *
 * \return 'true' wenn Übereintimmung
 * \return 'false' wenn keine Übereinstimmung
 */
static retval_t do_param_name(const param_context_t *paramc) {
    int result = 0;

    char *name = basename((char *)paramc->file_name);
    result = fnmatch(paramc->value, name, 0);
    debug_print("DEBUG: do_param_name for '%s' with '%s' => %d\n", paramc->file_name, paramc->value, result);

    if (result == 0)
        return OK_PROCEED;
    else if (result == FNM_NOMATCH)
        return OK_STOP;

    return ERR_INVALID_PATTERN;
}

/**
 * do_param_path Funktion
 * Finde Directoryeinträge mit passendem Pfad (inkl. Namen)

 *
 * \param file_name
 * \param value
 * \param s
 *
 * \func fnmatch() überprüft ob 'file_name' mit 'value' übereinstimmt.
 *      Wenn Übereinstimmung, dann 0.
 *      Wenn keine Übereinstimmung, dann FNM_NOMATCH
 *
 * \return true wenn Übereinstimmung
 * \return false wenn keine Übereinstimmung
 */
static retval_t do_param_path(const param_context_t *paramc) {
    int result = 0;

    if (S_ISDIR(paramc->file_stat->st_mode))
        return false;

    result = fnmatch(paramc->value, paramc->file_name, 0);

    if (result == 0)
        return OK_PROCEED;
    else if (result == FNM_NOMATCH)
        return OK_STOP;

    return ERR_INVALID_PATTERN;
}
/*
 * handle_error Funktion
 * Diese Funktion wird für das Errorhandling aufgerufen.
 */
static void handle_error(const char *file_name, const char *command, const char *value, int result) {
    if (result == ERR_INVALID_ARGUMENT)
        error(0, 0, "invalid argument '%s'", command);
    else if (result == ERR_VALUE_UNEXPECTED)
        error(0, 0, "unexpected value '%s' on '%s'", value, command);
    else if (result == ERR_VALUE_MISSING)
        error(0, 0, "missing value on '%s'", command);
    else if (result == ERR_INVALID_TYPE_ARGUMENT)
        error(0, 0, "invalid type value '%s'", value);
    else if (result == ERR_NOT_IMPLEMENTED)
        error(0, 0, "NOT IMPLEMENTED: can't unhandle command '%s'!", command);
    else if (result == ERR_OUTPUT_BROKEN)
        error(0, errno, "can't write to stdout!");
    else if (result == ERR_NO_USER_FOUND)
        error(0, errno, "Can't find user '%s'", value);
    else if (result == ERR_INVALID_PATTERN)
        error(0, errno, "Pattern '%s' failed on '%s' with code '%d'", value, file_name, result);
}