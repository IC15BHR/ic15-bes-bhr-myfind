#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "helpers.h"

#define ARG_MIN 2

void do_file(const char *file_name, const char *const *parms);
void do_dir(const char *dir_name, const char *const *parms);

void do_print(const char *name);
void do_list(const char *file_name, const char *const *parms);

void do_help(void);

void do_params(const char *name, const char *const *pString);

void sprintf_permissions(const struct stat *fileStat, char *bufp);

void do_user(const char *file_name, const char *value, const char *const *parms);

static const char ARG_PRINT[] = "-print";
static const char ARG_LIST[] = "-ls";
static const char ARG_USER[] = "-user";
static const char ARG_NAME[] = "-name";
static const char ARG_TYPE[] = "-type";
static const char ARG_NOUSER[] = "-nouser";
static const char ARG_PATH[] = "-path";

/*
 * Argument Structure:
 * [0] = exec path
 * [1] = target path (first "file_name")
 * [n] = expressions
 */

int main(int argc, const char *argv[]) {
  if (argc < ARG_MIN) {
    do_help();
    exit_with_error(1, "Too few arguments given!\n");
  }

  do_file(argv[1], argv);

  return 0;
}

void do_file(const char *file_name, const char *const *parms) {
  if (file_name == NULL)
    return;

  DIR *dir = opendir(file_name); // try open dir
  if (dir != NULL) {
    do_dir(file_name, parms);
    closedir(dir);
  }

  do_params(file_name, parms);
}

void do_params(const char *file_name, const char *const *parms) {
  const char *p;
  int i = 2;
  int output_done = 0;

  while ((p = parms[i++]) != NULL) {
    const char* command = p;
    //== Arguments Without Values ==
    if (command[0] != '-')
      exit_with_error(1, "%s: %s invalid argument format\n", parms[0], command);

    if (is_str_equal(command, ARG_PRINT)) {
      do_print(file_name);
      output_done = 1;
      continue;
    }

    if (is_str_equal(command, ARG_LIST)) {
      do_list(file_name, parms);
      output_done = 1;
      continue;
    }

    if (is_str_equal(command, ARG_NOUSER)) {
      // TODO: Implement -nouser argument
      output_done = 0;
      continue;
    }

    //== Arguments With Values ==
    if (parms[i] == NULL || parms[i][0] == '-')
      exit_with_error(1, "%s: invalid argument format (value) %s\n",parms[0], p);

    const char* value = p++;
    if (is_str_equal(command, ARG_USER)) {
      // TODO: Implement -user <userid/name> argument
      do_user(file_name, value, parms);
      output_done = 0;
      continue;
    }

    if (is_str_equal(command, ARG_NAME)) {
      // TODO: Implement -name <name> argument
      output_done = 0;
      continue;
    }

    if (is_str_equal(command, ARG_TYPE)) {
      // TODO: Implement -type <type> argument
      output_done = 0;
      continue;
    }

    if (is_str_equal(command, ARG_PATH)) {
      // TODO: Implement -path <pattern> argument
      output_done = 0;
      continue;
    }

    exit_with_error(1, "invalid argument %s\n", command);
  }

  if (!output_done)
    do_print(file_name);
}

void do_user(const char *file_name, const char *value, const char *const *parms) {
  struct stat s;

  stat(file_name, &s);


}

void do_dir(const char *dir_name, const char *const *parms) {
  struct dirent *dp;
  DIR *dirp = opendir(dir_name);
  if (dirp != NULL) {
    while ((dp = readdir(dirp)) != NULL) {
      if (is_str_equal(dp->d_name, ".") || is_str_equal(dp->d_name, ".."))
        continue;

      char path[FILENAME_MAX];
      snprintf(path, sizeof path, "%s/%s", dir_name, dp->d_name);
      do_params(dir_name, parms);

      if (dp->d_type == DT_DIR)
        do_dir(path, parms);
      else
        do_file(path, parms);
    }
    closedir(dirp);
  } else {
    print_last_error(parms);
  }
}

void do_print(const char *file_name) { printf("%s\n", file_name); }

void do_list(const char *file_name, const char *const *parms) {
  struct stat s;
  struct tm lt;

  stat(file_name, &s);
  // Get Last Modified Time
  localtime_r(&s.st_mtime, &lt);
  char timetext[80];
  strftime(timetext, 80, "%b %d %H:%M", &lt);

  // Get User Name
  const struct passwd *usr = getpwuid(s.st_uid);
  if (usr == NULL) {
    print_last_error(parms);
    return;
  }

  // Get Group Name
  const struct group *grp = getgrgid(s.st_gid);
  if (grp == NULL) {
    print_last_error(parms);
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
         "  -print     returns name of directory\n"
         "  -ls        returns formatted list\n"
         "  -ls        returns formatted list\n"
         "  -ls        returns formatted list\n"
         "  -ls        returns formatted list\n");
}
