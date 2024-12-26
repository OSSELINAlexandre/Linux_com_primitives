#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

pid_t ret1;
int  userFirst, userSecond;
int  pipeForInput[2], pipeForOutput[2];
char resultString[4][10];
char inputString [2][10];

int main()
{
    printf("\n\t\t\t\t\t\t[anonymous pipe]: pipe shared by inheritance.\n");
    printf("An operator input is going to be asked:\n");
    printf("\t-the father is going to send both inputs.\n");
    printf("\t-the son is going to do the addition.\n");
    printf("\t-the father will print the ouput.\n\n");
    
    pipe (pipeForInput);
    pipe (pipeForOutput);
    ret1 = fork();
 
    if (ret1 == 0) 
    {
    	printf("Son process\t\tPID=%d.\n", getpid());
    	printf("Son process\t\t-> Father PID=%d.\n\n", getppid());
	
	close(pipeForInput[1]);
	close(pipeForOutput[0]);
	
	read(pipeForInput[0], resultString[0], 10);
	read(pipeForInput[0], resultString[1], 10);
	int result = atoi(resultString[0]) + atoi (resultString[1]);
	sprintf(resultString[2], "%d", result);
	
	write(pipeForOutput[1], resultString[2], 10);
	
	printf("Son process, done reading and writing.\n");
        close(pipeForInput[0]);
        close(pipeForOutput[1]);
	exit(0);
	
    }else
    {
	printf("Father process\t\tPID=%d.\n", getpid());
	printf("[Father]:Son process\t-> PID=%d.\n", ret1);
	
	sleep(1);
        printf ("Please enter a first number:\n");
        scanf("%d", &userFirst);       
        printf ("Please enter a second number:\n");
        scanf("%d", &userSecond);	
	
	close(pipeForInput[0]);
	close(pipeForOutput[1]);
	
	sprintf(inputString[0], "%d", userFirst);
	sprintf(inputString[1], "%d", userSecond);

	write(pipeForInput[1], inputString[0],10);
	write(pipeForInput[1], inputString[1],10);
	
	read(pipeForOutput[0],  resultString[3],10);
	wait();
	printf("[Father] Son process is done.\n");
	printf("Output operation => : %s\n", resultString[3]);   	    
    }
    
    return 0;
}

