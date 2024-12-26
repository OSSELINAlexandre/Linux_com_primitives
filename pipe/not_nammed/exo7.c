#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

pid_t ret1, ret2;
int fileDescriptorAnonymous[2];
char firstMsg[30]  = "\tMessage from the Father.\n";
char secondMsg[30]  = "\tMessage from the Son.\n";
char buffer[75];
int main()
{
    printf("\n\t\t\t\t\t\t[anonymous pipe]: pipe shared by inheritance.\n");
    printf("Two strings are going to be communicated:\n");
    printf("\t-one from the father to a son.\n");
    printf("\t-a second from a son to another son.\n\n");
    
    pipe (fileDescriptorAnonymous);
    ret1 = fork();
    
    
    if (ret1 == 0) 
    {
    	printf("1st son process               PID=%d.\n", getpid());
    	printf("1st son process               -> Father PID=%d.\n", getppid());

        close(fileDescriptorAnonymous[0]);
	write(fileDescriptorAnonymous[1], secondMsg , strlen(secondMsg));
        close(fileDescriptorAnonymous[1]);
        
	exit(0);
	
    }else
    {
	    ret2 = fork ();
	    if (ret2 == 0) {
	    	  
    	            printf("2nd son process               PID=%d.\n", getpid());
    	            printf("2nd son process               -> Father PID=%d.\n", getppid());
    	            
	    	    close(fileDescriptorAnonymous[1]);
		    read(fileDescriptorAnonymous[0], buffer, 75);
		    printf("Communicated messages:\n%s\n", buffer);
            	    close(fileDescriptorAnonymous[0]);
		    exit(0);
	    }else
	    {
    	            printf("Father process                PID=%d.\n", getpid());
    	            printf("[Father]:1st son process      -> PID=%d.\n", ret1);
    	            printf("[Father]:2nd son process      -> PID=%d.\n", ret2);
    	            
    	            close(fileDescriptorAnonymous[0]);
    	            write(fileDescriptorAnonymous[1], firstMsg , strlen(firstMsg));
            	    close(fileDescriptorAnonymous[1]);
            	    
		    wait();
		    printf("1st Son process is done.\n");
		    wait();
		    printf("2nd Son process is done.\n");
	    }
    }
    
    return 0;
}

