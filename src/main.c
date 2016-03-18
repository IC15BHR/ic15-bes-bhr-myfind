/**
 * @file main.c
 * Betriebssysteme MyFindBetriebssysteme MyFind
 * Beispiel 1
 *
 * Dies ist das Main-Modul des Programms MyFind
 *
 * Es akzeptiert diese möglichen Argumente:
 * -ls, -user, -name, -type, -nouser, -path.
 *
 * Die Funktionsweise ist an das unter Linux verbreitete Programm "find" angelehnt
 * Es durchsucht das gegebene Verzeichnis nach weiteren Verzeichnissen und Files und
 * validiert diese gegen eine beliebige Anzahl an Expression-Argumenten um zutreffende
 * Files und Directories auszugeben. Die Ausgabe kann mit bestimmten Parametern formatiert
 * werden.
 *
 * @author Baliko Markus	    <ic15b001@technikum-wien.at>
 * @author Haubner Alexander    <ic15b033@technikum-wien.at>
 * @author Riedmann Michael     <ic15b054@technikum-wien.at>
 *
 * @date 2016/03/18
 *
 * @version 2.0
 *
 */

// -------------------------------------------------------------- includes --
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <error.h>
#include <errno.h>

#include <dirent.h>
#include <sys/stat.h>

#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fnmatch.h>

// -------------------------------------------------------------- defines --
#define ARG_MIN 2
#define OPTS_COUNT sizeof(OPT_NAME) / sizeof(OPT_NAME[0])

#ifndef DEBUG // to make -DDEBUG gcc flag possible
#define DEBUG 0
#endif

#define USERNAME_MAX 255
#define GROUPNAME_MAX 255
#define PERMISSIONS_TEXT_SIZE 11
#define TIME_TEXT_SIZE 80

#define ANSI_COLOR_YELLOW "\033[0;33m"
#define ANSI_COLOR_RESET "\033[0m"

#define debug_print(fmt, ...)                                                                                          \
    do {                                                                                                               \
        if (DEBUG)                                                                                                     \
            fprintf(stdout, ANSI_COLOR_YELLOW fmt ANSI_COLOR_RESET, __VA_ARGS__);                                      \
    } while (0)

#define check_flags(mode, flags) (mode & flags) == flags

// -------------------------------------------------------------- typedefs --

/**
 * OPT
 * \brief Wird für die schnellere Verarbeitung der Argumente verwendet.
 *
 * Jeder Eintrag bezieht sich mit seinem Wert auf einen Eintrag im OPT_NAME-Array
 */
typedef enum OPT {
    INVALID = 0, //!< invalid opt. Wird zur überprüfung verwendet
    PRINT = 1,   //!< output print. Gibt den Namen des gefundenen Elements über stdout aus
    LS = 2,      //!< output list. Gibt pro gefundenes Element eine formatierte Tabellenzeile aus
    USER = 3,    //!< filter by fileowner. Filtert die Ausgabe auf einen angegebenen File-Owner
    NAME = 4,    //!< filter by filename. Filtert die Ausgabe über ein angegebenes Filename-Pattern
    TYPE = 5,    //!< filter by filetype. Filtert die Ausgabe über einen angegebenen Filetype
    NOUSER = 6,  //!< filter by no valid fileowner. Gibt Files aus die keinen gültigen Besitzer haben.
    PATH = 7     //!< filter by path. Filtert die Ausgabe auf Files die einen Pfad besitzer der einem Pattern entspricht
} opt_t;

/**
 * \brief Wird als pseudo-interface für die parameter-verarbeitungs Funktionen verwendet
 *
 * Dieses Context-Struct wird in der Funktion do_params erstellt und dient dazu einheitliche Arbeitsinformationen
 * über das aktuelle File oder Directory an die weiteren Funktionen durchreichen zu können.
 */
typedef struct PARAM_CONTEXT {
    const char *file_name;        //!< pfad der zu bearbeitenden Datei
    const struct stat *file_stat; //!< metadaten der Datei
} param_context_t;

/**
 * \brief wird für die parameter-prüfung verwendet
 */
typedef struct PARAM {
    opt_t opt;   //!< gefundene Option oder invalid-flag
    char *value; //!< Erweiterte Benutzereingabe bei dynamischen Argumenten (z.B. -name <pattern>)
} param_t;

