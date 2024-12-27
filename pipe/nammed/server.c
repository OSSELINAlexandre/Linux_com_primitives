#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include "definition.h"
#include <string.h>


int  fdFirst, fdSecond;
int  firstInput, secondInput;
char clientInput[2][10];
char serverOuput[10];
char pathToPipe[2][255];

int main()
{
    printf("\t\t[Server]\n");
	

    strcpy( pathToPipe[0] , getenv("thePath"));
    strcpy( pathToPipe[1] , getenv("thePath"));
    
    strcat(pathToPipe[0], fileA);
    strcat(pathToPipe[1], fileB);
    
    mode_t theMode = S_IRWXU;
    
    mkfifo (pathToPipe[0], theMode);
    mkfifo (pathToPipe[1], theMode);
    
    fdFirst  = open (pathToPipe[0], O_RDONLY);
    fdSecond = open (pathToPipe[1], O_WRONLY);
    printf("[Server] Wainting for values.\n");
    read(fdFirst, clientInput[0], 10);
    read(fdFirst, clientInput[1], 10);
    printf("[Server] values received: %s & %s.\n", clientInput[0] , clientInput[1]);
    
    firstInput = atoi(clientInput[0]);
    secondInput = atoi(clientInput[1]);
    
    sprintf(serverOuput, "%d", (firstInput * secondInput));
    printf("[Server] value to send: %s.\n", serverOuput);
    
    write(fdSecond, serverOuput, 10);
    
    close(fdFirst);
    close(fdSecond);
    
    return 0;
}

