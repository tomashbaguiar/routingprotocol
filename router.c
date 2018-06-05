#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <json-c/json.h>

#define PORTA   55151                                                           // Porta padrão dos roteadores.
#define MAX_IP  254

//  Estrutura da tabela de roteamento   //
struct tableEntry  {
    char *ip;                                                                   // Guarda o endereço de IPv4 do nó.
    uint32_t cost;                                                              // Guarda o custo de atingi-lo.
    uint8_t nextHop;                                                            // Guarda o próximo nó para atingi-lo.
};

//  Estrutura da tabela de roteamento
//  deste nó.
struct topTable {
    struct tableEntry *list[MAX_IP];                                            // Guarda a tabela de roteamento deste nó.
    int myID;                                                                   // Guarda id deste nó.
    char *ipAddr;                                                               // Guarda IP deste nó.
};

//  Funções e procedimentos do protocolo
void retrieve_command(char *command, char *ip, uint32_t *cost);                 // Procedimento que recebe os comandos.
struct tableEntry createTableEntry(uint32_t id, char *ipAddr, uint32_t nextHop, uint32_t cost);// Função que cria novo vetor de um nó.


uint8_t getID(struct tableEntry node);                                          // Função que retorna o ID de um nó.


int main(int argc, char *argv[])                                                //  ./router <ADDR> <PERIOD> [STARTUP]
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

    //  Inicia o loop de recebimento de comandos    //
    while(1)    {
        //  Recebe o comando    //
        char command[6] = {0};
        char ip[15] = {0};
        uint32_t weight = 0;
        retrieve_command(command, ip, &weight);

        //  Realiza a operação desejada //
        if(!strcmp("quit", command)) {
            fprintf(stdout, "Saindo...\n");
            break;
        }
        else if(!strcmp("add", command)) {                                      // Operação de adição de vértice.
            //  Cria a estrutura de endereçamento do socket //
            /*
            struct sockaddr_in newAddr;
            newAddr = myAddr;
            newAddr.sin_addr.s_addr = inet_addr(ip);
            */

            //  Coloca o novo nó na tabela de roteamento    //
            //
            //
        }
        else if(!strcmp("del", command))    {                                   // Operação de removação de vértice.
            //  Verifica se o nó existe //
            uint8_t exists = 0;
            //

            //  Remove o nó da tabela de roteamento //
            //
            if(exists)  {
            }
        }
        else if(!strcmp("trace", command))  {
            //  Verifica para quem mandar a mensagem    //
            //
            //

            //  Envia a mensagem    //
            //
            ssize_t recved = 0;
            struct sockaddr_in routerAddr;
            //socklen_t routerLen = sizeof(routerAddr);
            memset(&routerAddr, 0, addrLen);
            //recved = sendto(myfd, (json_object *) jobj, sizeof(json_object), 0, (struct sockaddr *) &routerAddr, routerLen);
            if(recved == -1) {                                                  // Verifica envio da mensagem.
                perror("sendto");
                return EXIT_FAILURE;
            }
        }
        else    {
            fprintf(stderr, "Comando inválido.\n\tUtilização:\n");
            fprintf(stderr, "\tadd <ip> <weight>\n\tdel <ip>\n\ttrace <ip>\n");
            fprintf(stderr, "\texit\n");
        }
    }

    close(myfd);

    return EXIT_SUCCESS;
}

void retrieve_command(char *command, char *ip, uint32_t *weight)
{
    //  Recebe o comando    //
    char *line = NULL;
    size_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    
    //  Recupera os argumentos  //
    if(weight == NULL)
        sscanf(line, "%s %s", command, ip);
    else
        sscanf(line, "%s %s %u", command, ip, weight);
}

uint8_t getID(struct tableEntry node)
{
    int8_t dotcounter = 3;
    while(dotcounter > 0)
    {
        char aux = *(node.ip)++;
        if(aux == '.')
            dotcounter--;
    }

    return ((uint8_t) atoi(ip));
}
