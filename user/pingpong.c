#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p1[2], p2[2];
    pipe(p1);
    pipe(p2);
    int pid = fork();
    char a = 'a';
    int exit_status = 0;
    if(pid == 0) {
        close(p1[1]);
        close(p2[0]);


        if(read(p1[0], &a, sizeof(char)) != sizeof(char)) {
            fprintf(2, "%d child read error\n", getpid());
            exit_status = 1;
        } else {
            fprintf(1, "%d: received ping\n", getpid());
        }

        if(write(p2[1], &a, sizeof(char)) != sizeof(char)) {
            fprintf(2, "%d child write error\n", getpid());
            exit_status = 1;
        }
        close(p1[0]);
        close(p2[1]);
        exit(exit_status);
        
    } else if (pid > 0) {
        close(p1[0]);
        close(p2[1]);
        
        if(write(p1[1], &a, sizeof(char)) != sizeof(char)) {
            fprintf(2, "%d father write error\n", getpid());
            exit_status = 1;
        }
        wait(0);
        if(read(p2[0], &a, sizeof(char)) != sizeof(char)) {
            fprintf(2, "%d father read error\n", getpid());
            exit_status = 1;
        } else {
            fprintf(1, "%d: received pong\n", getpid());
        }

        close(p1[1]);
        close(p2[0]);
        exit(exit_status);
    } else {
        fprintf(2, "fork error!\n");
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        exit(1);
    }
}