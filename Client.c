#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <string.h> 
#include <pthread.h>
#include <stdint.h>

#define ever (;;)   

#define PORT 8888
#define MAX_MESSAGE_SIZE 1024
#define MAX_USERNAME_LEN 255

void *print_recieved_messages(void* socket); //Thread_func

int connect_to_server();
void create_printer(int sock);
void choose_username(int sock);
void get_and_send_messages(int sock);

int main(int argc, char *argv[]) {
    int sock = connect_to_server();
    choose_username(sock);
    create_printer(sock);

    
    while(1) {
        get_and_send_messages(sock);
    }
    
    return 0;
}

void get_and_send_messages(int sock) {
    char message[MAX_MESSAGE_SIZE];

    fgets(message, MAX_MESSAGE_SIZE, stdin);
    strtok(message, "\n");

    if(strcmp(message, "exit") == 0) exit(0);

    send(sock, message, strlen(message), 0);

}

void choose_username(int sock) {
    char username[MAX_USERNAME_LEN];

    printf("Choose username: ");
    fgets(username, MAX_USERNAME_LEN, stdin);
    strtok(username, "\n");

    send(sock, username, strlen(username), 0);
}

void create_printer(int sock) {
    pthread_t thread;

    int rc = pthread_create(&thread, NULL, print_recieved_messages, (void *) (intptr_t)sock);

    if (rc) {
        printf("ERROR; pthread_create() return %d\n", rc);
        exit(-1);
    }
}

int connect_to_server() {
    int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    return sock;
}

void *print_recieved_messages(void *socket) {
    int sock = (intptr_t) socket;
    const char delim[2] = "#";
    char *token;

    for ever {
        char buffer[1024] = {0};
        if (recv(sock, buffer, 1024, 0) == 0) {
            pthread_exit(0);
        }

        if (strcmp("E404", buffer) == 0) {
            printf("There is no such username.\n");
            continue;
        }

        if (strcmp("E400", buffer) == 0) {
            printf("Invalid format.[Recipient#Message]\n");
            continue;
        }

        token = strtok(buffer, delim);
        printf("Recieved a message from %s.\n", token);
        token = strtok(NULL, delim);
        printf("Message: %s\n", token);
    }

}