/**
 *  \brief enum das alle möglichen Status-Codes der Applikation enthält.
 *         Wird auch von Funktionen genützt um detailierte Fehlerinfos durchzureichen.
 */
typedef enum {
    // non error codes
    OK_NOERROR = 0,         //!< Kein Fehler
    OK_PROCEED = 1,         //!< mit Parameter-Verarbeitung vortfahren
    OK_STOP = 2,            //!< Parameter-Verarbeitung abbrechen
    OK_VALUE_NOTNEEDED = 3, //!< Keine zusetzliche Eingabe zu diesem Parameter notwendig
    OK_VALUE_EXISTS = 4,    //!< Zusätzliche Eingabe zu diesem Parameter vorhanden

    // error codes
    ERR_NONCRITICAL = 0,            //!< Fehler der das Programm nicht anhält
    ERR_TO0_FEW_ARGUMENTS = -1,     //!< Zu wenige Argumente übergeben
    ERR_INVALID_ARGUMENT = -2,      //!< Ungültiges Argument übegeben
    ERR_VALUE_UNEXPECTED = -3,      //!< Keine zusätzliche Eingabe erwartet aber erhalten
    ERR_VALUE_MISSING = -4,         //!< Zusätzliche Eingabe erwartet aber keine erhalten
    ERR_NO_USER_FOUND = -5,         //!< Übergabener Benutzer wurde nicht gefunden
    ERR_OUTPUT_BROKEN = -6,         //!< Fehler bei der Ausgabe
    ERR_INVALID_TYPE_ARGUMENT = -7, //!< Ungültiger Type eingegeben
    ERR_INVALID_PATTERN = -8,       //!< Ungültiges Pattern eingegeben
    ERR_NOT_IMPLEMENTED = -255,     //!< Noch nicht implementiert
} retval_t;

// -------------------------------------------------------------- prototypes --
static void do_help(void);

static retval_t do_file(const char *file_name, const char *const *parms);
static retval_t do_dir(const char *dir_name, const char *const *parms);

static retval_t do_params(const char *name, const char *const *parms, struct stat *file_stat);
static retval_t get_param(const char *command, const char *next_param, param_t *param);
static retval_t strtoopt(const char *command, opt_t *opt);
static retval_t check_value(opt_t opt, const char *next_parm);
static retval_t handle_param(const param_t *param, const param_context_t *paramc);

static retval_t do_param_print(const param_context_t *paramc);

static retval_t do_param_list(const param_context_t *paramc);
static char get_file_type(int mode);
static size_t snprintf_permissions(char *buf, size_t bufsize, int mode);
static size_t snprintf_username(char *buf, size_t bufsize, uid_t uid);
static size_t snprintf_groupname(char *buf, size_t bufsize, gid_t gid);
static size_t snprintf_filetime(char *buf, size_t bufsize, const time_t *time);

static retval_t do_param_nouser(const param_context_t *paramc);
static retval_t do_param_user(const param_t *param, const param_context_t *paramc);
static retval_t do_param_type(const param_t *param, const param_context_t *paramc);
static retval_t do_param_name(const param_t *param, const param_context_t *paramc);
static retval_t do_param_path(const param_t *param, const param_context_t *paramc);

static void handle_error(const param_t *param, int result);

// -------------------------------------------------------------- constants --
/**
 * \brief wird verwendet um die Benutzereingaben zu validieren. Index entspricht Wert des OPTs
 *
 */
static const char *const OPT_NAME[] = {"", "-print", "-ls", "-user", "-name", "-type", "-nouser", "-path"};

// -------------------------------------------------------------- functions --

/**
 * \brief Programm Einstiegspunkt.
 *
 * Führt eine grundlegende Prüfung der Eingabe durch und gibt Pfad und Argumente an do_file weiter.
 *
 * \param argc ist die Anzahl der Argumente welche übergeben werden.
 * \param argv ist das Argument selbst.
 *
 * \func do_help() wird aufgerufen, wenn zu wenig Argumente übergeben werden.
 * \func do_file() wird aufgerufen, wenn eine richtige Anzahl an Argumenten übergeben wurde.
 *
 * \return gibt einen eigenen result-code zurück. Siehe "errorcodes"
 */
