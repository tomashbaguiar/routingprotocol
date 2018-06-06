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
#include <pthread.h>

#define PORTA   55151                                                           // Porta padrão dos roteadores.
#define MAX_IP  255
#define IP_STD	"127.0.1."														// "IP" da rede.
#define INF_COST 0xff
#define NOLINK  -1

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
    struct tableEntry table[MAX_IP][MAX_IP];                                    // Guarda a tabela de roteamento deste nó e dos vizinhos.
    int myID;                                                                   // Guarda id deste nó.
    //char *ipAddr;                                                               // Guarda IP deste nó.
};

//  Funções e procedimentos do protocolo    //
void retrieve_command(char *command, char *ip, uint32_t *cost);                 // Procedimento que recebe os comandos.
//struct tableEntry createTableEntry(uint32_t id, char *ipAddr, uint32_t nextHop, uint32_t cost);// Função que cria novo vetor de um nó.
//struct tableEntry createTableEntry(char *ipAddr, uint32_t nextHop, uint32_t cost);// Função que cria novo vetor de um nó.
struct tableEntry createTableEntry(uint8_t nextHop, uint8_t cost);              // Função que cria novo vetor de um nó.
//void updateTables(uint8_t src, uint8_t dst, uint8_t cst);                       // Atualiza uma entrada na tabela.
void disableLink(uint8_t nodeID);                                               // Retira um vizinho.
uint8_t getID(char*);                                                           // Função que retorna o ID de um nó.
void initTable();                                                               // Procedimento que inicializa a tabela do nó.
void addLink(char *nodeIP, uint32_t cost);                                      // Procedimento que adiciona um arco a um nó.

//  Procedimentos thread para recebimento/envio de atualizações na tabela   //
void *ptSendUpdate(void *args);                                                 // Envia pacotes de atualização para os vizinhos.
void *ptRecvData(void*);                                                        // Recebe datagramas.

//  Variáveis globais   //
struct topTable mySelf;                                                         // Guarda a tabela de roteamento deste nó.
char *myIP = NULL;                                                              // Guarda o endereço IP deste nó.
int myfd = 0;                                                                   // Guarda o descritor do socket.
uint8_t neigh[MAX_IP] = {0};                                                    // Vetor que guarda a lista de adjacência.

pthread_mutex_t lock;                                                           // Mutex para acesso à tabela de roteamento.
uint8_t quit = 0;                                                               // Booleano para existência das threads.

