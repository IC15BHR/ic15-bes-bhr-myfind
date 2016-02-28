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
 * @version 2
 *
 * @todo comments
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

/*
 * -------------------------------------------------------------- defines --
 */
#define ARG_MIN 2

#ifndef DEBUG
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
/*
 * -------------------------------------------------------------- globals --
 */
/*
 * -------------------------------------------------------------- prototypes --
 */
void do_help(void);

void do_file(const char *file_name, const char *const *parms);
void do_dir(const char *dir_name, const char *const *parms);
void do_params(const char *name, const char *const *parms);

int do_print(const char *file_name);

/*
 * -------------------------------------------------------------- constants --
 */
enum OPT { UNKNOWN, PRINT, LS, USER, NAME, TYPE, NOUSER, PATH };

const char *const OPTS[] = {"", "-print", "-ls", "-user", "-name", "-type", "-nouser", "-path"};

/*
 * -------------------------------------------------------------- functions --
 */

/**
 * Main Funktion
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
 * //TODO: Comment
 */
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
 * \return kein Return-Wert da "void".
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
 *
 * \return kein Return-Wert da "void".
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
 * geht diese in einer While-Schleife durch und führt bei
 * Treffer das jeweilige If-Statement / Funktion aus.
 *
 * \param file_name
 * \param params
 *
 * \func do_print() gibt den Filenamen auf der Konsole aus.
 * \func do_list() gibt username, groupname und permissions aus.
 * \func do_nouser()
 * \func do_user()
 * \func do_type()
 * \func do_name()
 * \func do_path()
 *
 * \return kein Return-Wert da "void"
 */
void do_params(const char *file_name, const char *const *parms) {
    const char *command;
    int i = 0;
    bool printed = false;

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

        // print, ls, nouser can't have a value
        if (opt == PRINT || opt == LS || opt == NOUSER) {
            if (parms[i] != NULL && parms[i][0] != '-') {
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

        error(100, 0, "NOT IMPLEMENTED: can't unhandle command '%s'!", command);
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
 * \return int ohne Return?
 * TODO: Return value?
 */
int do_print(const char *file_name) { return printf("%s\n", file_name); }
