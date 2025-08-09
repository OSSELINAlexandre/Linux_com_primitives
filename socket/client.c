#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(){
int sock = 0;
char Buffer[1024];
struct sockaddr_in sock_serv;

sock = socket(AF_INET, SOCK_STREAM,0);
if (sock == -1){
    printf("[Error] On creation of Client Socket!");
}else
{
    printf("[Success] Client socket creation.\n");
}

sock_serv.sin_family = AF_INET; 
sock_serv.sin_port = htons (7777);

if (inet_pton(AF_INET, "127.0.0.1", &sock_serv.sin_addr) <=0){
    printf("[Error] On conversion for address of server.\n");
}

if (connect(sock, (struct sockaddr*)&sock_serv, sizeof(sock_serv)) <0){
    printf("[Error] On connection to the server socket.\n");
    close(sock);
    return -1;
}

printf("Connected to distant adress on 127.0.0.1:7777\n");

const char* text = "Hello from the RaspBerry Pi!";

send (sock, text, strlen(text), 0);

ssize_t bytesReceived = recv(sock, Buffer, 1023, 0);

if (bytesReceived > 0){

    Buffer[1024] = '/0';

    printf("[Raspberry] Received Msg : %s\n",  Buffer);
}


return 0;
}
