
// cliente.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 1024
#define END_TAG "__END__\n"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Error:\nUso: %s <ipServidor> <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd_c;
    struct sockaddr_in servidor;
    struct sockaddr_in cliente;
    socklen_t addr_len = sizeof(cliente);
    char *ip = argv[1];
    char buf_peticion[BUFF_SIZE];
    char buf_respuesta[BUFF_SIZE];

    // Creando el descriptor de socket para el cliente
    fd_c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_c < 0) {
        perror("Error: socket()\nNo se pudo crear el socket\n");
        exit(EXIT_FAILURE);
    }

    // Configurando la dirección y puerto del ser vidor a conectar
    memset((char *) &servidor, 0, sizeof(struct sockaddr_in));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons( (u_short) atoi(argv[2]) );
    if (inet_pton(AF_INET, ip, &servidor.sin_addr) != 1) {
        perror("Error: Dirección IP inválida");
        exit(EXIT_FAILURE);
    }

    // conectando al servidor
    if (connect(fd_c, (struct sockaddr *)&servidor, sizeof(struct sockaddr_in)) < 0) {
        perror("Error: No se pudo conectar al servidor");
        exit(EXIT_FAILURE);
    }
    
    struct hostent *server_hostent = gethostbyaddr((const void *)&servidor.sin_addr, sizeof(servidor.sin_addr), AF_INET);
    char *hostname;
    if (server_hostent != NULL && server_hostent->h_name != NULL) {
        hostname = server_hostent->h_name;
    } else {
        hostname = "unknown";
    }

    printf("Conectado al servidor %s@%s:%s\n", hostname, ip, argv[2]);

    // Formando prompt identificador del cliente
    char hostinfo[strlen(hostname) + INET_ADDRSTRLEN + 2]; // +2 for '@' and '\0'
    if (getsockname(fd_c, (struct sockaddr *)&cliente, &addr_len) == 0) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliente.sin_addr, client_ip, INET_ADDRSTRLEN);
        snprintf(hostinfo, sizeof(hostinfo), "%s@%s", hostname, client_ip);
    } else {
        snprintf(hostinfo, sizeof(hostinfo), "%s@you", hostname);
    }

    while (1) {
        printf("%s $ ",hostinfo);
        fflush(stdout);

        fgets(buf_peticion, BUFF_SIZE, stdin);
        buf_peticion[strcspn(buf_peticion, "\n")] = '\0';

        send(fd_c, buf_peticion, strlen(buf_peticion), 0);

        if (strcmp(buf_peticion, "exit") == 0 || strcmp(buf_peticion, "salir") == 0)
            break;

        int bytes_received;
        while (( bytes_received = recv(fd_c, buf_respuesta, BUFF_SIZE - 1, 0)) > 0) {
            buf_respuesta[bytes_received] = '\0';

            // Check if END_TAG is in this chunk 
            if (strstr(buf_respuesta, END_TAG) != NULL) {
                char *end_ptr = strstr(buf_respuesta, END_TAG);
                *end_ptr = '\0';
                printf("%s", buf_respuesta);
                break;
            }
            
            printf("%s", buf_respuesta);
        }
        printf("\n");
    }

    close(fd_c);
    exit(EXIT_SUCCESS);
}

