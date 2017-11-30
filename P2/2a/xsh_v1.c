#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

char error_message[30] = "An error has occurred\n";
int exit_status = 0;


int
typeHelper (char *args[], char *path[], int num_path)
{
  //char cand[128] = "/usr/bin/";
  //strcat(cand, args[1]);

  //int status;
  //struct stat *buffer = malloc(sizeof(struct stat*));

  int find_exe;
  struct stat *buffer = malloc(sizeof(struct stat));
  for (find_exe = 0; find_exe < num_path; find_exe++) {
    char cand[128];
    strcpy(cand, path[find_exe]);
    strcat(cand, "/");
    strcat(cand, args[1]);
    
    if (stat(cand, buffer) == 0) {
      write(STDOUT_FILENO, args[1], strlen(args[1]));
      write(STDOUT_FILENO, " is ", 4);
      write(STDOUT_FILENO, cand, strlen(cand));
      write(STDOUT_FILENO, "\n", 1);
      free(buffer);
      return 0;
    }
  }
  write(STDERR_FILENO, error_message, strlen(error_message));
  free(buffer);
  return 1;

/*
  status = stat(cand, buffer);
  if (status == 0) {
    write(1, args[1], strlen(args[1]));
    write(1, " is ", 4);
    write(1, cand, strlen(cand));
    write(1, "\n", 1);
    return 0;
  } else {
    write(2, error_message, strlen(error_message));
    return 1;
  }*/
}

void
printPrompt (char *cwd) {
  write(STDOUT_FILENO, "[", 1);
  write(STDOUT_FILENO, cwd, strlen(cwd));
  write(STDOUT_FILENO, "]\n",2);
  char tmp[1];
  sprintf(tmp,"%d", exit_status);
  write(STDOUT_FILENO, tmp, 1);
  write(STDOUT_FILENO, "> ", 2);
}

int
main (int argc, char *argv[])
{
  char cwd[1024];
  char cmd[130];
  char *sep = " \t\n";
  char *myArgv[128];
  char *path[128];

  path[0] = "/bin";
  int num_path = 1;

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printPrompt(cwd);
  } else
    perror("getcwd() error");
  
  while(1) {
    
    cmd[129] = '\n';
    fgets(cmd, 130, stdin);
    
    if ((cmd[129] == '\0') && (cmd[128] != '\n')) {
      exit_status = 1;
      write(STDERR_FILENO, error_message, strlen(error_message));
      while (getchar() != '\n') continue;
      printPrompt(cwd);
      memset(&myArgv[0], 0, sizeof(myArgv));
      continue;
    }

    int count;
    count = 0;
    char * myTok = strtok(cmd, sep);
    while (myTok != NULL) {
      myArgv[count] = myTok;
      //printf("%s\n", myArgv[count]);
      myTok = strtok(NULL, sep);
      count++;
    }

    if (count != 0) {
      if (strcmp(myArgv[0], "exit") == 0) {
        if (count == 1) {
          exit(0);
        } else {
          exit_status = 1;
          write(STDERR_FILENO, error_message, strlen(error_message));
        }
      } else if (strcmp(myArgv[0], "cd") == 0) {
        if (count == 1) {
          chdir(getenv("HOME"));
          getcwd(cwd, sizeof(cwd));
          exit_status = 0;
        } else if (count == 2) {
          int rc_cd = chdir(myArgv[1]);
          if (rc_cd == 0) {
            getcwd(cwd, sizeof(cwd));
            exit_status = 0;
          } else {
            exit_status = 1;
            write(STDERR_FILENO, error_message, strlen(error_message));
          }
        } else {
          exit_status = 1;
          write(STDERR_FILENO, error_message, strlen(error_message));
        }

      } else if (strcmp(myArgv[0], "path") == 0) {
        if (count == 1) {
          int itr;
          for (itr = 0; itr < num_path; itr++) {
            write(STDOUT_FILENO, path[itr], strlen(path[itr]));
            write(STDOUT_FILENO, "\n", 1);
          }
          exit_status = 0;
        } else {
          memset(&path[0], 0,sizeof(path));
          num_path = count - 1;

          int itr;
          for (itr = 1; itr < count; itr++) {
            path[itr-1] = malloc(*myArgv[itr]);
            strcpy(path[itr-1], myArgv[itr]);
          }
          exit_status = 0;
        }
      } else if (strcmp(myArgv[0], "type") == 0) {
        if (count != 2) {
          exit_status = 1;
          write(STDERR_FILENO, error_message, strlen(error_message));
        } else {
          if (strcmp(myArgv[1], "exit") == 0) {
            write(STDOUT_FILENO, "exit is a shell builtin\n", 25);
            exit_status = 0;
          } else if (strcmp(myArgv[1], "cd") == 0) {
            write(STDOUT_FILENO, "cd is a shell builtin\n", 22);
            exit_status = 0;
          } else if (strcmp(myArgv[1], "path") == 0) {
            write(STDOUT_FILENO, "path is a shell builtin\n", 25);
            exit_status = 0;
          } else if (strcmp(myArgv[1], "type") == 0) {
            write(STDOUT_FILENO, "type is a shell builtin\n", 25);
            exit_status = 0;
          } else {
            exit_status = typeHelper(myArgv, path, num_path);
          }
        }
      } else {
        int find_exe;
        int findflag = 0;
        struct stat *buffer = malloc(sizeof(struct stat));
        for (find_exe = 0; find_exe < num_path; find_exe++) {
          char cand[128];
          strcpy(cand, path[find_exe]);
          strcat(cand, "/");
          strcat(cand, myArgv[0]);
          
          if (stat(cand, buffer) == 0) {

            findflag = 1;
            char tmpName[128];
            strcpy(tmpName, myArgv[0]);
            myArgv[0] = malloc(128);
            strcpy(myArgv[0], path[find_exe]);
            strcat(myArgv[0], "/");
            strcat(myArgv[0], tmpName);

            int rc = fork(); 
            if (rc == 0) {
              execvp(myArgv[0], myArgv);
              write(STDERR_FILENO, error_message, strlen(error_message));
            } else if (rc > 0) {
              int stat_loc = 0;
              waitpid(rc, &stat_loc, 0);
              //printf("%d\n", stat_loc);
              if (WIFEXITED(stat_loc)) {
                exit_status = WEXITSTATUS(stat_loc);
              } 
              if (WIFSIGNALED(stat_loc)) {
                exit_status = WTERMSIG(stat_loc);
              }

              break;
            } else {
              printf("fork error\n");
            }
          } 
        }
        free(buffer);
        if (findflag == 0) {
          exit_status = 1;
          write(STDERR_FILENO, error_message, strlen(error_message));
        }
      }
    }

    printPrompt(cwd);
    memset(&myArgv[0], 0, sizeof(myArgv));
    
  }
  free(myArgv[0]);
  for (int i = 0; i < num_path; i++) {
    free(path[i]);
  }
  return 0;
}


/*

*/