#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

pid_t ret;
int count = 0;

void treatAlarm(){
    printf("\n[FatherProcess][treatAlarm]");
    printf("\n[Father Process]: Killing the son, => Oedipe went wrong !");

    printf("Hey ! ret=%d , pid=%d ,", ret, getpid());
    kill (SIGKILL, ret);
}


void treatSonDeath(){
    printf("\n[FatherProcess][treatSonDeath]");
    printf("\n[Father Process]: The son is dead, I could have waited through the wait primitive.");
}

void treatUserUnagreablePreventionFromKillingMYSon(){
    printf("\n[FatherProcess][treatUserUnagreablePreventionFromKillingMYSon]");

    if (0 == count)
    {
        printf("\n[FatherProcess] : Please let me be free, I want to kill my son.");
        count++;
    }else if (1 == count)
    {
        printf("\n[FatherProcess] : I already told you that I want to kill my son, let me be.");
        count++;
    }else
    {
        printf("\n[FatherProcess] : Goodness won.");
        kill (SIGKILL, getpid());
    }
}

void treatInterruptionForSon(){
    printf("\n[SonProcess][treatInterruptionForSon]");
    printf("\n[SonProcess] Nothing is happening, Only the father can kill the son !");

}

void treatFatherDeath(){
    printf("\n[SonProcess][treatFatherDeath]");
    printf("\n[SonProcess] My father is dead, I'm killing myself !");
    kill (SIGKILL, getpid());

}

void creatingStructForInter(){
    signal(SIGINT, (__sighandler_t) treatInterruptionForSon);
}

void creatingStructForAlarm () {

    printf("\n[FatherProcess][creatingStructForAlarm]");
    signal(SIGALRM, (__sighandler_t) treatAlarm);

}

void creatingStructForKillOfSon(){

    printf("\n[FatherProcess][creatingStructForKillOfSon]");
    signal(SIGCHLD, (__sighandler_t) treatSonDeath);

}

void creatingStructForPreventingInterruption (){

    printf("\n[FatherProcess][creatingStructForPreventingInterruption]");
    signal(SIGINT, (__sighandler_t) treatUserUnagreablePreventionFromKillingMYSon);

}

void creatingStructForFatherDeath(){

    signal(SIGKILL, (__sighandler_t) treatFatherDeath);

}

int main()
{
    printf("\n[SignalHandler] : Creating a son.\n");
    ret = fork();
 
    if (ret == 0) 
    {
        creatingStructForInter();
        creatingStructForFatherDeath();
        while(1){
            sleep(1);
            printf("\n[Son process], i'm printing endlessly.");
        }
    }else
    {
        printf("\n[FatherProcess]");
        creatingStructForAlarm();
        creatingStructForKillOfSon();
        creatingStructForPreventingInterruption();
        alarm (15);
        while(1){
            sleep(2);
            printf("\n[Father process], Waiting for things to change.");
        }
    }
}
