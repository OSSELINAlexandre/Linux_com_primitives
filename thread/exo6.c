#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct ThreadArgs{
    int a;
    int b;
};

int userFirst, userSecond, userChoice, userResult, retValue;
pthread_t num_thread;
struct ThreadArgs myStruct;

void addition()
{
        printf ("Addition function: N°thread: %ld, PID=%d.\n", num_thread, getpid());
        userResult = userFirst + userSecond;
        printf ("[Thread addition] result=%d.\n", userResult);
}


void multiplication(void *aStruct)
{
        printf ("Multiplication function: N°thread: %ld, PID=%d.\n", num_thread, getpid());
        struct ThreadArgs *funcStruct;
        funcStruct = (struct ThreadArgs *) aStruct;
        userResult = funcStruct->a * funcStruct->b;
        printf ("[Thread multiplication] result=%d.\n", userResult);
}

int main()
{

       printf("\n\t\t\t\t\t\t[Threads]: Light process in Linux\n\n");
       printf("Threads are considered 'light processes': the code and data segments aren't dupliacted as in the fork.\n");
       printf("Instead, registers (and the Ordinal Counter) and the stack are saved for a given thread.\n");
       printf("Therefore, PID and data are the same as the one from the Father.\n\n");
       
       //The purpose of the code isn't to securize the data input from user mistakes.
       //It is solely to understand how and why threads are lights processes.
       
       printf ("Please enter a first number:\n");
       scanf("%d", &userFirst);
        
        
       printf ("Please enter a second number:\n");
       scanf("%d", &userSecond);

       printf ("Which operation do you want? 0: addition | 1: multiplication.\n");
       scanf("%d", &userChoice);

       if (userChoice == 0)  {
              if (pthread_create(&num_thread, NULL, (void *(*)())addition, NULL) == -1)
              {
                     perror ("pb pthread_create\n");
                     
              }else
              {
                     printf ("Father N°thread: %ld, PID=%d.\n", num_thread, getpid());
              }
       }
       else
       {
	//Transforming the input passing a struct by argument.
	myStruct.a = userFirst;
	myStruct.b = userSecond;
	
              if (pthread_create(&num_thread, NULL, (void *(*)())multiplication, (void *) &myStruct) == -1)
              {
                     perror ("pb pthread_create\n");
                     
              }else
              {
                     printf ("Father N°thread: %ld, PID=%d.\n", num_thread, getpid());
              }
       }
       
       pthread_join(num_thread, NULL);
       
       return 0;
}
