// servidor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define QLEN 3

#define BUFF_SIZE 1024
#define END_TAG "__END__\n"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Erro:\n Uso: %s <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd_s, fd_c;
    struct sockaddr_in servidor, cliente;
    socklen_t longClient = sizeof(cliente);
    struct hostent* info_cliente;
    char buf_peticion[BUFF_SIZE];

    // Creando el descriptor de socket del servidor
    fd_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (fd_s < 0) {
        perror("Error: Server-socket()\nNo se pudo crear el socket para aceptar conexiones\n");
        exit(EXIT_FAILURE);
    }else{
        printf("Success: Server-socket()\n");
    }

    if (setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1){
        perror("Error: Server-setsockopt()\n");
        exit(EXIT_FAILURE);
    }else{
        printf("Success: Server-setsockopt()\n");
    }

    // Configurando la dirección y puerto del servidor
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons((u_short) atoi(argv[1]));
    memset(&(servidor.sin_zero), '\0', 8);

    // Enlazando el socket con la dirección y puerto
    if (bind(fd_s, (struct sockaddr *)&servidor, sizeof(servidor)) < 0) {
        perror("Error:  Server-bind()\nNo se pudo enlazar el socket con una dirección y puerto");
        exit(EXIT_FAILURE);
    }else{
        printf("Success: Server-bind()\n");
    }

    // Poniendo el socket en modo escucha
    listen(fd_s,QLEN);
    if (fd_s < 0) {
        perror("Error: Server-listen()\nNo se pudo poner el socket en modo escucha");
        exit(EXIT_FAILURE);
    }else{
        printf("Success: Server-listen()\n");
    }
    printf("Servidor iniciado en %s:%d\n", inet_ntoa(servidor.sin_addr), ntohs(servidor.sin_port));
    printf("Esperando conexiones...\n");

    // Bucle principal para aceptar conexiones de clientes
    while (1) {
        // Aceptando una conexión de cliente
        fd_c = accept(fd_s, (struct sockaddr *)&cliente, &longClient);
        if (fd_c < 0) {
            perror("Error: Client-accept()\nNo se pudo aceptar la conexión del cliente");
            continue;
        }

        info_cliente = gethostbyaddr((char *) &cliente.sin_addr, sizeof(struct in_addr), AF_INET);
        
        char *hostname;
        if (info_cliente != NULL && info_cliente->h_name != NULL) {
            hostname = info_cliente->h_name;
        } else {
            hostname = "unknown";
        }
        // Concatenar hostname@ipaddress y mostrarlo
        char hostinfo[strlen(hostname) + INET_ADDRSTRLEN + 2]; // +2 for '@' and '\0'
        snprintf(hostinfo, sizeof(hostinfo), "%s@%s", hostname, inet_ntoa(cliente.sin_addr));
        printf("\nConectado: %s\n", hostinfo);


        while (1) {
            // Recibiendo comandos del cliente
            memset(buf_peticion, 0, BUFF_SIZE);
            int n = recv(fd_c, buf_peticion, BUFF_SIZE - 1, 0);
            if (n <= 0) break;

            buf_peticion[strcspn(buf_peticion, "\n")] = '\0';
            printf("%s $ %s  :  ", hostinfo, buf_peticion);

            if (strcmp(buf_peticion, "salir") == 0 || strcmp(buf_peticion, "exit") == 0){
                printf("SUCCESS\n");
                break;
            }
                
            // Ejecutando el comando recibido
            FILE *ptr_outputCommand = popen(buf_peticion, "r");
            if (ptr_outputCommand == NULL) {
                perror("ERROR\n");
                char *buf_error = "Error: Cliente.popen()\n";
                send(fd_c, buf_error, strlen(buf_error), 0);
                send(fd_c, END_TAG, strlen(END_TAG), 0);
                continue;
            }

            printf("SUCCESS\n");

            char buf_respuesta[BUFF_SIZE];
            while (fgets(buf_respuesta, sizeof(buf_respuesta), ptr_outputCommand) != NULL)
                send(fd_c, buf_respuesta, strlen(buf_respuesta), 0);

            pclose(ptr_outputCommand);
            send(fd_c, END_TAG, strlen(END_TAG), 0);
        }

        close(fd_c);
        printf("Desconectado: %s\n",hostinfo);
    }

    close(fd_s);
    exit(EXIT_SUCCESS);
}
