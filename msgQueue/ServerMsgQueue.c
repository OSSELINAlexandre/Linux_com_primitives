#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "definition.h"
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

int queueIDResa, queueIDConsultation;
struct pieceTheater tableInMemory[nbPieces];
pthread_t num_thread, num_thread1, num_thread2 ;
sem_t semaphore;

/*
Semaphore created in order to manage the existence of multiple clients.
*/
void semaphoreInit () {
   sem_init(&semaphore, 0, 1);
}

void dataTableInit(){
   tableInMemory[0] = struct_a;
   tableInMemory[1] = struct_b;
   tableInMemory[2] = struct_c;
   tableInMemory[3] = struct_d;
}

/*
Init of the queue is done in the server, and no sync mecanism has been created. 
Server is to launch before client.
*/
void msgQueueInit(){
   printf("\t\t[Server]:msgQueueInit\n");
   
   queueIDResa = msgget((key_t)idResa, 0666);
   if (queueIDResa != -1) {
      if (msgctl(queueIDResa, IPC_RMID, NULL) == -1) {
         perror("\t\t[Server]:Error on former queue deletion.\n");
         exit(1);
      }
   printf("\t\t[Server]:former queue sucessfully deleted.\n");
   }
    
   queueIDResa = msgget((key_t)idResa, 0750|IPC_CREAT|IPC_EXCL);
    
   if (queueIDResa == -1) {
      perror("\t\t[Server]:msgQueue creation.\n");
      exit(1);
   }

}

void sendPiecesInfo () {
   printf("\t\t[Server]:sendPiecesInfo\n");
   
   struct msgServerConsultation msgServer;
   struct pieceTheater unePiece;
   int i;
	
   for (i = 0; i < nbPieces; i++) {

   unePiece            = tableInMemory[i];
   msgServer.mtype     = idInformationPieces;
   msgServer.IDpiece   = unePiece.id;
   msgServer.nbPlaces  = unePiece.availablePlace;
   strcpy(msgServer.title, unePiece.title);
	      
   if (msgsnd(queueIDResa, &msgServer, sizeof(struct msgServerConsultation) - sizeof(msgServer.mtype), 0) == -1) {
      perror("\t\t[Server][Error]:treatConsultation.\n");
   }
   }
}

void treatSpecificConsultation () {
   printf("\t\t[Server]:treatSpecificConsultation\n");
   
   struct msgClientConsultation messageClient;
   struct msgServerConsultation msgServer;
        
   if (msgrcv(queueIDResa, &messageClient, sizeof(struct msgClientConsultation) - sizeof(messageClient.mtype), idConsultationRequest, 0) == -1) {
      perror("\t\t[Server][Error]:treatSpecificConsultation - réception.\n");
   }
        
   struct pieceTheater showConsulted = tableInMemory[messageClient.IDpiece];
       
   msgServer.mtype       = idConsultationResponse;
   msgServer.IDpiece     = showConsulted.id;
   msgServer.nbPlaces    = showConsulted.availablePlace;
   msgServer.price       = showConsulted.price;
   msgServer.isEventPast = showConsulted.isEventPast;
   strcpy(msgServer.title, showConsulted.title);
   
   if (msgsnd(queueIDResa, &msgServer, sizeof(struct msgServerConsultation) - sizeof(msgServer.mtype), 0) == -1) {
      perror("\t\t[Server][Error]:treatSpecificConsultation - send.\n");
   }

}

void treatSpecificReservation() {
   printf("\t\t[Server]:treatSpecificReservation\n");
      
   struct msgClientReservation messageClient;
   struct msgServeurReservation msgServer;
   int nouvelleCapacite;

   if (msgrcv(queueIDResa, &messageClient, sizeof(struct msgClientReservation) - sizeof(messageClient.mtype), idReservationRequest, 0) == -1) {
      perror("\t\t[Server][Error]:treatSpecificReservation - receipt.\n");

   }
   
   sem_wait(&semaphore);
        
   struct pieceTheater showConsulted = tableInMemory[messageClient.IDpiece];
   msgServer.mtype = idReservationResponse;
   msgServer.IDpiece = showConsulted.id;
   strcpy(msgServer.title, showConsulted.title);
	        
   if ((showConsulted.availablePlace - messageClient.nbPlaces) >= 0) {  
	       
   nouvelleCapacite = showConsulted.availablePlace - messageClient.nbPlaces;
   showConsulted.availablePlace = nouvelleCapacite;
   tableInMemory[messageClient.IDpiece] = showConsulted;
   msgServer.msgPlacesPrises = messageClient.nbPlaces;                       
   msgServer.isError = false;
   }else {
        
   msgServer.isError = true;
	
   char buffer[1024];
   snprintf(buffer, sizeof(buffer), "[Error] for show n°%d, the number of asked places %d is above the number of available pieces %d.\n", messageClient.IDpiece,messageClient.nbPlaces, showConsulted.availablePlace);
   strcpy(msgServer.msgError, buffer);
   }
        
        		
   if (msgsnd(queueIDResa, &msgServer, sizeof(struct msgServeurReservation) - sizeof(msgServer.mtype), 0) == -1) {
      perror("\t\t[Server][Error]:treatSpecificReservation: send reservation.\n");
   }
   
   sem_post(&semaphore);
}

void treatConsultation() {
   printf("\t\t[Server]:Consultation\n");
   
   sendPiecesInfo();
   treatSpecificConsultation();
}

void treatReservation(){
   printf("\t\t[Server]:Reservation\n");
   
   sendPiecesInfo();
   treatSpecificReservation();
}

/*
The management of reservation or consultation is done by two different threads.
Thereofre, the server can continue the reading of the queue whislt a thread computes. 
*/
void manageReception(bool consultation, bool reservation){
   printf("\t\t[Server]:manageReception\n");
   
    if (true == consultation){
        
      if (pthread_create(&num_thread1, NULL, (void *(*)())treatConsultation, NULL) == -1)
      {
         perror ("\t\t[Server][Error]:pthread_create\n");
      }else
      {
         printf ("\t\t[Server]:manageReception launch.\n");
      }
	        
   }else if (true == reservation){
        
      if (pthread_create(&num_thread2, NULL, (void *(*)())treatReservation, NULL) == -1)
      {
         perror ("\t\t[Server][Error]:pthread_create\n");           
      }else
      {
         printf ("\t\t[Server]:manageReception launch.\n");
      }
        
   }
}

/*
Main function is the controller for the server.
*/
int main()
{
   struct msgClientUserChoice request;
   printf("\t\t[Server]\n");
     
   dataTableInit();
   msgQueueInit();
   semaphoreInit();
		
     
   while (1) {
      if (msgrcv(queueIDResa, &request, sizeof(struct msgClientUserChoice) - sizeof(request.mtype), idClientServeurCom, 0) == -1) {
      perror("\t\t[Server][Error] on client receipt.\n");
	}
	
	manageReception (request.tableConsultaiton, request.specificBooking);
        
    }
     
    return 0;
}


