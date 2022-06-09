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
int respostas = 0;

int sock;
struct sockaddr_in me, from;
int adl = sizeof(me);

pthread_t threads[2];  

// Método que representa o lado servidor da conexão
void *receiveCommand(void *arg) {
    char linha[ECHOMAX];
    char retorno[ECHOMAX];

    do{
        bzero(linha, ECHOMAX);
        recvfrom(sock, linha, ECHOMAX, 0, (struct sockaddr *)&from, &adl);

        respostas = respostas-1;
        if(respostas <= 0){   
            respostas = 0;        

            system(linha);

            //Trecho para copiar saída do comando em um arquivo e depois ler ele
            FILE* output_file = NULL;
            char buffer[ECHOMAX];

            output_file = popen(linha, "r");

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

            sendto(sock, buffer, ECHOMAX, 0, (struct sockaddr *)&from, sizeof(from));            
        }
        else{
            printf(" ______________________________________________________\n"); 
            printf("|                                                      |\n"); 
            printf(linha);    
            printf("|______________________________________________________|\n"); 
        }
    }
    while(strcmp(linha,"exit"));
}

// Método que representa o lado cliente da conexão
void *sendCommand(void *arg) {
    char linha[ECHOMAX];

    do{
        gets(linha);

        printf(" Atual_________________________________________________\n"); 
        printf("|                                                      |\n"); 
        system(linha);
        printf("|______________________________________________________|\n"); 

        // Envia o comando aos peers conectados
        for(int i = 0; i < peerC; i++){
            struct sockaddr_in target;
 
            bzero((char *)&target,sizeof(target));
            target.sin_family = AF_INET;
            target.sin_addr.s_addr = inet_addr(peersIP[i]); // Endereço do peer destino
            target.sin_port = htons(6000);                  // Porta
            
            sendto(sock, linha, ECHOMAX, 0, (struct sockaddr *)&target, sizeof(target));
            respostas = peerC+1;
        }
    }while(strcmp(linha,"exit"));
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
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("ERRO na Criacao do Socket!\n");

    bzero((char *)&me, sizeof(me));
	me.sin_family = AF_INET;
	me.sin_addr.s_addr=htonl(INADDR_ANY); //Endereço atual
	me.sin_port = htons(6000);            //Porta

    if(bind(sock, (struct sockaddr *)&me, sizeof(me)) != -1)
    {
        printf("Conectado!\n\n");    
        //Cria as threads de envio e recebimento
        pthread_create(&(threads[0]), NULL, receiveCommand, NULL);
        pthread_create(&(threads[1]), NULL, sendCommand, NULL);        
        pthread_join(threads[0], NULL);
        pthread_join(threads[1], NULL);
    }
    else puts("Porta ocupada");
    close(sock);
    return 0;
}