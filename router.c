#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <json-c/json.h>

#define PORTA   55151                                                           // Porta padrão dos roteadores.

/*
//  Estrutura que guarda as
//  informações de um nó.
struct node {
    struct sockaddr_in addr;                                                    // Guarda o endereço do nó.
    int fd;                                                                     // Guarda o descritor do socket do nó.
*/


/*********************************************************************************/
char msg[] = "{\"type\": \"data\", \"source\": \"127.0.1.1\", \"destination\": \"127.0.0.1\", \"payload\": \"{\"destination\": \"127.0.0.1\"}\"}";
/*********************************************************************************/


//  ./router <ADDR> <PERIOD> [STARTUP]

int main(int argc, char *argv[])
{
    //  Verifica quantidade de argumentos   //
    if(argc < 3)    {
        fprintf(stderr, "Utilização:\t./router <ADDR> <PERIOD> [STARTUP]\n");
        return EXIT_FAILURE;
    }

    //  Inicializa o socket //
    int myfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(myfd == -1)    {                                                         // Verifica erros na criação do socket.
        perror("socket");
        return EXIT_FAILURE;
    }

    //  Endereça o socket   //
    struct sockaddr_in myAddr;
    socklen_t addrLen = sizeof(myAddr);
    memset(&myAddr, 0, addrLen);
    myAddr.sin_family = AF_INET;                                                // Socket IPv4.
    myAddr.sin_port = htons(PORTA);                                             // Escuta na PORTA.
    myAddr.sin_addr.s_addr = inet_addr(argv[1]);                                // Endereço IP do roteador.

    //  Fixa socket a porta //
    if(bind(myfd, (struct sockaddr *) &myAddr, addrLen) == -1)  {               // Verifica erro em bind.
        perror("bind");
        return EXIT_FAILURE;
    }



/********************************************* teste de configuração ***********************************************/
json_object *jobj = json_tokener_parse(msg);
json_parse(jobj);

ssize_t recved = 0;
struct sockaddr_in routerAddr;
socklen_t routerLen = sizeof(routerAddr);
memset(&routerAddr, 0, addrLen);
if(!strcmp("0",argv[2]))    {
    char *test = malloc(1024 * sizeof(char));
    recved = recvfrom(myfd, (char *) test, (1024 * sizeof(char)), 0, (struct sockaddr *) &routerAddr, &routerLen);
    if(recved <= 0) {
        perror("recvfrom");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "[Received]:\t%s\tfrom\t[%s]\t%ld bytes\n", test, inet_ntoa(routerAddr.sin_addr), recved);
    free(test);
    test = NULL;
}
else    {
    routerAddr = myAddr;
    routerAddr.sin_addr.s_addr = inet_addr(argv[3]);
    //recved = sendto(myfd, (char *) msg, strlen(msg), 0, (struct sockaddr *) &routerAddr, routerLen);
    recved = sendto(myfd, (json_object *) jobj, sizeof(json_object), 0, (struct sockaddr *) &routerAddr, routerLen);
    if(recved == -1) {
        perror("sendto");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "[Sent]:\t%s\tto\t[%s]\t%ld bytes\n", msg, inet_ntoa(routerAddr.sin_addr), recved);
}
/*******************************************************************************************************************/

    return EXIT_SUCCESS;
}