int main(int argc, char *argv[]) {
    int result;

    if (argc < ARG_MIN) {
        do_help();
        error(ERR_TO0_FEW_ARGUMENTS, 0, "Too few arguments given!");
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
    debug_print("DEBUG: Finished execution! Exitcode: '%d'\n", result);

    //returning positive errornumber if error happend
    return (unsigned int)result;
}

/**
 * \brief Gibt Hilfetext für Benutzer aus
 *
 * Diese Funktion wird aufgerufen, wenn der User zu wenig Argumente übergibt.
 * Dem User wird vorgeschlagen, welche Argumente er nutzen kann.
 */
static void do_help(void) {
    (void)fprintf(stdout, "Usage: find <dir> <expressions>\n\nExpressions:\n"
                          "  -print              returns formatted list\n"
                          "  -ls                 returns formatted list\n"
                          "  -user   <name/uid>  file-owners filter\n"
                          "  -name   <pattern>   file-name filter\n"
                          "  -type   [bcdpfls]   node-type filter\n"
                          "  -nouser             filter nonexisting owners\n"
                          "  -path   <pattern>   path filter\n");
}

/**
 * \brief Diese Funktion überprüft ob es sich um ein directory handelt oder nicht
 *        und ruft, wenn kein Fehler passiert ist, do_params() auf.
 *
 * Wird ein Directory erkannt, wird zusätzlich do_dir aufgerufen.
 * Wird ein Fehler beim auslesen der Attribute erkannt wird die Verarbeitung abgebrochen.
 *
 * \param file_name ist der relative Pfad der zu prüfenden Datei
 * \param params übernimmt die angegebenen Argumente
 *
 * \func lstat() ließt die file-Attribute aus und speichert sie in einen Buffer
 * \func do_params() wird aufgerufen um die Parameter zu verarbeiten.
 * \func do_dir() wird zusätzlich aufgerufen wenn es sich um ein directory handelt.
 *
 * \return einen Statuscode der Auskunft über mögliche Fehler bei der Verarbeitung gibt
 */
static retval_t do_file(const char *file_name, const char *const *parms) {
    retval_t result;
    struct stat status;

    debug_print("DEBUG: do_file '%s'\n", file_name);

    errno = 0;
    if (lstat(file_name, &status) == -1) {
        error(ERR_NONCRITICAL, errno, "can't get stat of '%s'", file_name);
        errno = 0;
        result = OK_NOERROR; // do not panic on unreadable stat
    } else {
        result = do_params(file_name, parms, &status);

        // only go deeper if no error has happend
        if (result == OK_NOERROR && S_ISDIR(status.st_mode))
            result = do_dir(file_name, parms);
    }

    debug_print("DEBUG: ended do_file with '%d' \n", result);
    return result;
}

/**
 * \brief Diese Funktion verarbeitet ein Directory indem es alle Einträge durchgeht und an do_file() übergibt.
 *
 * \param dir_name Name des zu verarbeitenden Verzeichnisses
 * \param params angegebenen Argumente
 *
 * \func opendir() öffnet einen Directory-Stream um die Elemente des Directorys zu laden.
 * \func readdir() liefert einen Pointer zu einem "struct dirent" der den Eintrag beschreibt.
 * \func do_file() springt zurück (rekussive Aufruf) in die do_file Funktion.
 * \func closedir() schließt den Diretory-Stream wieder.
 *
 * \return einen Statuscode der Auskunft über mögliche Fehler bei der Verarbeitung gibt
 */
static retval_t do_dir(const char *dir_name, const char *const *parms) {
    struct dirent *dp;
    retval_t result = OK_NOERROR;

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

            // build path (before error-handling for better error-message quality)
            size_t pathsize = strlen(dir_name) + strlen(dp->d_name) + 2; // lengths + '/' + \0
            char path[pathsize];
            snprintf(path, pathsize, "%s/%s", dir_name, dp->d_name);

            // if readpath throws an error, print it and continue
            if (errno != 0) {
                error(ERR_NONCRITICAL, errno, "can't read dir '%s'", path);
                errno = 0;
                continue;
            }

            debug_print("DEBUG: readdir '%s' '%s'\n", dp->d_name, path);

            // process found file or directory
            result = do_file(path, parms);
            if (result != OK_NOERROR)
                break;
        }

        // don't panic on this, cause we can't do anything against it at this point
        if (closedir(dirp) == -1) {
            error(ERR_NONCRITICAL, errno, "faild to close dir '%s'", dir_name);
            errno = 0;
        }
    } else {
        // mostly because we are not allowed to, so no error-propagation needed
        error(ERR_NONCRITICAL, errno, "can't open dir '%s'", dir_name);
        errno = 0;
    }

    debug_print("DEBUG: ended do_dir with '%d' \n", result);
    return result;
}

