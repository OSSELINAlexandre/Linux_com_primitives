#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pid_t ret;
int count = 0;
int steps = 10;
int i;

int main (){
      
       printf("\n\t\t\t\t\t\t\t[Fork duplication]\n\n");
       printf("This example shows that when a fork is done, it duplicates everything in central memory, including the variable section.\n");
       printf("Even if both processes share the count variable in source code, both instantiation are independent.\n");
       printf("It justifies the 'heavy process duplication' name of the 'fork'.\n\n");
       
       
ret = fork();
if (ret == 0) {

	printf("\nSon process  ->\tFather PID=%d.\n", getppid());
	printf("Son process  PID=%d.\n", getpid());
        printf("Son process  -> count value (should be 0): %d.\n", count);
        for (i = 0; i < steps; i++) {
	count += 1;
        printf("Son process  -> count value: %d\n", count);
        }
        
        printf("Son process  -> final count value (should be 10): %d.\n", count);
        exit(1);
	
}
else
{
	printf("Father process PID=%d.\n", getpid());
	printf("Father process -> 1st son PID=%d.\n", ret);
        printf("Father process -> count value (should be 0): %d.\n", count);
	for (i = 0; i < steps; i++) {
	count += 2;
        printf("Father process -> count value: %d\n", count);
        }
        
        printf("Father process -> final count value (should be 20): %d.\n", count);
        wait();
        printf("\nFather process -> son process is dead.\n");
}
        return 0;
}

