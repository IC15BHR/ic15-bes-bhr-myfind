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
#define debug_print(fmt, ...) do { if (DEBUG) fprintf(stdout, fmt, __VA_ARGS__); } while (0)

////// PROTOTYPES TODO: Sort prototypes by call-order!
void do_file(const char *file_name, const char *const *parms);
void do_dir(const char *dir_name, const char *const *parms);

void do_print(const char *name);
void do_list(const char *file_name);

void do_help(void);

void do_link(const char *file_name, const char *const *parms);
void do_params(const char *name, const char *const *parms);

void sprintf_permissions(const struct stat *fileStat, char *bufp);

void print_progname(void);

int do_user(const char *file_name, const char *value);
int do_pattern(const char *file_name, const char * pattern);
int do_type(const char *file_name, const char *value);
int do_name(const char *file_name, const char *value);
int do_nouser(const char *file_name);
int do_path(const char *file_name, const char *value);

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
    error(1, 0, "Too few arguments given!"); //TODO: maybe errorcode available?
  }

  char * targetpath = realpath(argv[1], NULL); //TODO: Memoryleak on error! Better way?
  debug_print("DEBUG: targetpath = %s\n", targetpath);

  //&argv[2] because "Anleitung" says so:
  //"parms ist dabei die Adresse des ersten Arguments (so wie beim argv), das eine Aktion ist"
  do_file(targetpath, &argv[2]);

  free(targetpath);
  return 0;
}

void do_dir(const char *dir_name, const char *const *parms) {
  struct dirent *dp;

  if (dir_name == NULL){
    debug_print("DEBUG: NULL dirname in do_dir%s","\n");
    return;
  }

  debug_print("DEBUG: do_dir '%s'\n", dir_name);

  DIR *dirp = opendir(dir_name);
  if (dirp != NULL) {
    //readdir returns NULL && errno=0 on EOF, errno != 0 is not EOF!
    while (((dp = readdir(dirp)) != NULL) || errno != 0) {
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
        debug_print("DEBUG: skip '%s' '%s' (%d)\n", dir_name, dp->d_name, errno);
        continue;
      }

      char path[PATH_MAX]; //TODO: Not efficient - Malloc/Realloc?
      snprintf(path, PATH_MAX, "%s/%s", dir_name, dp->d_name);

      if(errno != 0){
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

  if (file_name == NULL){
    debug_print("DEBUG: NULL filename in do_file %s","\n");
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
  const char *p;
  int i = 0;
  int output_done = 0;

  while ((p = parms[i++]) != NULL) {
    const char* command = p;
    //== Arguments Without Values ==
    if (command[0] != '-')
      error(1, 0, "%s invalid argument format", command);

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
      if(!do_nouser(file_name)) break;
      output_done = 0;
      continue;
    }

    //== Arguments With Values ==
    if (parms[i] == NULL)
      error(1, 0, "invalid argument format (value) %s", p);

    const char* value = p++; //save next arg into value and inc p

    if (strcmp(command, ARG_USER) == 0) {
      if(!do_user(file_name, value)) break;
      output_done = 0;
      continue;
    }

    if (strcmp(command, ARG_NAME) == 0) {
      if(!do_name(file_name, value)) break;
      output_done = 0;
      continue;
    }

    if (strcmp(command, ARG_TYPE) == 0) {
      if(!do_type(file_name, value)) break;
      output_done = 0;
      continue;
    }

    if (strcmp(command, ARG_PATH) == 0) {
      if(!do_path(file_name, value)) break;
      output_done = 0;
      continue;
    }

    error(1, 0, "invalid argument '%s'", command);
  }

  if (!output_done) //outputs print if no last print/ls is given in args
    do_print(file_name);
}

int do_nouser(const char *file_name) {
  //TODO: Implement nouser checking
  printf("check nouser on '%s'\n", file_name); //only for testing
  return true;
}

int do_type(const char *file_name, const char *value) {
  //TODO: Implement type checking
  printf("check type '%s' on '%s'\n", value, file_name); //only for testing
  return true;
}

int do_name(const char *file_name, const char *value) {
  //TODO: Implement name-check
  printf("check name '%s' on '%s'\n", value, file_name); //only for testing
  return true;
}

int do_user(const char *file_name, const char *value) {
  //TODO: Implement checking of username/userid
  printf("check user '%s' on '%s'\n", value, file_name); //only for testing
  return true;
}

int do_path(const char *file_name, const char *value) {
  //TODO: Implement path
  printf("check path '%s' on '%s'\n", value, file_name); //only for testing
  return true;
}

void do_print(const char *file_name) { printf("%s\n", file_name); }

void do_list(const char *file_name) {
  struct stat s;
  struct tm lt;

  stat(file_name, &s);
  // Get Last Modified Time
  localtime_r(&s.st_mtime, &lt);
  char timetext[80];
  strftime(timetext, 80, "%b %d %H:%M", &lt);

  // Get User Name
  errno = 0;
  const struct passwd *usr = getpwuid(s.st_uid);
  if (usr == NULL) {
    error(0, errno, "Failed to get user from id '%d'", s.st_uid);
    return;
  }

  // Get Group Name
  errno = 0;
  const struct group *grp = getgrgid(s.st_gid);
  if (grp == NULL) {
    error(0, errno, "Failed to get group from id '%d'", s.st_gid);
    return;
  }

  // Get Permissions
  char permissions[11];
  sprintf_permissions(&s, permissions);

  printf("%8lu %8lu", s.st_ino, s.st_size);
  printf(" %s ", permissions);
  printf("%2lu %10s %10s %13s %s\n", s.st_nlink, usr->pw_name, grp->gr_name,
         timetext, file_name);
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