/**
 * \brief Diese Funktion überprüft die eigegebenen Parameter,
 *        geht diese in einer While-Schleife durch und führt bei
 *        Treffer den jeweiligen 'case' aus.
 *
 * \param file_name ist der relative Pfad der zu prüfenden Datei
 * \param params übernimmt die angegebenen Argumente
 * \param file_stat ein Pointer auf das stat-struct der zu prüfenden Datei
 *
 * \func get_param() Analysiert die Usereingabe und gibt ein struct zurück
 * \func handle_param() Ruft die einzelnen unterfunktionen basierend auf der OPT auf.
 * \func handle_error() Errorhandling.
 * \func do_param_print() gibt den Filenamen auf der Konsole aus.
 *
 * \return OK_NOERROR wenn erfolgreich oder einen negativen Error-Code im Fehlerfall
 */
static retval_t do_params(const char *file_name, const char *const *parms, struct stat *file_stat) {
    const char *command = NULL;
    int i = 0;
    retval_t result = OK_PROCEED;
    param_context_t paramc = {file_name, file_stat};
    param_t param = {INVALID, NULL};
    bool printed = false;

    // save current param to command and increment counter
    while ((command = parms[i++]) != NULL) {
        // analyze parameter
        result = get_param(command, parms[i], &param);
        if (result < 0)
            break;
        else if (result == OK_VALUE_EXISTS)
            i++; // skip value and got to next argument

        // handle parameter
        result = handle_param(&param, &paramc);

        // handle possible error
        if (result != OK_PROCEED)
            break;

        //if output happend, set printed flag
        if(param.opt == LS || param.opt == PRINT)
            printed = true;
    }

    // print some error-info if error happend
    if (result < 0)
        handle_error(&param, result);
    else {
        // if no error or STOP happend and line is not already printed => do print
        if (result == OK_PROCEED && !printed)
            result = do_param_print(&paramc);

        // normalize positive return for better upstream handling
        if (result == OK_PROCEED || result == OK_STOP)
            result = OK_NOERROR;
    }

    debug_print("DEBUG: ended do_params with '%d' \n", result);
    return result;
}

/**
 * \brief Analysiert die Benutzereingabe und liefer bei Erfolg ein ein struct mit aufbereiteten Daten.
 *        Im Fehlerfall wird ein negativer Errorcode zurückgegeben.
 *
 * \param command zu überprüfender Command-String
 * \param next_param unausgewerteter parameter der dem momentanen argument folgt (kann NULL sein)
 * \param param Ausgabe-Pointer für das Ergebnis-Struct
 * \func strtoopt() löst den param-string zu einem OPT auf oder gibt einen Fehlercode zurück.
 * \func check_value() gibt als Ergebnis einen Wert des Enums RETVAL_CV zurück.
 *
 * \return OK_NOERROR wenn erfolgreich sonst einen negativen Errorcode
 */
static retval_t get_param(const char *command, const char *next_param, param_t *param) {
    retval_t result;
    opt_t opt;

    // set INVALID in case of an error
    param->opt = INVALID;

    // check argument validity
    if ((result = strtoopt(command, &opt)) < 0)
        return result;

    debug_print("DEBUG: found arg '%s' as OPT:'%lu'\n", command, (long)opt);

    // check value validity
    if ((result = check_value(opt, next_param)) < 0)
        return result;

    param->opt = opt;

    // handle positive results
    if (result == OK_VALUE_EXISTS) {
        debug_print("DEBUG: using value '%s'\n", next_param);
        param->value = strdup(next_param);
    }

    return result;
}

