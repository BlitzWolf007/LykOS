#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    printf("Init");
    fflush(stdout);

    pid_t pid = fork();

    printf("hi from %d", pid);
    fflush(stdout);

    if (pid == 0)
    {
        execl("/bin/console", "console", NULL);
        perror("exec");
        return 1;
    }

    while (true)
        ;

    return 0;
}
