#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define bufSize 170

pid_t ret;
char inheritance[70] = "Writing in a file some text before son is created.\n";
char son[100] = "I am the son and writing in the file from my inherited file descriptor.\0\n";
char buffer[bufSize];
int fd;


int main (){
      
       printf("\n\t\t\t\t\t\t[Fork duplication - Fie descriptor case]\n\n");
       printf("This example shows that when a fork is done, if a file is open, both processes write in the same file.\n\n");

	fd = open ("test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
	
	if (fd == -1) {
		printf("\nError in the opening");
	}
	write (fd, inheritance , strlen(inheritance));
       
        ret = fork();

if (ret == 0) {

	printf("\nSon process  ->\tFather PID=%d.\n", getppid());
	printf("Son process  PID=%d.\n", getpid());
	
	write(fd, son, strlen(son));
        exit(1);
	
}
else
{
	printf("Father process PID=%d.\n", getpid());
	printf("Father process -> 1st son PID=%d.\n", ret);
        wait();
        printf("\nFather process -> son process is dead. The file should be written with the son sentence.\n\n");
        
        lseek(fd, 0, SEEK_SET);
        read (fd, buffer, bufSize);
        
        printf("Content of the file:\n\"%s\"\n\n", buffer);
        
        close(fd);
}
        return 0;
}

