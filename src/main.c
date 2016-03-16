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
 */
typedef enum OPT { UNKNOWN, PRINT, LS, USER, NAME, TYPE, NOUSER, PATH } opt_t;

/**
 * /enum RETVAL_CV
 * TODO: kurze Erklärung zu den einträgen
 */
typedef enum RETVAL_CV {
    UNKNOWN_OPT = 0,
    NO_VALUE = 1,
    VALUE_OK = 2,
    UNEXPECTED_PARAM = -1,
    MISSING_PARAM = -2
} retval_cv_t;

typedef struct PARAM_CONTEXT {
    const char *file_name;
    const char *value;
    const struct stat *file_stat;
    const struct passwd *user;
    const struct group *group;
    bool user_loaded;
    bool group_loaded;
    bool permissions_loaded;
    char permissions[11];
} param_context_t;

/*
 * -------------------------------------------------------------- prototypes --
 */
static void do_help(void);

static void do_file(const char *file_name, const char *const *parms);
static void do_dir(const char *dir_name, const char *const *parms);

static void do_params(const char *name, const char *const *parms, struct stat *s);
static opt_t get_opt(const char *command);
static retval_cv_t check_value(opt_t opt, const char *next_parm);
static bool handle_param(opt_t opt, param_context_t *paramc);

static bool do_param_print(const param_context_t *paramc);

static bool do_param_list(const param_context_t *paramc);
static size_t snprintf_permissions(char *buf, size_t bufsize, int mode);
static size_t snprintf_username(char *buf, size_t bufsize, const struct passwd *usr);
static size_t snprintf_groupname(char *buf, size_t bufsize, const struct group *grp);
static size_t snprintf_filetime(char *buf, size_t bufsize, const time_t *time);

static bool do_param_nouser(const param_context_t *paramc);
static bool do_param_user(const param_context_t *paramc);
static bool do_param_type(const param_context_t *paramc);
static bool do_param_name(const param_context_t *paramc);
static bool do_param_path(const param_context_t *paramc);

/*
* -------------------------------------------------------------- constants --
*/
static const char *const OPT_NAME[] = {NULL, "-print", "-ls", "-user", "-name", "-type", "-nouser", "-path"};

//MyFind Error/Exit Codes
static const int MF_ERR_NONCRITICAL = 0x0000;
static const int MF_ERR_TO0_FEW_ARGUMENTS = 0x0001;
static const int MF_ERR_INVALID_ARGUMENT = 0x0011;
static const int MF_ERR_UNEXPECTED_ARGUMENT = 0x0012;
static const int MF_ERR_MISSING_ARGUMENT = 0x0013;
static const int MF_ERR_NO_USER_FOUND = 0x0111;
static const int MF_ERR_INVALID_TYPE_ARGUMENT = 0x0161;
static const int MF_ERR_INVALID_PATTERN = 0x0171;
static const int MF_ERR_NOT_IMPLEMENTED = 0xFFFF;
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
    if (argc < ARG_MIN) {
        do_help();
        error(MF_ERR_TO0_FEW_ARGUMENTS, 0, "Too few arguments given!"); // TODO: maybe errorcode available?
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
 * Dem User wird vorgeschlagen, welche Argumente er nutzen kann.
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
        error(MF_ERR_NONCRITICAL, errno, "can't get stat of '%s'", file_name);
        errno = 0;
        return;
    }

    do_params(file_name, parms, &status);

    if (S_ISDIR(status.st_mode)) {
        do_dir(file_name, parms);
    }
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
                error(MF_ERR_NONCRITICAL, errno, "can't read dir '%s'", path);
                errno = 0;
                continue;
            }

            debug_print("DEBUG: readdir '%s' '%s'\n", dp->d_name, path);
            do_file(path, parms);
        }

        if (closedir(dirp) == -1) {
            error(MF_ERR_NONCRITICAL, errno, "faild to close dir '%s'", dir_name);
            errno = 0;
        }
    } else {
        error(MF_ERR_NONCRITICAL, errno, "can't open dir '%s'", dir_name);
        errno = 0;
    }
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
 * \func do_param_print() gibt den Filenamen auf der Konsole aus.
 * \func do_param_list() gibt username, groupname und permissions aus.
 * TODO: Funktionen
 */