int main(int argc, char *argv[])                                                //  ./router <ADDR> <PERIOD> [STARTUP]
{
    //  Verifica quantidade de argumentos   //
    if(argc < 3)    {
        fprintf(stderr, "Utilização:\t./router <ADDR> <PERIOD> [STARTUP]\n");
        return EXIT_FAILURE;
    }
    myIP = malloc(strlen(argv[1]) * sizeof(char));
    strcpy(myIP, argv[1]);
    unsigned int period = (unsigned int) atoi(argv[2]);                         // Recebe o período de atualização da tabela.

    //  Inicializa o socket //
    myfd = socket(AF_INET, SOCK_DGRAM, 0);
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
    myAddr.sin_addr.s_addr = inet_addr(myIP);                                   // Endereço IP do roteador.

    //  Fixa socket a porta //
    if(bind(myfd, (struct sockaddr *) &myAddr, addrLen) == -1)  {               // Verifica erro em bind.
        perror("bind");
        return EXIT_FAILURE;
    }

    //  Inicia a tabela de roteamento   //
    initTable();

    //  Inicializa os mutexes   //
    if(pthread_mutex_init(&lock, NULL) != 0)    {                               // Verifica erro na inicialização.
        perror("init-tmutex");
        return EXIT_FAILURE;
    }

    //  Inicia o loop de recebimento de comandos    //
    pthread_mutex_lock(&lock);                                                  // Bloqueia o mutex antes de iniciar o loop.
    while(1)    {
                
        //  Chama as threads de envio e recebimento //
        pthread_t stid, rtid;                                                   // Variáveis de pthread.
        if(pthread_create(&stid, NULL, &ptSendUpdate, (void *) &period) != 0) { // Verifica erro.
            perror("sUpdate-create");
            return EXIT_FAILURE;
        }
        if(pthread_create(&rtid, NULL, &ptRecvData, NULL) != 0)   {             // Verifica erro.
            perror("sUpdate-create");
            return EXIT_FAILURE;
        }
        
        //  Recebe o comando    //
        char command[6] = {0};
        char ip[15] = {0};
        uint32_t weight = 0;
        retrieve_command(command, ip, &weight);
printf("comando = -%s-%s-%hu.\n", command, ip, weight);

        //  Realiza a operação desejada //
        if(!strcmp("quit", command)) {
            fprintf(stdout, "Saindo...\n");
            quit = 1;                                                           // Sinal para destruir threads.
            if(pthread_join(stid, NULL) != 0)
                perror("join-sender");
            if(pthread_join(rtid, NULL) != 0)
                perror("join-receiver");
            break;
        }
        else if(!strcmp("add", command)) {                                      // Operação de adição de arco.
            //  Cria a estrutura de endereçamento do socket //
            /*
            struct sockaddr_in newAddr;
            newAddr = myAddr;
            newAddr.sin_addr.s_addr = inet_addr(ip);
            */

            //  Coloca o novo nó na tabela de roteamento    //
            addLink(ip, weight);
        }
        else if(!strcmp("del", command))    {                                   // Operação de removação de arco.
            //  Remove o arco da tabela de roteamento //
            disableLink(getID(ip));
        }
        else if(!strcmp("trace", command))  {
            //  Verifica para quem mandar a mensagem    //
            //
            /****** busca na tabela deste nó (mySelf.table[myID][i]) proximo nó para dst ip. */

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
            fprintf(stderr, "\tquit\n");
        }
        pthread_mutex_unlock(&lock);                                            // Desbloqueia o mutex da tabela.
    }

    pthread_mutex_destroy(&lock);                                               // Destrói o mutex.

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

char *getIP(uint8_t id)
{
	char *ip = malloc((strlen(IP_STD) + 4) * sizeof(char));
	strcpy(ip, IP_STD);
	char aux[4] = {0};
	sprintf(aux, "%hu", id);
	strcat(ip, aux);

	return ip;
}

//struct tableEntry createTableEntry(uint8_t id, char *ipAddr, uint8_t nextHop, uint8_t cost)
//struct tableEntry createTableEntry(char *ipAddr, uint8_t nextHop, uint8_t cost)
struct tableEntry createTableEntry(uint8_t nextHop, uint8_t cost)
{
    struct tableEntry new;
    new.cost = cost;
    new.nextHop = nextHop;
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

void addLink(char *nodeIP, uint32_t cost)
{
    pthread_mutex_lock(&lock);                                                  // Bloqueia o mutex antes de escrever.
	mySelf.table[getID(myIP)][getID(nodeIP)] = createTableEntry(getID(nodeIP), cost);
	mySelf.table[getID(nodeIP)][getID(myIP)] = createTableEntry(getID(myIP), cost);
    neigh[getID(nodeIP)] = 1;
    pthread_mutex_unlock(&lock);                                                // Desbloqueia o mutex depois de escrever.
}

void disableLink(uint8_t nodeID)
{
    //  Retira vetor do node ID da tabela    //
    //mySelf.table[getID(myIP)][nodeID].cost = INF_COST;
    //mySelf.table[getID(myIP)][nodeID].nextHop = NOLINK;
    pthread_mutex_lock(&lock);                                                  // Bloqueia o mutex antes de escrever.
    mySelf.table[getID(myIP)][nodeID] = createTableEntry(NOLINK, INF_COST);
    mySelf.table[nodeID][getID(myIP)] = createTableEntry(NOLINK, INF_COST);
    neigh[nodeID] = 0;
    pthread_mutex_unlock(&lock);                                                // Desbloqueia o mutex depois de escrever.
}

void initTable()
{
	//  Inicializa a tabela //
    for(uint8_t i = 0; i < MAX_IP; i++)
        for(uint8_t j = 0; j < MAX_IP; j++)
            mySelf.table[i][j] = createTableEntry(NOLINK, INF_COST);

    //  Cria entrada do nó  //
    uint8_t myID = getID(myIP);
    struct tableEntry me = createTableEntry(myID, 0);

    //  Adiciona este nó à tabela    //
    mySelf.table[myID][myID] = me;
}

void *ptSendUpdate(void *args)
{
    unsigned int period = *((unsigned int *) args);                             // Converte o argumento.

    //  Cria mensagem de update //
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    memset(&addr, 0, addrLen);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORTA);

    struct tableEntry table[MAX_IP];
    while(!quit)    {
        sleep(period);
        pthread_mutex_lock(&lock);
        for(uint8_t i = 0; i < MAX_IP; i++)    {
            if(neigh[i] == 1)   {
                addr.sin_addr.s_addr = inet_addr(getIP(i));
                for(uint8_t j = 0; j < MAX_IP; j++)
                    table[j] = mySelf.table[mySelf.myID][j];


                char msg[] = "update";
                if(sendto(myfd, (char *) msg, strlen(msg) * sizeof(char), 0, (struct sockaddr *) &addr, addrLen) == -1)  {
                    perror("sendto1");
                    pthread_exit(NULL);
                }


                if(sendto(myfd, (struct tableEntry *) table, MAX_IP * sizeof(struct tableEntry), 0, (struct sockaddr *) &addr, addrLen) == -1)  {
                    perror("sendto");
                    pthread_exit(NULL);
                }
            }
        }
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(NULL);
}

void *ptRecvData(void* args)
{
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    while(!quit)    {
        char msg[10] = {0};
        ssize_t recvd = recvfrom(myfd, (char *) msg, 10 * sizeof(char), 0, (struct sockaddr *) &addr, &addrLen);
        if(recvd <= 0)  {
            perror("recvfrom1");
            pthread_exit(NULL);
        }
    
        struct tableEntry table[MAX_IP];
        recvd = recvfrom(myfd, (struct tableEntry *) table, MAX_IP * sizeof(struct tableEntry), 0, (struct sockaddr *) &addr, &addrLen);
        if(recvd <= 0)  {
            perror("recvfrom");
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&lock);

        fprintf(stdout, "Received:[%s] \tfrom %s.\n", msg, inet_ntoa(addr.sin_addr));

        pthread_mutex_unlock(&lock);
    }
    pthread_exit(args);
}

