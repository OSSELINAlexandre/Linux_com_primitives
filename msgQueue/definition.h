#include <stdbool.h>
#ifndef DEF_H_ 
#define DEF_H_
#define nbPieces               4
#define idReservationRequest   50
#define idReservationResponse  51
#define idConsultationRequest  60
#define idConsultationResponse 61
#define idInformationPieces    70
#define idClientServeurCom     80
#define idResa                 150

struct msgClientUserChoice {
    long mtype;
    bool specificConsultation;
    bool specificBooking;
    bool tableConsultaiton;
    bool updateTable;
};

struct msgClientConsultation {
    long mtype;
    int IDpiece;
};

struct msgClientReservation {
    long mtype;
    int  IDpiece;
    int  nbPlaces;
};

struct msgServerConsultation {
    long mtype;
    int  IDpiece;
    char title[100];
    int  nbPlaces;
    int  price;
    bool isEventPast;
};

struct msgServeurReservation
{
long mtype;
int  IDpiece;
char title[100];
int  msgPlacesPrises;
bool isError;
char msgError[1024];
};

struct pieceTheater
{
int  id;
char title[100];
int  initialCapacity;
int  availablePlace;
int  price;
bool isEventPast;
};

const struct pieceTheater struct_a = { .id = 0, .title = "Demons, Dostoïevski.", .initialCapacity = 100, .availablePlace = 100, .price = 20, .isEventPast = false};

const struct pieceTheater struct_b = { .id = 1, .title = "Achille, Homère.", .initialCapacity = 50, .availablePlace = 50, .price = 30, .isEventPast = false};

const struct pieceTheater struct_c = { .id = 2, .title = "The new testament according to John, John.", .initialCapacity = 75, .availablePlace = 75, .price = 1, .isEventPast = true};

const struct pieceTheater struct_d = { .id = 3, .title = "Inferno, Dantes.", .initialCapacity = 77, .availablePlace = 77, .price = 10, .isEventPast = true};

#endif 

