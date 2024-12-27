#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include "definition.h"
#include <string.h>

int  fdFirst, fdSecond;
char userInput[3][10];
char pathToPipe[2][255];
char results[10];
int  userFirst, userSecond;


int main()
{
    printf("\t\t[Client]\n");
    
    strcpy( pathToPipe[0] , getenv("thePath"));
    strcpy( pathToPipe[1] , getenv("thePath"));
    
    strcat(pathToPipe[0], fileA);
    strcat(pathToPipe[1], fileB);
    
    fdFirst = open (pathToPipe[0], O_WRONLY);
    fdSecond = open (pathToPipe[1], O_RDONLY);
    
    printf ("Please enter a first number:\n");
    scanf("%d", &userFirst);       
    printf ("Please enter a second number:\n");
    scanf("%d", &userSecond);	
    sprintf(userInput[0], "%d", userFirst);
    sprintf(userInput[1], "%d", userSecond);
    
	

    write(fdFirst, userInput[0], 10);
    write(fdFirst, userInput[1], 10);
    printf("[Client]: I have passed the values: %s & %s.\n", userInput[0] ,userInput[1]);
    
    read(fdSecond, results,10);
    printf("[Client]: received results: %s \n", results);

    close(fdFirst);
    close(fdSecond);
    
    return 0;
}