static void do_params(const char *file_name, const char *const *parms, struct stat *s) {
    const char *command = NULL, *nextParam = NULL;
    int i = 0;
    bool printed = false, stop = false;
    param_context_t paramc = {NULL, NULL, NULL, NULL, NULL, false, false, false, ""};

    paramc.file_name = file_name;
    paramc.file_stat = s;

    //TODO: Weiter kommentieren
    //save current param to command and increment counter
    while ((command = parms[i++]) != NULL) {
        nextParam = parms[i];

        opt_t opt = get_opt(command);
        debug_print("DEBUG: found arg '%s' on '%d' as OPT:'%lu'\n", command, i-1, (long)opt);

        if(opt == UNKNOWN){
            error(MF_ERR_INVALID_ARGUMENT, 0, "invalid argument '%s'", command);
            return;
        }

        retval_cv_t cvret = check_value(opt, nextParam);
        debug_print("DEBUG: checked value '%s' got '%d' \n", nextParam, cvret);
        switch(cvret) {
            case UNEXPECTED_PARAM:
                error(MF_ERR_UNEXPECTED_ARGUMENT, 0, "unexpected value '%s' on '%s'", parms[i], command);
                return;
            case MISSING_PARAM:
                error(MF_ERR_MISSING_ARGUMENT, 0, "missing value on '%s'", command);
                return;
            default:
                break;
        }

        //increment to skip value if value needed;
        if(cvret == VALUE_OK)
            i++;

        paramc.value = nextParam;

        stop = !handle_param(opt, &paramc);

        if (stop)
            break;

        switch(opt) {
            case PRINT:
            case LS:
                printed = true;
                break;
            default:
                break;
        }
    }

    if (!printed && !stop)
        do_param_print(&paramc);
}

/**
 * get_opt Funktion
 * Diese Funktion überprüft ob der gegebene Command-String einer gültigen Option entspricht.
 *
 * \param command zu überprüfender Command-String
 *
 * \return opt_t gibt als Ergebnis einen Wert des Enums OPT zurück.
 * \relates OPT
 */
static opt_t get_opt(const char *command) {
    //start on index 1 because 0 is UNKNOWN = NULL
    for(size_t j = 1; j < OPTS_COUNT; j++) {
        const char *current_opt = OPT_NAME[j];
        if (strcmp(current_opt, command) == 0) {
            return (opt_t) j;
        }
    };

    return UNKNOWN; //default value if no valid param is found
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
static retval_cv_t check_value(opt_t opt, const char *next_parm) {
    switch (opt) {
        case PRINT:
        case LS:
        case NOUSER:
            //if no value is expected check if next param is null or a valid arg (start with '-')
            if (next_parm != NULL && next_parm[0] != '-')
                return UNEXPECTED_PARAM;
            return NO_VALUE;
        case USER:
        case TYPE:
        case NAME:
        case PATH:
            // if value is needed check if not null
            if (next_parm == NULL)
                return MISSING_PARAM;
            return VALUE_OK;
        default:
            return UNKNOWN_OPT;
    }
}

/**
 * Ruft die einzelnen unterfunktionen basierend auf der OPT auf.
 * TODO: vernünftig Kommentieren
 */
static bool handle_param(opt_t opt, param_context_t *paramc) {
    debug_print("DEBUG: handle param '%d'\n", opt);
    //dependency handling with fallthrought
    switch (opt){
        case NOUSER:
        case LS:
            if(!paramc->group_loaded) {
                paramc->group = getgrgid(paramc->file_stat->st_gid);
                paramc->group_loaded = true;
            }
        case TYPE:
            if(!paramc->permissions_loaded) {
                snprintf_permissions(paramc->permissions, 11, paramc->file_stat->st_mode);
                paramc->permissions_loaded = true;
            }
        case USER:
            if(!paramc->user_loaded) {
                paramc->user = getpwuid(paramc->file_stat->st_uid);
                paramc->user_loaded = true;
            }
        default:
            break;
    }

    //call param method
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
            error(MF_ERR_NOT_IMPLEMENTED, 0, "NOT IMPLEMENTED: can't unhandle command '%s'!", OPT_NAME[opt]);
            return false;
    }
}

/**
 * do_param_print Funktion
 * Diese Funktion gibt den Namen des Files zurück.
 *
 * \param file_name
 *
 * \func printf()
 *
 * \return Anzahl der charakter
 */
