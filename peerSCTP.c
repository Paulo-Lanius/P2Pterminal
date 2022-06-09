#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#include <netinet/sctp.h>

#define ECHOMAX 255

char *peersIP[10];
int peerC;    
char linhaEnvio[ECHOMAX], linhaRetorno[ECHOMAX], linhaRecebida[ECHOMAX];
int loc_sockfd, loc_newsockfd, rem_sockfd;
struct sockaddr_in me, target;
struct sctp_initmsg initmsg;
pthread_t threads[2];  

void envio(char *ip) {

    bzero((char *)&target,sizeof(target));
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(ip); // Endereço do peer destino
    target.sin_port = htons(6000);          // Porta
    
    rem_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (rem_sockfd < 0) {
        perror("Criando stream socket");
        exit(1);
    }
    
    if (connect(rem_sockfd, (struct sockaddr *) &target, sizeof(target)) < 0) {
        perror("Conectando stream socket");
        exit(1);
    }
        
    sctp_sendmsg(rem_sockfd, &linhaEnvio, sizeof(linhaEnvio), NULL, 0, 0, 0, 0, 0, 0);                
    sctp_recvmsg(rem_sockfd, &linhaRetorno, sizeof(linhaRetorno), NULL, 0, 0, 0);

    printf(" %s____________________________________________\n", ip); 
    printf("|                                                      |\n"); 
    printf(linhaRetorno);
    printf("|______________________________________________________|\n"); 

    close(rem_sockfd);
}

// Método que representa o lado servidor da conexão
void *receiveCommand(void *arg) {   
    do{                
        listen(loc_sockfd, initmsg.sinit_max_instreams);        

        int tamanho = sizeof(struct sockaddr_in);
        loc_newsockfd = accept(loc_sockfd, (struct sockaddr *)&me, &tamanho);
        
        bzero(linhaRecebida, ECHOMAX);           
        sctp_recvmsg(loc_newsockfd, &linhaRecebida, sizeof(linhaRecebida), NULL, 0, 0, 0);
                   
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
        
        sctp_sendmsg(loc_newsockfd, &buffer, sizeof(buffer), NULL, 0, 0, 0, 0, 0, 0);
    }
    while(strcmp(linhaRecebida,"exit"));
}

// Método que representa o lado cliente da conexão
void *sendCommand(void *arg) {    
    do{
        fgets(linhaEnvio, ECHOMAX, stdin);

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

    bzero((char *)&me, sizeof(me));
	me.sin_family = AF_INET;
	me.sin_addr.s_addr = htonl(INADDR_ANY); //Endereço atual
	me.sin_port = htons(6000);              //Porta

    bzero((char *)&initmsg, sizeof(initmsg));
	initmsg.sinit_num_ostreams = 5;
	initmsg.sinit_max_instreams = 5;
	initmsg.sinit_max_attempts = 4;   

    // Criação do socket
    loc_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (loc_sockfd < 0) {
		perror("Criando stream socket");
		exit(1);
	}

    if (bind(loc_sockfd, (struct sockaddr *) &me, sizeof(struct sockaddr)) < 0) {
		perror("Ligando stream socket");
		exit(1);
	}

    if (setsockopt (loc_sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof (initmsg)) < 0){
		perror("setsockopt(initmsg)");
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