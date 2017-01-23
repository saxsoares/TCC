#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define REQ_LEN (64*1024)
#define RESP_LEN REQ_LEN
#define TAM_BUFFER REQ_LEN

int main(int argc, char **argv) {
	int dotCounter = 0;
	int funcao = 0;	//1 para download | 2 para upload

	unsigned char buffer[TAM_BUFFER];
	int descritor;

	if (argc != 3) {
		printf("uso: %s [host1:]file1 [host2:]file2\n", argv[0]);
		return 0;
	}

	if(index(argv[1], ':') != NULL){	//Verifica se deve executar função download ou upload. Download
		funcao = 1;
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;        // modo de abertura/criação do arquivo
		descritor = open(argv[2], O_CREAT | O_RDWR | O_TRUNC, mode);// realizando abertura do arquivo a ser escrito
		if(descritor < 0){
			printf("Erro ao criar arquivo");
			return -1;
		}
	}
	else{			//Upload
		funcao = 2;
		descritor = open(argv[1], O_RDONLY);        // abrindo o arquivo para leitura
		if(descritor < 0){
			printf("Erro ao criar arquivo");
			return -1;
		}
	}

	int i = 0;
	int w = 1;

	while(w){
		if(argv[funcao][i] != ':')	//Encontrar a separacao entre o IP e o diretorio remoto
			i++;                    // poderia ser utilizada a função 'index' aqui, mas assim funciona
		else w = 0;
	}
//Atribuicao dos vetores de char para o caso definido
	char endereco[i];
	char diretorio[strlen(argv[funcao]) - i];	//Lembrando que o valor de 'funcao' eh 1 ou 2

	strncpy(endereco, argv[funcao], i);		//	  //	   //	    //	     //    //
	strcpy(diretorio, &argv[funcao][i + 1]);	//	  //	   //	    //	     //    //
//Atribuicao dos vetores de char para o caso definido

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   // Criando socket de comunicação
	if (sock < 0) {
		perror("socket()");
		return -1;
	}

	char response[REQ_LEN];

	struct sockaddr_in saddr;
	bzero(&saddr, sizeof(saddr));
	saddr.sin_addr.s_addr = inet_addr(endereco);        // Preenchendo estrutura do socket
	saddr.sin_family = AF_INET;                         //
	saddr.sin_port = htons(5000);		//Porta de comunicacao padronizada entre o cliente e o servidor

	if (connect(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0){   // tentando conectar ao computador remoto
		perror("connect()");
		return -1;
	}
	else{
//Informar ao servidor se trata-se de download ou upload
		if(funcao == 1) send(sock, "1", 1, 0);  //Download
		else		send(sock, "2", 1, 0);      //Upload
//Informar ao servidor se trata-se de download ou upload

		if (send(sock, diretorio, strlen(diretorio), 0) < 0){
			perror("send()");
			return -1;
		}
		if (recv(sock, response, 3, 0) < 0){
			perror("recv()");
			return -1;
		}
		if(!strcmp(response, "ERR")){
			printf("ERRO: Diretorio ou nome de arquivo invalido ou inexistente\n");
			return -1;
		}

		bzero(buffer, TAM_BUFFER);

		if (descritor == -1) {
			perror("open()");
			return -1;
		}

		if(funcao == 1){
			void *ponteiro_buffer;
			int n, escrita;

			escrita = recv(sock, buffer, TAM_BUFFER, 0);
			printf("Downloading..");
			while(escrita > 0){
				dotCounter++;
				if(!(dotCounter%100))
					printf(".");
				if (escrita < 0)
					perror("recv()");

				ponteiro_buffer = buffer;
				n = write(descritor, ponteiro_buffer, escrita);
				if (n == -1) {
					perror("write()");
					close(descritor);
					return 0;
				}
				escrita = recv(sock, buffer, TAM_BUFFER, 0);
			}
			printf("\n");
		}
		else{
			int leitura;
			dotCounter++;
			if(!(dotCounter%100))
			printf("Uploading..");
			do {
				printf(".");
				bzero(buffer, TAM_BUFFER);
				leitura = read(descritor, buffer, TAM_BUFFER);
				if (leitura == -1) {
					perror("read()");
					close(descritor);
					return 0;
				}
				send(sock, buffer, strlen((void *)buffer), 0);
			}while(leitura > 0);
		}
			printf("\n");
			dotCounter = 0;
	}
	return 0;
}
