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
#include <sys/select.h>

#define PORTA   55151                                                           // Porta padrão dos roteadores.
#define MAX_IP  255
#define IP_STD	"127.0.1."														// "IP" da rede.
#define INF_COST 0xfe
#define NOLINK   257

//  Estrutura da tabela de roteamento   //
struct tableEntry  {
    //char *ip;                                                                   // Guarda o endereço de IPv4 do nó.
    uint8_t cost;                                                              // Guarda o custo de atingi-lo.
    uint16_t nextHop;                                                            // Guarda o próximo nó para atingi-lo.
};

//  Estrutura da tabela de roteamento   //
//  deste nó.                           //
struct topTable {
    //struct tableEntry *table[MAX_IP];                                            // Guarda a tabela de roteamento deste nó.
    struct tableEntry table[MAX_IP][MAX_IP];                                    // Guarda a tabela de roteamento deste nó e dos vizinhos.
    uint16_t myID;                                                               // Guarda id deste nó.
    //char *ipAddr;                                                               // Guarda IP deste nó.
};

//  Funções e procedimentos do protocolo    //
void retrieve_command(char *command, char *ip, uint8_t *cost);                  // Procedimento que recebe os comandos.
struct tableEntry createTableEntry(uint16_t nextHop, uint8_t cost);              // Função que cria novo vetor de um nó.
//void updateTables(uint8_t src, uint8_t dst, uint8_t cst);                       // Atualiza uma entrada na tabela.
void disableLink(uint16_t nodeID);                                               // Retira um vizinho.
uint16_t getID(char*);                                                           // Função que retorna o ID de um nó.
void initTable();                                                               // Procedimento que inicializa a tabela do nó.
void addLink(char *nodeIP, uint8_t cost);                                       // Procedimento que adiciona um arco a um nó.
void printTable(struct tableEntry *table);                                      // Imprime tabela de roteamento.
ssize_t sendDatagram(char *type, uint16_t nodeID, char *ip, struct tableEntry *table);// Função que envia um datagrama.
void updateTable(struct tableEntry *table, uint16_t id);                   // Atualiza a tabela deste nó.
void updateMySelfTable();

//  Procedimento thread para envio de atualizações da tabela   //
void *ptSendUpdate(void *args);                                                 // Envia pacotes de atualização para os vizinhos.

//  Variáveis globais   //
struct topTable mySelf;                                                         // Guarda a tabela de roteamento deste nó.
char *myIP = NULL;                                                              // Guarda o endereço IP deste nó.
int myfd = 0;                                                                   // Guarda o descritor do socket.
uint8_t neigh[MAX_IP] = {0};                                                    // Vetor que guarda a lista de adjacência.

uint8_t quit = 0;                                                               // Booleano para existência da thread.

pthread_mutex_t lock;

