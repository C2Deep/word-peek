#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>

char *set_process_abs_path(void);

char *set_process_abs_path(void)
{

    char path[PATH_MAX];

    ssize_t len = readlink("/proc/self/exe", path, PATH_MAX - 1);

    if(len > 0)
    {
        char *lastSlash = strrchr(path, '/');

        *(lastSlash) = '\0';
        chdir(path);
    }
    else
        return NULL;

}
