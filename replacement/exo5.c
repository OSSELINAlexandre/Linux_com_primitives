#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

pid_t ret;
int aVariable = 9;

int main()
{
    printf("Using replacement function primitive in C code.\n\n");
    
    char *argv[3];
    argv[0] = "ps";
    argv[1] = "-a";
    argv[2] = NULL;

    ret = fork ();
 
    if (ret == 0) 
    {
    printf("Son process    -> Father PID=%d.\n", getppid());
    printf("Son process    PID=%d.\n", getpid());
    aVariable+=1;
    printf("Son process    -> access to the variable aVariable: %d.\n", aVariable);
    execvp("ps", argv);
    printf("SHOULD NOT BE PRINTED => Code has been replaced.\n");
    exit(1);
    }else
    {
    printf("Father process PID=%d.\n", getpid());
    printf("Father process -> access to the variable aVariable: %d.\n", aVariable);
    execl ("codeReplacing.exe", "codeReplacing.exe", NULL, NULL);
    printf("SHOULD NOT BE PRINTED => Code has been replaced.\n");
    wait();
    }
    
    return 0;
}

