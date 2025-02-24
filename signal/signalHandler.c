#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

pid_t ret;
int count = 0;

void treatAlarm(){
    printf("\n[FatherProcess][treatAlarm]");
    printf("\n[Father Process]: Killing my son, Oedipe was wrong !");
    kill (ret, SIGKILL);
}

void treatUserUnagreablePreventionFromKillingMYSon(){
    printf("\n[FatherProcess][treatUserUnagreablePreventionFromKillingMYSon]");

    if (0 == count)
    {
        printf("\n[FatherProcess]: Please let me be free, I want to kill my son.");
        count++;
    }else if (1 == count)
    {
        printf("\n[FatherProcess]: I already told you that I want to kill my son, let me be.");
        count++;
    }else
    {
        printf("\n[FatherProcess]: Goodness won.");
        kill(ret, SIGUSR1); 
        sleep(1);
        kill(getpid(),SIGKILL);
    }
}

void treatInterruptionForSon(){
    printf("\n[SonProcess][treatInterruptionForSon]");
    printf("\n[SonProcess]: Nothing is happening, Only the father can kill the son !");

}

void treatFatherDeath(){
    printf("\n[SonProcess][treatFatherDeath]");
    printf("\n[SonProcess]: My father is dead, I'm killing myself !");
    kill (getpid(), SIGKILL);

}

void treatSonDeath(){
    printf("\n[FatherProcess][treatSonDeath]");
    printf("\n[FatherProcessProcess]: My son is dead, I'll wait 5 seconds before killing myself.");
    sleep(5);
    kill (getpid(), SIGKILL);

}

void creatingSignalForInter(){
    printf("\n[FatherProcess][creatingSignalForInter]");
    signal(SIGINT, (__sighandler_t) treatInterruptionForSon);
}

void creatingSignalForAlarm () {
    printf("\n[FatherProcess][creatingSignalForAlarm]");
    signal(SIGALRM, (__sighandler_t) treatAlarm);
}

void creatingSignalForPreventingInterruption (){
    printf("\n[FatherProcess][creatingSignalForPreventingInterruption]");
    signal(SIGINT, (__sighandler_t) treatUserUnagreablePreventionFromKillingMYSon);
}

void creatingSignalForFatherDeath(){
    printf("\n[SonProcess][creatingSignalForFatherDeath]");
    signal(SIGUSR1, (__sighandler_t) treatFatherDeath);
}

void creatingSignalForDeathOfSon(){
    printf("\n[FatherProcess][creatingSignalForDeathOfSon]");
    signal(SIGCHLD, (__sighandler_t) treatSonDeath);
}


int main()
{
    printf("\n\t\t[SignalHandler]\n");
    printf("\n[SignalHandler]: Creating a son.\n");
    ret = fork();
 
    if (ret == 0) 
    {
        printf("\n[SonProcess]");
        creatingSignalForFatherDeath();
        creatingSignalForInter();
        while(1){
            sleep(1);
            printf("\n[Son process]: I'm printing endlessly.");
        }
    }else
    {
        printf("\n[FatherProcess]");
        creatingSignalForAlarm();
        creatingSignalForPreventingInterruption();
        creatingSignalForDeathOfSon();
        alarm (5);
        while(1){
            sleep(2);
            printf("\n[Father process]: Waiting for things to change.");
        }
    }
}
