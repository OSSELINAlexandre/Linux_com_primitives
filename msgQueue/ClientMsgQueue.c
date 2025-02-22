#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "definition.h"

int choiceOfService, showChoice, userChoice, numberOfPlaces;
int showChoiceFunc, numberOfPlacesFunc;
int queueIDResa;

int secondClientChoice(){
	
   int i;
   printf ("Enter a theater show n° to see its informations:\n");
    
   for (i = 0; i < nbPieces; i++) {
    
   struct msgServerConsultation reponseServeur;
    
   if (msgrcv(queueIDResa, &reponseServeur, sizeof(struct msgServerConsultation) - sizeof(reponseServeur.mtype), idInformationPieces, 0) == -1) {
      perror("[Client][Error]:secondClientChoice.\n");
   }
      printf ("\t- show n°%d, title: %s, available place(s): %d.\n", reponseServeur.IDpiece, reponseServeur.title, reponseServeur.nbPlaces);

   }
	
   printf ("Your choice : ");
   scanf("%d", &showChoiceFunc);
       
   return showChoiceFunc;
}

int demandeClientNombreDePlaces(){

   printf ("\nHow many places do you want to buy : ");
   scanf("%d", &numberOfPlacesFunc);

   return numberOfPlacesFunc;
}

void sendClientChoice(int choiceClient){

   struct msgClientUserChoice msgClient;
	
   msgClient.mtype = idClientServeurCom;

   if (1 == choiceClient){

      msgClient.specificConsultation = false;
      msgClient.specificBooking  = true;
      msgClient.tableConsultaiton = false;
      msgClient.updateTable = false;
      
   }else
   {
      msgClient.specificConsultation = false;
      msgClient.specificBooking = false;
      msgClient.tableConsultaiton  = true;
      msgClient.updateTable = false;

   }
	
   if (msgsnd(queueIDResa, &msgClient, sizeof(struct msgClientUserChoice) - sizeof(msgClient.mtype), 0) == -1) {
      perror("[Client][Error]:sendClientChoice.\n");
   }
}

void sendConsultation (int choice){

   struct msgClientConsultation msgClient;
   struct msgServerConsultation msgServeur;
   
   msgClient.mtype = idConsultationRequest;
   msgClient.IDpiece = choice;
   
   if (msgsnd(queueIDResa, &msgClient, sizeof(struct msgClientConsultation) - sizeof(msgClient.mtype), 0) == -1) {
      perror("[Client][Error]:sendConsultation - Client.\n");
   }

   if (msgrcv(queueIDResa, &msgServeur, sizeof(struct msgServerConsultation) - sizeof(msgServeur.mtype), idConsultationResponse, 0) == -1) {
      perror("[Client][Error]:sendConsultation - Serveur.\n");
}
   printf("Info for show n°%d:\n\t- title: %s,\n\t- available places: %d,\n\t- price per place: %d,\n\t- is event passed: %s.\n", msgServeur.IDpiece , msgServeur.title, msgServeur.nbPlaces, msgServeur.price, msgServeur.isEventPast ? "true" : "false" );
	
}     
    
void sendReservation (int choice, int resa){

   struct msgClientReservation msgClient;
   struct msgServeurReservation msgServeur;
   
   msgClient.mtype    = idReservationRequest;
   msgClient.IDpiece  = choice;
   msgClient.nbPlaces = resa;
    
   if (msgsnd(queueIDResa, &msgClient, sizeof(struct msgClientReservation) - sizeof(msgClient.mtype), 0) == -1) {
      perror("[Error]:sendReservation - Client.\n");
   }
   
   if (msgrcv(queueIDResa, &msgServeur, sizeof(struct msgServeurReservation) - sizeof(msgServeur.mtype), idReservationResponse, 0) == -1) {
      perror("[Error]:sendReservation - Server.\n");
   }
   
   if (false == msgServeur.isError){
		
      printf("Booking for n°%d accepeted, booked places: %d.\n", msgServeur.IDpiece , msgServeur.msgPlacesPrises);
      
   }else{
   
      printf("%s", msgServeur.msgError);
   
   }
}

void treatConsultation (int choiceOfService){

   sendClientChoice(choiceOfService);    
   showChoice = secondClientChoice();
   if (showChoice >= 0 && showChoice < nbPieces){
      sendConsultation(showChoice);
   }else{
      printf ("[Client][Error] - Please enter a valid show ID, %d doesn't belong to the provided list.\n", showChoice);
   }
         
}

void treatReservation (int choiceOfService){

   sendClientChoice(choiceOfService);
   showChoice = secondClientChoice();
   if (showChoice >= 0 && showChoice < nbPieces){
      numberOfPlaces = demandeClientNombreDePlaces();
      sendReservation(showChoice, numberOfPlaces);
   }else{
      printf ("[Client][Error] - Please enter a valid show ID.\n",showChoice);
   }
}

int introMessageAndFirstUserChoice (){

   printf("\n\n======================== [Theater Show Booking System] ========================\n\n");
   
   printf ("Please choose:\n\t- '0' if you want to see information related to theater shows,\n\t- '1' if you want to book a show.\nYour choice :");
   
   scanf("%d", &choiceOfService);
      
   return choiceOfService;
}

int main ()
{
   printf("\t\t\t\t[Client]\n");
   queueIDResa = msgget(idResa, 0750);
	
   while(true){
      
      userChoice = introMessageAndFirstUserChoice();
    
      if (0 == userChoice){

            treatConsultation(choiceOfService);  
                 
      }else if (1 == userChoice)
      {
            treatReservation(choiceOfService);
         
      }else
      {
            printf ("[Error] Please choose either '0' or '1'.\n");
      }
   }
    
    return 0;
}

