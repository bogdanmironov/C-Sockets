#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>

#define ever (;;)
#define PORT 8888
#define MAX_CLIENTS 30
#define MAX_USERNAME_LEN 255
#define MAX_MESSAGE_SIZE 1024

char usernames[MAX_CLIENTS][MAX_USERNAME_LEN];

void get_message_and_leave_reciever(char*, char*);
int contains(char usernames[MAX_CLIENTS][MAX_USERNAME_LEN], char *buffer);

int main (int argc, char *argv[]) {

    int server_fd, valread;
    int client_socket[MAX_CLIENTS], new_socket;
    int opt = 1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    fd_set readfds;
    int sd, max_sd;
    int activity;

    
    // char *welcome_message = "Hello from server"; //asdasdsad

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        client_socket[i] = 0;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for ever {
        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            sd = client_socket[i];

            if(sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno!=EINTR)) {
            printf("Select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            char buffer[1024] = {0};
            recv(new_socket, buffer, 1024, 0);
            printf("New connection , socket fd is %d , ip is : %s , port : %d , username: %s \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs (address.sin_port), buffer); 

            // if (send(new_socket, welcome_message, strlen(welcome_message), 0) != strlen(welcome_message)) {   
            //     perror("send");   
            // }    //TODO


            for (int i = 0; i < MAX_CLIENTS; ++i) {    
                if (client_socket[i] == 0) {   
                    client_socket[i] = new_socket;   
                    printf("Adding to list of sockets as %d\n" , i);   
                         
                    strcpy(usernames[i], buffer);

                    break;   
                }   
            } 
        } else {
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                sd = client_socket[i];
                char buffer[1024] = {0};
                
                if (FD_ISSET( sd, &readfds)) {
                    if ((valread = recv(sd, buffer, 1024, 0)) == 0) {
                        printf("%s disconected.\n", usernames[i]);

                        close( sd );   
                        client_socket[i] = 0; 
                    } else {
                        if(strstr(buffer, "#") == NULL) {
                            send(client_socket[i], "E400", 4, 0);
                            break;
                        }

                        buffer[valread] = '\0';  
                        
                        char message[MAX_MESSAGE_SIZE];
                        get_message_and_leave_reciever(buffer, message);

                        printf("%s\n", buffer);
                        printf("%s\n", message);

                        int recipient_id = contains(usernames, buffer);

                        if (recipient_id == -1) {
                            printf("Such username was not found.\n");
                            send(client_socket[i], "E404", 4, 0);
                            break;
                        }

                        memset(buffer, 0, sizeof(buffer));
                        strcat(buffer, usernames[i]);
                        strcat(buffer, "#");
                        strcat(buffer, message);

                        send(client_socket[recipient_id], buffer, strlen(buffer), 0);

                    }
                }
            }
        }

    }

    return 0;
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