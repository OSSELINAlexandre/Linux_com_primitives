#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    printf("Using fork and printing PID of father and son.\n");
    
    pid_t ret;
    ret = fork();
 
    if (ret == 0) 
    {
    printf("Son's process : Father PID: %d.\n", getppid());
    printf("Son's process : Son PID: %d.\n", getpid());
    exit(1);
    }else
    {
    printf("Father's process : Father PID: %d.\n", getpid());
    printf("Father's process : Son PID: %d.\n", ret); 
    wait();
    }
    
    return 0;
}

