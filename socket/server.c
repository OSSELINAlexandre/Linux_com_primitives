#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>


int main(){

int sock,sockCli;
struct sockaddr_in sock_serv, sock_client;
char Buffer[1024]; 
socklen_t client_len = sizeof(sock_client);  

sock = socket(AF_INET, SOCK_STREAM,0);
    if (sock == -1){

    printf("[Error] On creation of server socket!\n");

}else
{

printf("[Success]\n");

}


sock_serv.sin_family = AF_INET;
sock_serv.sin_port = htons (7777);
sock_serv.sin_addr.s_addr = INADDR_ANY;

 if (bind(sock, (struct sockaddr*)&sock_serv, sizeof(sock_serv)) < 0){

    printf("[Error] On binding of socket\n");
    close(sock);
}


if (listen(sock, 5) < 0){
    printf("[Error] On listening to the socket.\n");
}

sockCli = accept(sock, (struct sockaddr*)&sock_client, &client_len);

if (sockCli <0){
    printf("[Error] On accept of the Socket from client.\n");
}


ssize_t bytesReceived = recv(sockCli, Buffer, 1023, 0);

if (bytesReceived > 0){

    Buffer[1024] = '/0';
    printf("This is the send buffer : %s\n",  Buffer);
    const char* text =  "Hello from Linux of Chad!";
    send (sockCli, text, strlen(text), 0);
}
    
    close(sockCli);
    close(sock);
    return 0;
}
