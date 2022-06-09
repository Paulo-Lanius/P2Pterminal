#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#define ECHOMAX 255

char *peersIP[10];
int peerC;    
char linhaRecebida[ECHOMAX], linhaRetorno[ECHOMAX], linhaEnvio[ECHOMAX];
int loc_sockfd, loc_newsockfd, rem_sockfd;
struct sockaddr_in loc_addr, rem_addr;
pthread_t threads[2];  

void envio(char *ip) {

    bzero((char *)&rem_addr,sizeof(rem_addr));
    rem_addr.sin_family = AF_INET;
    rem_addr.sin_addr.s_addr = inet_addr(ip); // Endereço do peer destino
    rem_addr.sin_port = htons(6000);          // Porta
    
    rem_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (rem_sockfd < 0) {
        perror("Criando stream socket");
        exit(1);
    }
    
    if (connect(rem_sockfd, (struct sockaddr *) &rem_addr, sizeof(rem_addr)) < 0) {
        perror("Conectando stream socket");
        exit(1);
    }
        
    send(rem_sockfd, &linhaEnvio, sizeof(linhaEnvio), 0);    
    recv(rem_sockfd, &linhaRetorno, sizeof(linhaRetorno), 0);    

    printf(" %s____________________________________________\n", ip); 
    printf("|                                                      |\n"); 
    printf(linhaRetorno);
    printf("|______________________________________________________|\n"); 

    close(rem_sockfd);
}

// Método que representa o lado servidor da conexão
void *receiveCommand(void *arg) {    
    do{
        listen(loc_sockfd, 5);

	    int tamanho = sizeof(struct sockaddr_in);   	
       	loc_newsockfd =	accept(loc_sockfd, (struct sockaddr *)&loc_addr, &tamanho);

        bzero(linhaRecebida, ECHOMAX);
        recv(loc_newsockfd, &linhaRecebida, sizeof(linhaRecebida), 0);                

        system(linhaRecebida);

        //Trecho para copiar saída do comando em um arquivo e depois ler ele
        FILE* output_file = NULL;
        char buffer[ECHOMAX];

        output_file = popen(linhaRecebida, "r");

        if(output_file)
        {
            int i = 0;
            char c;

            while((c = fgetc(output_file)) != EOF)
            {
                buffer[i] = c;
                i++;
            } 
            buffer[i] = '\0';			
        }
        //

        send(loc_newsockfd, &buffer, sizeof(buffer), 0);
    }
    while(strcmp(linhaRecebida,"exit"));
}

// Método que representa o lado cliente da conexão
void *sendCommand(void *arg) {
    do{
        fgets (linhaEnvio, ECHOMAX, stdin);

        printf(" Atual_________________________________________________\n"); 
        printf("|                                                      |\n"); 
        system(linhaEnvio);
        printf("|______________________________________________________|\n"); 

        // Envia o comando aos peers conectados
        for(int i = 0; i < peerC; i++){
            envio(peersIP[i]);
        }
    }while(strcmp(linhaEnvio,"exit"));
}

int main(int argc, char *argv[])
{
    // Armazena o número de peers conectados e quais são eles
    peerC = argc-1;
    for(int i = 1; i < argc; i++)
    {
        peersIP[i-1] = argv[i];
    }

    // Criação do socket
    loc_sockfd = socket(AF_INET, SOCK_STREAM, 0);		
	if (loc_sockfd < 0) {
		perror("Criando stream socket");
		exit(1);
	}

    bzero((char *)&loc_addr, sizeof(loc_addr));
	loc_addr.sin_family = AF_INET;
	loc_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //Endereço atual
	loc_addr.sin_port = htons(6000);                //Porta

    if (bind(loc_sockfd, (struct sockaddr *) &loc_addr, sizeof(struct sockaddr)) < 0) {
		perror("Ligando stream socket");
		exit(1);
	}
    
    printf("Conectado!\n\n");    
    //Cria as threads de envio e recebimento
    pthread_create(&(threads[0]), NULL, receiveCommand, NULL);
    pthread_create(&(threads[1]), NULL, sendCommand, NULL);        
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    
    close(loc_sockfd);
	close(loc_newsockfd);
    return 0;
}