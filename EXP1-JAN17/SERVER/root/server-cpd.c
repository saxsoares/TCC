#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>


#define REQ_LEN (64*1024)
#define RESP_LEN REQ_LEN
#define MAX_CON 1024
#define TAM_BUFFER REQ_LEN

struct param{
	pthread_t tid;
	int csock;
	struct sockaddr_in caddr;
};
typedef struct param param;


void *handle_con(void *args) {
	unsigned char buffer[TAM_BUFFER];
	param p = *(param *)args;
	char request[REQ_LEN], response[RESP_LEN];

	bzero(request, REQ_LEN);
	bzero(response, RESP_LEN);

	if (recv(p.csock, request, 1, 0) < 0){	//1 Para download e 2 para upload
		perror("recv()");
		pthread_exit(NULL);
	}

	int funcao = atoi(request);

	if (recv(p.csock, request, REQ_LEN, 0) < 0) {
		perror("recv()");
		pthread_exit(NULL);
	}

	int descritor;
	if(funcao == 1) //Download
		descritor = open(request, O_RDONLY);
	else{           //Upload
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		descritor = open(request, O_CREAT | O_RDWR | O_TRUNC, mode);
	}

	if (descritor == -1) {		//Abertura do arquivo nao foi bem sucedida
		char MESSAGE[3] = {'E', 'R', 'R'};	//ERRO
		send(p.csock, MESSAGE, strlen(MESSAGE), 0);
		perror("open()");

		pthread_exit(NULL);
	}

	//Arquivo foi aberto com sucesso e o programa continua
	char MESSAGE[3] = {'A', 'C', 'K'};
	if(send(p.csock, MESSAGE, strlen(MESSAGE), 0) < 0){
		perror("ack()");
		pthread_exit(NULL);
	}
	int dotCounter = 0;

	if(funcao == 1){	//Download
		int leitura;
		printf("Uploading..");
		do {
			dotCounter++;
			if(!(dotCounter % 100))
				printf(".");
			bzero(buffer, TAM_BUFFER);
			leitura = read(descritor, buffer, TAM_BUFFER);
			if (leitura == -1) {
				perror("read()");
				close(descritor);
				return 0;
			}
			send(p.csock, buffer, strlen( (void *)buffer), 0);
		}while(leitura > 0);
		printf("\n");
	}
	else{			//Upload
		void *ponteiro_buffer;
		int n, escrita;
		printf("Downloading..");
		while((escrita = recv(p.csock, buffer, TAM_BUFFER, 0) )> 0){
			dotCounter++;
			if(!(dotCounter % 10))
				printf(".");
	        	ponteiro_buffer = buffer;
			n = write(descritor, ponteiro_buffer, escrita);
			if (n == -1) {
				perror("write()");
				close(descritor);
				return 0;
			}
		}
		printf("\n");
	}
	close(p.csock);
	close(descritor);
	pthread_exit(NULL);
}


int main(int argc, char **argv) {

	struct sockaddr_in saddr;

//	if (argc != 2) {
//		printf("uso: %s <porta>\n", argv[0]);
//		return 0;
//	}

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   // criação do socket de comunicação
	if (sock < 0) {
		perror("socket()");
		return -1;
	}

	saddr.sin_family = AF_INET;          // Preenchendo estrutura saddr.
	saddr.sin_port = htons(5000);		//Porta de comunicacao padronizada entre o cliente e o servidor
	saddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0){  // executando o bind no socket de comunicação, assimilando um endereço ao socket criado
		perror("bind()");
		return -1;
	}

	if (listen(sock, MAX_CON) < 0){     // Ouvindo a partir do socket criado, na porta especificada no preenchimento da estrutura saddr
		perror("listen()");
		return -1;
	}
	int csock, caddr_len;
	struct sockaddr_in caddr;
	int i = 0;
	param p[MAX_CON];

	while(1) {
		bzero(&caddr, sizeof(caddr));
		caddr_len = sizeof(caddr);

		csock = accept(sock, (struct sockaddr *)&caddr, (socklen_t *)&caddr_len);   // Aguardando Conexão
		if (csock < 0){
			perror("accept()");
			return -1;
		}
		p[i].csock = csock;                 // quuando o cliente conecta, preenche a estrutura de parametros da thread
		p[i].caddr = caddr;                 //
		pthread_create(&p[i].tid, NULL, handle_con, &p[i]);     // lança a thread e volta a auguardar outras conexões
		i = (i + 1) % MAX_CON;
	}
	return 0;
}
