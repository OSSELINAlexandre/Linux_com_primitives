#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int aVariable = 5;

int main()
{
    printf("I'm the source code from codeReplacing. I should have the same PID than Father Process :> PID=%d.\n", getpid());
    printf("This is the value of my variable, which shows that my variable section has been loaded in central memory:\n");
    printf("codeReplacing  -> aVariable value: %d.\n", aVariable);
    printf("no other 'SHOULD NOT BE PRINTED' than this one should be printed on the screen.\n");
    return 0;
}

