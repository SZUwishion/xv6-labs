#include <kernel/types.h>
#include <user/user.h>

void transfer_primes(int lpipe[2], int rpipe[2], int lfirst)
{
    int num;
    while(read(lpipe[0], &num, sizeof(int)) == sizeof(int)) {
        if(num % lfirst != 0) {
            write(rpipe[1], &num, sizeof(int));
        }
    }
    close(lpipe[0]);
    close(rpipe[1]);
}

void primes(int lpipe[2])
{
    close(lpipe[1]);
    int lfirst;
    if(read(lpipe[0], &lfirst, sizeof(int)) != sizeof(int)) {
        exit(1);
    } else {
        int p[2];
        pipe(p);
        fprintf(1, "prime %d\n", lfirst);
        transfer_primes(lpipe, p, lfirst);
        primes(p);
    }
}

int main(int argc, char *argv[])
{
    int pid = fork();
    int lp[2];
    pipe(lp);

    for(int i = 2; i <= 35; i++) {
        if(write(lp[1], &i, sizeof(int)) != sizeof(int)) {
            fprintf(2, "initialize write error\n");
            exit(1);
        }
    }
    
    if(pid < 0) {
        fprintf(2, "fork error\n");
        exit(1);
    } else if(pid == 0) {
        primes(lp);
    } else {
        wait(0);
        exit(0);
    }
    exit(0);
}