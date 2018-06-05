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
#define MAX_IP  255

//  Estrutura da tabela de roteamento   //
struct tableEntry  {
    //char *ip;                                                                   // Guarda o endereço de IPv4 do nó.
    uint32_t cost;                                                              // Guarda o custo de atingi-lo.
    uint8_t nextHop;                                                            // Guarda o próximo nó para atingi-lo.
};

//  Estrutura da tabela de roteamento   //
//  deste nó.                           //
struct topTable {
    //struct tableEntry *table[MAX_IP];                                            // Guarda a tabela de roteamento deste nó.
    struct tableEntry table[MAX_IP][MAX_IP];                                    // Guarda a tabela de roteamento deste nó e de seus vizinhos.
    int myID;                                                                   // Guarda id deste nó.
    //char *ipAddr;                                                               // Guarda IP deste nó.
};

//  Funções e procedimentos do protocolo    //
void retrieve_command(char *command, char *ip, uint32_t *cost);                 // Procedimento que recebe os comandos.
//struct tableEntry createTableEntry(uint32_t id, char *ipAddr, uint32_t nextHop, uint32_t cost);// Função que cria novo vetor de um nó.
struct tableEntry createTableEntry(char *ipAddr, uint32_t nextHop, uint32_t cost);// Função que cria novo vetor de um nó.
void updateTables(uint8_t src, uint8_t dst, uint8_t cst);                       // Atualiza uma entrada na tabela.
void disableLink(uint8_t nodeID);                                               // Retira um vizinho.
uint8_t getID(struct tableEntry node);                                          // Função que retorna o ID de um nó.
void initTable(char *myIp);                                                     // Procedimento que inicializa a tabela do nó.


//  Variáveis globais   //
struct topTable mySelf;                                                         // Guarda a tabela de roteamento deste nó.
char *myIP = NULL;                                                              // Guarda o endereço IP deste nó.

int main(int argc, char *argv[])                                                //  ./router <ADDR> <PERIOD> [STARTUP]
{
    //  Verifica quantidade de argumentos   //
    if(argc < 3)    {
        fprintf(stderr, "Utilização:\t./router <ADDR> <PERIOD> [STARTUP]\n");
        return EXIT_FAILURE;
    }
    myIp = malloc(strlen(argv[1] * sizeof(char)));
    strcpy(myIP, argv[1]);

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
    //myAddr.sin_addr.s_addr = inet_addr(argv[1]);                                // Endereço IP do roteador.
    myAddr.sin_addr.s_addr = inet_addr(myIP);                                   // Endereço IP do roteador.

    //  Fixa socket a porta //
    if(bind(myfd, (struct sockaddr *) &myAddr, addrLen) == -1)  {               // Verifica erro em bind.
        perror("bind");
        return EXIT_FAILURE;
    }

    //  Inicia a tabela de roteamento   //
    initTable(myIP);

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

uint8_t getID(char *ip)
{
    char *ipAddr = malloc(strlen(ip) * sizeof(char));
    strcpy(ipAddr, ip);
    int8_t dotcounter = 3;
    while(dotcounter > 0)
    {
        char aux = *(ipAddr)++;
        if(aux == '.')
            dotcounter--;
    }

    return ((uint8_t) atoi(ipAddr));
}

//struct tableEntry createTableEntry(uint8_t id, char *ipAddr, uint8_t nextHop, uint8_t cost)
//struct tableEntry createTableEntry(char *ipAddr, uint8_t nextHop, uint8_t cost)
struct tableEntry createTableEntry(uint8_t nextHop, uint8_t cost)
{
    struct tableEntry new;
    new.cost = cost;
    //new.id = getID(ipAddr);
    new.nextHop = nextHop;
    //new.ipAddr = ipAddr;
    return new;
}

/*
void updateTables(uint8_t src, uint8_t dst, uint8_t cst)
{
    //  Verifica se existe caminho  //
    if(cst == -1)   {
        myTable[src]
}
*/

void disableLink(uint8_t nodeID)
{
    //  Retira vetor do node ID da tabela    //
    //mySelf.table[getID(myIP)][nodeID].cost = INF_COST;
    //mySelf.table[getID(myIP)][nodeID].nextHop = NOLINK;
    mySelf.table[getID(myIP)][nodeID] = createTableEntry(NOLINK, INF_COST);
}

void initTable()
{
    //  Cria entrada do nó  //
    uint8_t myID = getID(myIP);
    //struct tableEntry me = createTableEntry(myID, myIp, myID, 0);
    //struct tableEntry me = createTableEntry(myIP, myID, 0);
    struct tableEntry me = createTableEntry(myID, 0);
    //me.ipAddr = myIp;

    //  Inicializa a tabela //
    for(uint8_t i = 0; i < MAX_IP; i++) {
        for(uint8_t j = 0; j < MAX_IP; j++) {
            //mySelf.table[i][j].ip = NULL;
            mySelf.table[i][j].cost = INF_COST;
            mySelf.table[i][j].nextHop = NOLINK;
        }
    }

    //  Adiciona este nó à tabela    //
    mySelf.table[myID][myID] = me;
}

