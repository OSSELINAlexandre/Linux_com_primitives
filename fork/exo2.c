#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pid_t ret1, ret2, ret3;

int main (){
      
       
       printf("Creating:\n");
       printf("\t-two forked processes from the father.\n");
       printf("\t-one forked process from one of the previously created son.\n\n");

ret1 = fork();
if (ret1 == 0) {
	printf("1st son process  ->\tFather PID=%d.\n", getppid());
	printf("1st son process  PID=%d.\n", getpid());
	
	ret3 = fork();
	if (ret3 == 0) {
	
        printf("Grandson process ->\tFather PID=%d.\n", getppid());
        printf("Grandson process PID=%d.\n", getpid());
	exit(1);
	}else
	{
	
	printf("1st son process  ->\tGrandson PID=%d.\n", ret3);
	wait();
	exit(1);
	}
}
else
{
	printf("Father process   PID=%d.\n", getpid());
	printf("Father process   ->\t1st son PID=%d.\n", ret1);
        ret2 = fork();
        if (ret2 == 0) {
        
	printf("2nd son process  ->\tFather PID=%d.\n", getppid());
	printf("2nd son process  PID=%d.\n", getpid());
	exit(1);
        }else{
        
	printf("Father process   ->\t2nd son PID=%d.\n", ret2);
	wait();
	wait();
	printf("\nFather process   -> All son's processes are dead.\n");  
        }
}
        return 0;
}

