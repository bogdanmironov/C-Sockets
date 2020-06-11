#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>

#define ever (;;)
#define PORT 8888
#define MAX_CLIENTS 30
#define MAX_USERNAME_LEN 255
#define MAX_MESSAGE_SIZE 1024

char usernames[MAX_CLIENTS][MAX_USERNAME_LEN];
int number_of_clients;

void get_message_and_leave_reciever(char*, char*);
int contains(char usernames[MAX_CLIENTS][MAX_USERNAME_LEN], char *buffer);
int setup_server(struct sockaddr_in *);
int setup_select(int server_socket, fd_set *readfds, int *client_sockets);
void add_socket(int server_socket, struct sockaddr_in address, int (*client_sockets)[MAX_CLIENTS]);
void add_socket_to_list(int new_socket, int (*client_sockets)[MAX_CLIENTS], char *name);
void relay_message(int *client_sockets, fd_set readfds);

int main (int argc, char *argv[]) {

    int client_sockets[MAX_CLIENTS] = {0};
    struct sockaddr_in address;
    int server_socket = setup_server(&address);
    fd_set readfds;

    for ever {
        int max_sd = setup_select(server_socket, &readfds, client_sockets);

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ( (activity < 0) && (errno!=EINTR) ) printf("Select error");

        if ( FD_ISSET(server_socket, &readfds) ) {

            add_socket(server_socket, address, &client_sockets);

        } else {

            relay_message(client_sockets, readfds);

        }

    }

    return 0;
}

void relay_message(int *client_sockets, fd_set readfds) {
    for ( int i = 0; i < MAX_CLIENTS; ++i ) {
        int sd = client_sockets[i];
        char buffer[1024] = {0};
        
        if ( FD_ISSET( sd, &readfds) ) {
            int valread = recv(sd, buffer, 1024, 0);
            if ( valread == 0 ) {
                printf("%s disconected.\n", usernames[i]);

                close( sd );   
                client_sockets[i] = 0; 
            } else {
                if( strstr(buffer, "#") == NULL ) {
                    send(client_sockets[i], "E400", 4, 0);
                    break;
                }

                buffer[valread] = '\0';  
                
                char message[MAX_MESSAGE_SIZE];
                get_message_and_leave_reciever(buffer, message);

                int recipient_id = contains(usernames, buffer);

                if ( recipient_id == -1 ) {
                    printf("Such username was not found.\n");
                    send(client_sockets[i], "E404", 4, 0);
                    break;
                }

                memset(buffer, 0, sizeof(buffer));
                strcat(buffer, usernames[i]);
                strcat(buffer, "#");
                strcat(buffer, message);

                send(client_sockets[recipient_id], buffer, strlen(buffer), 0);

            }
        }
    }
}

void add_socket(int server_socket, struct sockaddr_in address, int (*client_sockets)[MAX_CLIENTS]){
    int addrlen = sizeof(address);

    int new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);

    if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    char buffer[1024] = {0};
    recv(new_socket, buffer, 1024, 0);
    printf("New connection , socket fd is %d , ip is : %s , port : %d , username: %s \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs (address.sin_port), buffer); 


    add_socket_to_list(new_socket, &*client_sockets, buffer);

    int number_of_clients_n = htonl(number_of_clients);
    send(new_socket, &number_of_clients_n, sizeof(number_of_clients), 0);

    for (int i = 0; i < number_of_clients; ++i) {
        send(new_socket, usernames[i], strlen(usernames[i]), 0);
        printf("%s\n", usernames[i]);
    }
    

}

void add_socket_to_list(int new_socket, int (*client_sockets)[MAX_CLIENTS], char *name) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {    
        if ((*client_sockets)[i] == 0) {   
            (*client_sockets)[i] = new_socket;   
            printf("Adding to list of sockets as %d\n" , i);   
                    
            strcpy(usernames[i], name);

            number_of_clients = i + 1;
            break;   
        }   
    } 
}

int setup_select(int server_socket, fd_set *readfds, int *client_sockets) {
    FD_ZERO(&*readfds);

    FD_SET(server_socket, &*readfds);
    int max_sd = server_socket;

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        int sd = client_sockets[i];

        if(sd > 0) {
            FD_SET(sd, &*readfds);
        }

        if (sd > max_sd) {
            max_sd = sd;
        }
    }

    return max_sd;
}

int setup_server(struct sockaddr_in *address) {
    int opt = 1;
    
    int server_socket;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *) address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    return server_socket;
}

void get_message_and_leave_reciever(char *buffer, char *msg) {
    const char delim[2] = "#";
    char *token;

    token = strtok(buffer, delim);
    token = strtok(NULL, delim);

    strcpy(msg, token);
}

int contains(char usernames[MAX_CLIENTS][MAX_USERNAME_LEN], char *buffer) {
    int contains = -1;

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (strcmp(usernames[i], buffer) == 0) {
            contains = i;
        }
    }

    return contains;
}