/**
 * \brief Diese Funktion überprüft ob der gegebene Command-String einer gültigen Option entspricht.
 *
 * Er wird dazu jeder Eintrag im OPT-Enum iteriert und deie dazugehörige
 * Zeichenkette aus OPT_NAME geholt. Diese wird dann gegen die gegebene
 * Zeichenkette geprüft. Bei einer Übereinstimmung wird das dezugehörige
 * Enum returniert.
 *
 * \param command zu überprüfender Command-String
 *
 * \return opt_t gibt als Ergebnis einen Wert des Enums OPT zurück.
 */
static retval_t strtoopt(const char *command, opt_t *opt) {
    *opt = INVALID;
    if (strlen(command) < 2 || command[0] != '-')
        return ERR_INVALID_ARGUMENT;

    for (size_t j = 0; j < OPTS_COUNT; j++) {
        const char *current_opt = OPT_NAME[j];
        if (strcmp(current_opt, command) == 0) {
            *opt = (opt_t)j;
            return OK_NOERROR;
        }
    };

    return ERR_INVALID_ARGUMENT; // default value if no valid param is found
}

/**
 * \brief Diese Funktion überprüft ob ein "value"-Zusatz zum Argument notwendig ist und überprüft die Gültigkeit.
 *
 * \param opt ausgewertetes Argument als Option-Wert
 * \param next_param unausgewerteter parameter der dem momentanen argument folgt (kann NULL sein)
 *
 * \return gibt einen positiven Statuscode zwischen zurück.Bei Erfolg einen Fehlercode < 0
 */
static retval_t check_value(opt_t opt, const char *next_parm) {
    switch (opt) {
    case PRINT:
    case LS:
    case NOUSER:
        // if no value is expected check if next param is null or a valid arg (start with '-')
        if (next_parm != NULL && next_parm[0] != '-')
            return ERR_VALUE_MISSING;
        return OK_VALUE_NOTNEEDED;
    case USER:
    case TYPE:
    case NAME:
    case PATH:
        // if value is needed check if not null
        if (next_parm == NULL)
            return ERR_VALUE_UNEXPECTED;
        return OK_VALUE_EXISTS;
    default:
        return ERR_NOT_IMPLEMENTED;
    }
}

/**
 * \brief Diese Funktion bekommt OPT übergeben und die einzelnen Unterfunktionen,
 *        basierend auf OPT auf.
 * \param opt ausgewertetes Argument als Option-Wert
 * \param paramc context-struct der zu bearbeitenden Datei
 *
 * \return reicht die return-codes der aufgerufenen do_param_*-Funktion weiter.
 */
static retval_t handle_param(const param_t *param, const param_context_t *paramc) {
    debug_print("DEBUG: handle param '%s' '%s'\n", OPT_NAME[param->opt], param->value);

    // call param method
    switch (param->opt) {
    case PRINT:
        return do_param_print(paramc);
    case LS:
        return do_param_list(paramc);
    case NOUSER:
        return do_param_nouser(paramc);
    case USER:
        return do_param_user(param, paramc);
    case TYPE:
        return do_param_type(param, paramc);
    case NAME:
        return do_param_name(param, paramc);
    case PATH:
        return do_param_path(param, paramc);
    default:
        // should only be hit after extending the existing implementation
        return ERR_NOT_IMPLEMENTED;
    }
}

/**
 * \brief Behandelt das -print Argument
 *
 * Es wird der Datei-Name in einer Zeile ausgegeben
 *
 * \param paramc context-struct der zu bearbeitenden Datei
 *
 * \func fprintf()
 *
 * \return Bei Erfolg OK_PROCEED, sonst einen negativen Fehler-Code
 */
static retval_t do_param_print(const param_context_t *paramc) {
    errno = 0;
    if (fprintf(stdout, "%s\n", paramc->file_name) < 0)
        return ERR_OUTPUT_BROKEN;

    return OK_PROCEED;
}