int main(int argc, char *argv[])                                                //  ./router <ADDR> <PERIOD> [STARTUP]
{
    //  Verifica quantidade de argumentos   //
    if(argc < 3)    {
        fprintf(stderr, "Utilização:\t./router <ADDR> <PERIOD> [STARTUP]\n");
        return EXIT_FAILURE;
    }
    myIP = malloc(strlen(argv[1]) * sizeof(char));
    strcpy(myIP, argv[1]);
    mySelf.myID = getID(myIP);
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
    
    //  Chama a thread de envio //
    pthread_t stid;                                                 // Variáveis de pthread.
    if(pthread_create(&stid, NULL, &ptSendUpdate, (void *) &period) != 0) { // Verifica erro.
        perror("sUpdate-create");
        return EXIT_FAILURE;
    }

    //  Inicia o loop de recebimento de comandos    //
    pthread_mutex_init(&lock, NULL);
    fd_set rfds;
    //int maxfd = myfd;
    struct tableEntry table[MAX_IP];
    while(1)    {       
        //  Chama select para verificar entrada de dados    //
        FD_ZERO(&rfds);
        FD_SET(myfd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        if(select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0) {
            perror("select");
            quit = 1;                                                           // Sinal para destruir threads.
            if(pthread_join(stid, NULL) != 0)
                perror("join-sender");
            return EXIT_FAILURE;
        }
        
        if(FD_ISSET(STDIN_FILENO, &rfds))    {     
            //  Recebe o comando    //
            char command[6] = {0};
            char ip[15] = {0};
            uint8_t weight = 0;
            retrieve_command(command, ip, &weight);

            //  Realiza a operação desejada //
            if(!strcmp("quit", command)) {
                fprintf(stdout, "Saindo...\n");
                quit = 1;                                                           // Sinal para destruir threads.
                if(pthread_join(stid, NULL) != 0)
                    perror("join-sender");
                break;
            }
            else if(!strcmp("add", command)) {                                      // Operação de adição de arco.
                //  Coloca o novo nó na tabela de roteamento    //
                addLink(ip, weight);
printTable(mySelf.table[mySelf.myID]);
            }
            else if(!strcmp("del", command))    {                                   // Operação de removação de arco.
                //  Remove o arco da tabela de roteamento //
                disableLink(getID(ip));
            }
            else if(!strcmp("trace", command))  {
                //  Verifica para quem mandar a mensagem    //
                uint16_t idSend = mySelf.table[mySelf.myID][getID(ip)].nextHop;

                //  Envia a mensagem    //
                //recved = sendto(myfd, (json_object *) jobj, sizeof(json_object), 0, (struct sockaddr *) &routerAddr, routerLen);
                if(sendDatagram(command, idSend, ip, NULL) == -1) {
                    perror("sendto");
                    return EXIT_FAILURE;
                }
            }
            else if(!strcmp("table", command))
                printTable(mySelf.table[mySelf.myID]);
            else    {
                fprintf(stderr, "Comando inválido.\n\tUtilização:\n");
                fprintf(stderr, "\tadd <ip> <weight>\n\tdel <ip>\n\ttrace <ip>\n");
                fprintf(stderr, "\tquit\n");
            }
        }
        else if(FD_ISSET(myfd, &rfds))   {
            struct sockaddr_in addr;
            socklen_t addrLen = sizeof(addr);
                    
            memset(table, 0, MAX_IP * sizeof(struct tableEntry));
            ssize_t recvd = recvfrom(myfd, (struct tableEntry *) table, MAX_IP * sizeof(struct tableEntry), 0, (struct sockaddr *) &addr, &addrLen);
            if(recvd == -1)  {
                perror("recvfrom");
                return EXIT_FAILURE;
            }
            else if(recvd == 0) {
                fprintf(stderr, "Nó %s fechou-se.\n", inet_ntoa(addr.sin_addr));
            }
            else if(neigh[getID(inet_ntoa(addr.sin_addr))])    {
                updateTable(table, getID(inet_ntoa(addr.sin_addr)));
                fprintf(stdout, "Received:[update] \tfrom %s.\n", inet_ntoa(addr.sin_addr));
                for(uint8_t i = 0; i < MAX_IP; i++)
                    mySelf.table[getID(inet_ntoa(addr.sin_addr))][i] = table[i];
                //printTable(table);
                updateMySelfTable();
            }
        }
    }
    pthread_mutex_destroy(&lock);

    close(myfd);

    return EXIT_SUCCESS;
}

void retrieve_command(char *command, char *ip, uint8_t *weight)
{
    //  Recebe o comando    //
    char *line = NULL;
    size_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    
    //  Recupera os argumentos  //
    sscanf(line, "%s %s %hhu", command, ip, weight);
}

uint16_t getID(char *ip)
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

char *getIP(uint16_t id)
{
	char *ip = malloc((strlen(IP_STD) + 4) * sizeof(char));
	strcpy(ip, IP_STD);
	char aux[4] = {0};
	sprintf(aux, "%hu", id);
	strcat(ip, aux);

	return ip;
}

struct tableEntry createTableEntry(uint16_t nextHop, uint8_t cost)
{
    struct tableEntry new;
    new.cost = cost;
    new.nextHop = nextHop;
    return new;
}

void updateEntry(uint8_t dst, uint16_t nextHop, uint8_t cost)
{
    pthread_mutex_lock(&lock);
    mySelf.table[mySelf.myID][dst].cost = cost;
    mySelf.table[mySelf.myID][dst].nextHop = nextHop;
    pthread_mutex_unlock(&lock);
}

void addLink(char *nodeIP, uint8_t cost)
{
    pthread_mutex_lock(&lock);
	mySelf.table[getID(myIP)][getID(nodeIP)] = createTableEntry(getID(nodeIP), cost);
    neigh[getID(nodeIP)] = 1;
    pthread_mutex_unlock(&lock);
}

void disableLink(uint16_t nodeID)
{
    //  Retira vetor do node ID da tabela    //
    pthread_mutex_lock(&lock);
    mySelf.table[getID(myIP)][nodeID] = createTableEntry(NOLINK, INF_COST);
    neigh[nodeID] = 0;
    pthread_mutex_unlock(&lock);
}

void initTable()
{
	//  Inicializa a tabela //
    for(uint8_t i = 0; i < MAX_IP; i++) {
        for(uint8_t j = 0; j < MAX_IP; j++) {
            //mySelf.table[i][j] = createTableEntry(NOLINK, INF_COST);
            mySelf.table[i][j].nextHop = NOLINK;
            mySelf.table[i][j].cost = INF_COST;
        }
    }

    //  Cria entrada do nó  //
    uint16_t myID = getID(myIP);
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
/*
void *ptRecvData(void* args)
{
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    while(!quit)    {
printf("ta\n");
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
*/

void printTable(struct tableEntry *table)
{
    fprintf(stdout, "destino\t\tcusto\tnextHop\n");
    pthread_mutex_lock(&lock);
    for(uint8_t i = 1; i < MAX_IP; i++) {
        if(table[i].nextHop != NOLINK)
            fprintf(stdout, "%s\t%hu\t%s\n", getIP(table[i].nextHop), table[i].cost, getIP(table[i].nextHop));
    }
    pthread_mutex_unlock(&lock);
}

ssize_t sendDatagram(char *type, uint16_t nodeID, char *ip, struct tableEntry *table)
{
    struct sockaddr_in addr;
    socklen_t lenAddr = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORTA);
    addr.sin_addr.s_addr = inet_addr(getIP(mySelf.table[mySelf.myID][nodeID].nextHop));
    if(!strcmp("trace", type))
        return sendto(myfd, (char *) ip, strlen(ip) * sizeof(char), 0, (struct sockaddr *) &addr, lenAddr);
    else if(!strcmp("update", type))
        return sendto(myfd, (struct tableEntry *) table, MAX_IP * sizeof(struct tableEntry), 0, (struct sockaddr *) &addr, lenAddr);

    return -1;        
}

void updateTable(struct tableEntry *table, uint16_t id)
{
printf("updateTable id = %hu.\n", id);
    pthread_mutex_lock(&lock);
    for(uint8_t i = 1; i < MAX_IP; i++)
        mySelf.table[id][i] = table[i];
    pthread_mutex_unlock(&lock);
}

void updateMySelfTable()
{
    pthread_mutex_lock(&lock);
    for(uint8_t i = 1; i < MAX_IP; i++) {
        uint8_t curr = mySelf.table[mySelf.myID][i].cost;
        for(uint8_t j = 1; j < MAX_IP; j++) {
            uint8_t new = mySelf.table[j][i].cost;
            new += mySelf.table[mySelf.myID][mySelf.table[j][i].nextHop].cost;
if((j == 2) && (i == 3))
    printf("new = %hhu, curr = %hhu.\n", new, curr);
            if(new < curr)  {
                mySelf.table[mySelf.myID][i].cost = new;
                mySelf.table[mySelf.myID][i].nextHop = mySelf.table[j][i].nextHop;
            }
        }
    }
    pthread_mutex_unlock(&lock);
}