static bool do_param_print(const param_context_t *paramc) {
    errno = 0;
    if (fprintf(stdout, "%s\n", paramc->file_name) < 0) {
        error(MF_ERR_NONCRITICAL, errno, "can't write to stdout");
        return false;
    }
    return true;
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
 * TODO: ist eigentlich void, wie kann man das anders/besser formulieren?
 *
 * \return always 'true'
 * \return '1' always
 */
static bool do_param_list(const param_context_t *paramc) {
    const struct stat *s = paramc->file_stat;

    // Get Last Modified Time
    char timetext[80];
    snprintf_filetime(timetext, sizeof(timetext), &(s->st_mtim.tv_sec));

    // Get User Name
    char user_name[255]; // TODO: read buffer-size with sysconf()
    if (snprintf_username(user_name, sizeof(user_name), paramc->user) == 0)
        sprintf(user_name, "%d", s->st_uid);

    // Get Group Name
    char group_name[255]; // TODO: read buffer-size with sysconf()
    snprintf_groupname(group_name, sizeof(group_name), paramc->group);

    // Get Permissions
    char permissions[11];
    snprintf_permissions(permissions, sizeof(permissions), paramc->file_stat->st_mode);

    fprintf(stdout, "%6lu %4lu", s->st_ino, s->st_blocks / 2); // stat calculates with 512bytes blocksize ... 1024 should be used
    fprintf(stdout, " %s ", permissions);
    fprintf(stdout, "%3d %-8s %-8s %8lu %s %s\n", (int)s->st_nlink, user_name, group_name, s->st_size, timetext, paramc->file_name);

    return true;
}

/**
 * snprintf_filetime Funktion
 * Diese Funktion gibt die Zeit der letzten Änderung des Files zurück.
 *
 * \param buf
 * \param buffsize
 * \param time
 *
 * \func localtime_r() gibt die lokale Zeit im struct 'tm' zurück und nutzt 'lt' zum speichern des Ergebnisses.
 * \func strftime() gibt die Anzahl der geschriebenen Charakter zurück, oder '0' wenn das Maximum überschritten
 *      wird.
 *
 * \return Anzahl der geschriebenen Charakter, wenn erfolgreich.
 * \return '0' im Fehlerfall.
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
 * \return 0 Wenn kein User in passwd gefunden wurde
 * \return >0 Gibt länge des Usernamen zurück
 */
//TODO: Wenn NULL als buf übergeben wird, trotzdem ausgabe der Länge
static size_t snprintf_username(char *buf, size_t bufsize, const struct passwd *usr) {
    if(usr == NULL)
        return 0;

    strncpy(buf, usr->pw_name, bufsize);
    return strlen(usr->pw_name);
}

/**
 * snprintf_groupname Funktion
 * Diese Funktion sucht nach einen Eintrag mit der passenden Group-ID und speichert diese in '*grp' zwischen
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
 * \return strln wenn erfolgreich
 */
//TODO: Kommentar ausbessern
static size_t snprintf_groupname(char *buf, size_t bufsize, const struct group *grp) {
    if (grp == NULL)
        return 0;

    strncpy(buf, grp->gr_name, bufsize);
    return strlen(grp->gr_name);
}

/**
 * snprintf_permissions Funktion
 * Diese Funktion gibt die Permissions aus.
 * Es wird ein Array mit 12 Elementen initialisiert.
 *
 * Für das Element '0' wir überprüft:
 * 'b' = 'S_ISBLK' => block Device
 * 'c' = 'S_ISCHR' => char Device
 * 'd' = 'S_ISDIR' => Verzeichnis
 * 'p' = 'S_ISFIFO' => FiFo
 * 'l' = 'S_ISLNK' => symbolische Verknüpfung
 * 's' = 'S_ISOCK' => Socket
 *
 * Für das Element '1','4','7' wird überpfüft:
 * Wenn 'mode' & 'S_ISUSR' (Besitzer/Gruppe/Andere hat Lesezugriff), dann 'r'
 *
 * Für Element '2','5','8' wird überprüft:
 * Wenn 'mode' & 'S_IWUSR' (Besitzer/Gruppe/Andere hat Schreibzugriff), dann 'w'
 *
 * Für Element '3','6','9' wird überprüft:
 * Wenn check_flags() == true, dann 's'
 *      'S_IXUSR' (Besitzer hat Ausführrechte) | 'S_ISUID' (SUID-Bit)
 * Wenn 'mode' & 'S_IXUSR', dann 'x'
 * Wenn 'mode' & 'S_ISUID', dann 'S'
 * TODO: Nur Flags beschreiben, rest in code kommentieren?
 *
 * \param buf
 * \param bufsize
 * \param mode
 *
 * \func check_flags() Makro für Bitmask Flag-Checking
 * TODO: Verlinken?
 * \func strncpy() speichert 'lbuf' (Zeiger auf Quell-Array) in 'buf' (Zeiger auf Ziel-Array).
 */
static size_t snprintf_permissions(char *buf, size_t bufsize, int mode) {
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
    return 10; //number im characters that should be written to buffer
}

/**
 * do_param_nouser Funktion
 * Diese Funktion ...
 * TODO: Geht in die Funktion 'snprintf_username' und übergibt die Werte. Wnn '0' zurück kommt dann? Wenn != '0' zurück kommt dann?
 */
static bool do_param_nouser(const param_context_t *paramc) {
    return paramc->user == NULL;
}

/**
 * do_param_user Funktion
 * Diese Funktion sucht einen Eintrag einem passenden Usernamen und speichert diesen in 'pass' ab.
 * Wenn 'pass' == 'NULL' wird in 'uid' ein integer gespeichert
 * TODO: Welcher integer?
 * Wenn nicht wird in 'uid' 'pass->pw_uid' gespeichert
 *
 * \param value
 * \param s
 *
 * \func getpwnam() Sucht einen Eintrag mit einem passenden Usernamen
 * \func strtoul() Konvertiert einen String zu einen unsigned long integer
 *
 * \return 'uid' always
 */
static bool do_param_user(const param_context_t *paramc) {
    char *tmp;
    unsigned long uid;

    uid = strtoul(paramc->value, &tmp, 0);

    if (*tmp != '\0') {
        errno = 0;
        struct passwd *usr = getpwnam(tmp);

        if(usr == NULL) {
            error(MF_ERR_NO_USER_FOUND, errno, "Can't find user '%s'", paramc->value);
            return false;
        }

        uid = usr->pw_uid;
    } else { return false; }

    return uid == paramc->file_stat->st_uid;
}

/**
 * do_param_type Funktion
 * TODO: copy from print_permissions
 *
 * \param value
 * \param s
 *
 * \func strlen() gibt die Länge des Strings zurück.
 *
 * \return '0' wenn (s->st_mode & mask) == mask
 * \return TODO: was wird zurück gegeben, wenn '1'?
 */
static bool do_param_type(const param_context_t *paramc) {
    mode_t mask = 0;
    if (strlen(paramc->value) != 1) {
        error(MF_ERR_INVALID_TYPE_ARGUMENT, 0, "invalid type value '%s'", paramc->value);
        return false;
    }

    switch (paramc->value[0]) {
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
        error(MF_ERR_INVALID_TYPE_ARGUMENT, 0, "invalid flag on type '%c'", paramc->value[0]);
        break;
    }

    return (paramc->file_stat->st_mode & mask) == mask;
}

/**
 * do_param_name Funktion
 * Diese Funktion gibt zurück ob der Name gefunden wurde oder nicht.
 * TODO: Stimmt das? Neein siehe CIS ;)
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
static bool do_param_name(const param_context_t *paramc) {
    int result = 0;

    char *name = basename((char *) paramc->file_name);
    result = fnmatch(paramc->value, name, 0);
    debug_print("DEBUG: do_param_name for '%s' with '%s' => %d\n", paramc->file_name, paramc->value, result);

    if (result == 0)
        return true;
    else if (result == FNM_NOMATCH)
        return false;

    error(MF_ERR_INVALID_PATTERN, errno, "Pattern '%s' failed on '%s' with code '%d'", paramc->value, paramc->file_name, result);
    return false;
}

/**
 * do_param_path Funktion
 * Diese Funktion gibt zurück ob der Pfad gefunden wurde oder nicht.
 * TODO: Stimmt das? Nein siehe CIS ;)
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
static bool do_param_path(const param_context_t *paramc) {
    int result = 0;

    if (S_ISDIR(paramc->file_stat->st_mode))
        return false;

    result = fnmatch(paramc->value, paramc->file_name, 0);

    if (result == 0)
        return true;
    else if (result == FNM_NOMATCH)
        return false;

    error(MF_ERR_INVALID_PATTERN, errno, "Pattern '%s' failed on '%s' with code '%d'", paramc->value, paramc->file_name, result);
    return false;
}