/**
 * \brief Behandelt das -ls Argument
 *
 * Diese Funktion ruft die Funktionen 'snprintf_filetime()', 'snprintf_username', 'snprintf_groupname'
 * und 'snprintf_permissions' auf, übergibt diesen die jeweiligen Parameter
 * und gibt anschließend die Werte mittels 'fprintf()' aus.
 *
 * \param paramc context-struct der zu bearbeitenden Datei
 *
 * \func snprintf_filetime() bekommt eine Zahl != 0 zurück wenn erfolgreich und '0' im Fehlerfall.
 * \func snprintf_username() bekommt strln zurück wenn erfolgreich und '0' im Fehlerfall.
 * \func snprintf_groupname() bekommt strln zurück wenn erfolgreich und '0' im Fehlerfall.
 * \func snprintf_permissions() übergibt über strncpy() den Array für die Permissions.
 *
 * \return Bei Erfolg OK_PROCEED, sonst einen negativen Fehler-Code
 */
static retval_t do_param_list(const param_context_t *paramc) {
    const struct stat *s = paramc->file_stat;

    // Get Last Modified Time
    char timetext[TIME_TEXT_SIZE];
    snprintf_filetime(timetext, sizeof(timetext), &(s->st_mtim.tv_sec));

    // Get User Name
    char user_name[USERNAME_MAX];
    if (snprintf_username(user_name, sizeof(user_name), paramc->file_stat->st_uid) == 0)
        sprintf(user_name, "%d", s->st_uid);

    // Get Group Name
    char group_name[GROUPNAME_MAX];
    (void) snprintf_groupname(group_name, sizeof(group_name), paramc->file_stat->st_gid);

    // Get Permissions
    char permissions[PERMISSIONS_TEXT_SIZE];
    (void) snprintf_permissions(permissions, sizeof(permissions), paramc->file_stat->st_mode);

    // on error return error-code, otherwise return PROCEED
    return (fprintf(stdout,
                    "%6lu "           // inode
                    "%4lu "           // blocksize
                    "%s "             // permissions
                    "%3d "            // linkcount
                    "%-8s "           // username/id
                    "%-8s "           // groupname/id
                    "%8lu "           // size in byte
                    "%s "             // last modified time
                    "%s\n",           // filename/path
                    s->st_ino,        // inode
                    s->st_blocks / 2, // blksize - stat calculates with 512bytes blocksize ... 1024 should be used
                    permissions,      // permissions
                    (int)s->st_nlink, // linkcount
                    user_name,        // username/id
                    group_name,       // groupname/id
                    s->st_size,       // size in byte
                    timetext,         // last modified time
                    paramc->file_name // filename/üath
                    ) < 0)
               ? ERR_OUTPUT_BROKEN
               : OK_PROCEED;
}

/**
 * \brief Diese Funktion gibt die Zeit der letzten Änderung des Files zurück.
 *
 * \param buf Char-Buffer für Ergebnis. Mindestens 80 Zeichen lang
 * \param buffsize Buffergröße um Buffer-Overflows zu verhindern
 * \param time Wert in UNIX_TIMESTAMP Format
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
 * \brief Diese Funktion sucht nach einer passenden User-ID, speichert diese in '*usr' zwischen,
 * überprüft danach ob es sich um einen NULL-Pointer oder um einen Eintrag handelt und gibt, im Falle eines
 * Eintrages, den Namen zurück.
 *
 * \param buf Char-Buffer für Ergebnis. NULL übergeben um nur zu validieren.
 * \param buffsize Buffergröße um Buffer-Overflows zu verhindern. Wenn buf NULL hier 0 übergeben.
 * \param uid User-ID
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
 * \brief Sucht den Gruppennamen zu einer GroupID. Gibt die länge des Namen zurück oder 0 im Fehlerfall
 *
 * Benützt getgrgid() um die Gruppe auszulesen.
 * Wenn der Pointer == 'NULL', dann konnte die Group-ID nicht gefunden werden.
 * Wenn der Pointer != 'NULL', dann wird der Gruppenname zurück gegeben.
 * Kopiert bei Erfolg den Namen der erhalten Gruppe in buf.
 *
 * \param buf Char-Buffer für Ergebnis. NULL übergeben um nur zu validieren.
 * \param buffsize Buffergröße um Buffer-Overflows zu verhindern. Wenn buf NULL hier 0 übergeben.
 * \param gid Gruppen-ID
 *
 * \func getgrgid() Sucht nach einen Eintrag mit der passenden Group-ID. Gibt 'NULL' im Fehlerfall zurück.
 * \func strncpy() speichert 'grp->gr_name' (Zeiger auf Quell-Array) in 'buf' (Zeiger auf Ziel-Array).
 * \func strln() gibt die Länge des String zurück.
 *
 * \return länge des Gruppennamens oder 0 im Fehlerfall
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
 * \brief Diese Funktion gibt die Permissions aus.
 *
 * \param buf Char-Buffer für Ergebnis. Mindestens 11 Zeichen lang
 * \param buffsize Buffergröße um Buffer-Overflows zu verhindern.
 * \param mode Das mode Bitmask-Field der zu untersuchenden Datei
 *
 * \func get_file_type() Wird aufgerufen um den Filetype zu ermitteln
 * \func check_flags() Makro für Bitmask Flag-Checking
 * \func strncpy() speichert 'lbuf' (Zeiger auf Quell-Array) in 'buf' (Zeiger auf Ziel-Array).
 *
 * \return Gibt die länge des Permission-Texts aus.
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
 * \brief gibt anhand des Mode-Bitmask-Fields einer Datei deren Typ als character zurück
 *
 * 'b' = 'S_ISBLK' => block Device
 * 'c' = 'S_ISCHR' => char Device
 * 'd' = 'S_ISDIR' => Verzeichnis
 * 'p' = 'S_ISFIFO' => FiFo
 * 'l' = 'S_ISLNK' => symbolische Verknüpfung
 * 's' = 'S_ISOCK' => Socket
 *
 * \param mode Das mode Bitmask-Field der zu untersuchenden Datei
 *
 * \return einer der folgenden characters "bcdplsf" oder '-' wenn kein passendes Flag gefunden wurde
 */
