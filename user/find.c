#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


void find(char* file_name, char *path) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open \n");
        return;
    }

    if(fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (st.type != T_DIR) {
        fprintf(2, "error, the second input should be a directory\n");
        return;
    }

    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        fprintf(2, "find: path too long\n");
        return;
    }

    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)) {
        if(de.inum == 0) {
            continue;
        } else {
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (stat(buf, &st) < 0) {
                fprintf(2, "find: cannot stat \n");
                continue;
            }       
            if(st.type == T_DIR && strcmp(p, ".") != 0 && strcmp(p, "..") != 0) {
                find(file_name,buf);
            } else if(st.type == T_FILE) {
                if(strcmp(file_name, p) == 0) {
                    fprintf(1, "%s\n", buf);
                }
            }
        }
    }

    close(fd);
}

int main(int argc, char *argv[])
{
    find(argv[2],argv[1]);
    exit(0);
}



