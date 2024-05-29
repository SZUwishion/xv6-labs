#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char* argv[]) 
{
    if(strlen(argv[1]) == 0) {
        fprintf(2, "sleep: missing operand\n");
        exit(1);
    } else {
        sleep(atoi(argv[1]));
        exit(0);
    }
}