static char get_file_type(int mode) {
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
 * \brief Behandelt das -nouser Argument
 *
 * \param paramc context-struct der zu bearbeitenden Datei
 *
 * \func snprintf_username() wird verwendet um die Benutzerid zu prüfen
 *
 * \return returniert PROCEED wenn kein Benutzer für die Datei gefunden wurde und sonst STOP
 */
static retval_t do_param_nouser(const param_context_t *paramc) {
    size_t name_len = snprintf_username(NULL, 0, paramc->file_stat->st_uid);
    return (name_len == 0) ? OK_PROCEED : OK_STOP;
}

/**
 * \brief Behandelt das -user Argument
 *
 * Versucht einen gültigen Benutzer aus dem angegebenen Zusatz über die passwd aufzulösen.
 * Gelingt dies nicht wird versucht den Zusatz als UserId zu lesen.
 * Schlagen alle Versuche fehl wird ein Fehler zurückgegeben.
 * Wird der User richtig aufgelöst wird er gegen den Besitzer der Datei verglichen.
 *
 * \param paramc context-struct der zu bearbeitenden Datei
 *
 * \func getpwnam() Sucht einen Eintrag mit einem passenden Usernamen
 * \func strtoul() Konvertiert einen String zu einen unsigned long integer
 *
 * \return returniert PROCEED wenn der angegebene Benutzer Besitzer der Datei ist, sonst STOP.
 *         Kann der angegebene Benutzer nicht aufgelöst werden wird eine Errorcode zurückgegeben
 */
static retval_t do_param_user(const param_t *param, const param_context_t *paramc) {
    char *tmp;
    ulong uid;

    debug_print("DEBUG: resolving user '%s'\n", param->value);

    errno = 0;
    struct passwd *usr = getpwnam(param->value);

    // no user in passwd, check if userid is given
    if (usr == NULL) {
        uid = strtoul(param->value, &tmp, 0);

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
 * \brief Behandelt das -type Argument
 *
 * Überprüft ob der eingegebene Type einem validen Type entspricht.
 * Ist die Eingabe nicht genau ein Zeichen wird ein Errorcode geliefert.
 * Liefert einen Errorcode wenn Type nicht eines dieser Zeichen: "bcdpfls"
 *
 * Ansonsten wird der Type der Datei gegen den eingegebenen Type geprüft.
 *
 * \param paramc context-struct der zu bearbeitenden Datei
 *
 * \func strlen() gibt die Länge des Strings zurück.
 *
 * \return wenn der Type der Datei dem gesuchten Type entspricht wird PROCEED zurückgegeben, sonst STOP
 */
static retval_t do_param_type(const param_t *param, const param_context_t *paramc) {
    const char *valid_flags = "bcdpfls";

    if (strlen(param->value) != 1)
        return ERR_INVALID_TYPE_ARGUMENT;

    char type_flag = param->value[0];

    // check validity
    while (*valid_flags != type_flag && *valid_flags != '\0')
        valid_flags++;

    if ((*valid_flags) == '\0')
        return ERR_INVALID_TYPE_ARGUMENT;

    return (get_file_type(paramc->file_stat->st_mode) == type_flag) ? OK_PROCEED : OK_STOP;
}

/**
 * \brief Behandelt das -name Argument
 *
 * Es wird der Basisteil des voll Datei-Namen über das gegebene Pattern verglichen.
 *
 * \param paramc context-struct der zu bearbeitenden Datei
 *
 * \func basename() gibt die letzte Komponente des Pfardes zurück.
 * \func fnmatch() überprüft ob 'name' mit 'value' übereinstimmt.
 *                 Wenn Übereinstimmung, dann '0'.
 *                 Wenn keine Übereinstimmung, dann 'FNM_NOMATCH'
 *
 * \return Wenn das Pattern mit dem Dateinamen übereinstimmt wird PROCEED zurückgegeben, ansonsten STOP.
 *         Im Fehlerfall wird ein negativer Errorcode zurückgeliefert.
 */
static retval_t do_param_name(const param_t *param, const param_context_t *paramc) {
    char *name = basename((char *)paramc->file_name);
    int result = fnmatch(param->value, name, 0);
    debug_print("DEBUG: do_param_name for '%s' with '%s' => %d\n", paramc->file_name, param->value, result);

    if (result == 0)
        return OK_PROCEED;
    else if (result == FNM_NOMATCH)
        return OK_STOP;

    return ERR_INVALID_PATTERN;
}

/**
 * \brief Behandelt das -path Argument
 *
 * Es wird der voll Datei-Namen (= Pfad) über das gegebene Pattern verglichen.
 *
 * \param paramc context-struct der zu bearbeitenden Datei
 *
 * \func fnmatch() überprüft ob 'file_name' mit 'value' übereinstimmt.
 *                 Wenn Übereinstimmung, dann 0.
 *                 Wenn keine Übereinstimmung, dann FNM_NOMATCH
 *
 * \return Wenn das Pattern mit dem Pfad übereinstimmt wird PROCEED zurückgegeben, ansonsten STOP.
 *         Im Fehlerfall wird ein negativer Errorcode zurückgeliefert.
 */
static retval_t do_param_path(const param_t *param, const param_context_t *paramc) {
    int result = fnmatch(param->value, paramc->file_name, 0);
    debug_print("DEBUG: do_param_path for '%s' with '%s' => %d\n", paramc->file_name, param->value, result);

    if (result == 0)
        return OK_PROCEED;
    else if (result == FNM_NOMATCH)
        return OK_STOP;

    return ERR_INVALID_PATTERN;
}

/**
 * \brief Dient zum Ausgeben von benutzerfreundlichen Fehlermeldungen
 *
 * \func error() Wird nur mit (0, 0, ...) aufgerufen um einen exit-call zu vermeiden
 */
static void handle_error(const param_t *param, int result) {
    const char *command = OPT_NAME[param->opt];

    switch (result) {
    case ERR_INVALID_ARGUMENT:
        error(0, 0, "invalid argument '%s'", command);
        break;
    case ERR_VALUE_UNEXPECTED:
        error(0, 0, "unexpected value '%s' on '%s'", param->value, command);
        break;
    case ERR_VALUE_MISSING:
        error(0, 0, "missing value on '%s'", command);
        break;
    case ERR_INVALID_TYPE_ARGUMENT:
        error(0, 0, "invalid type value '%s'", param->value);
        break;
    case ERR_NOT_IMPLEMENTED:
        error(0, 0, "NOT IMPLEMENTED: can't unhandle command '%s'!", command);
        break;
    case ERR_OUTPUT_BROKEN:
        error(0, errno, "can't write to stdout!");
        break;
    case ERR_NO_USER_FOUND:
        error(0, errno, "Can't find user '%s'", param->value);
        break;
    case ERR_INVALID_PATTERN:
        error(0, errno, "Pattern '%s' with code '%d'", param->value, result);
        break;
    default:
        break;
    }
}