#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <string.h> 
#include <pthread.h>
#include <stdint.h>
#include <curses.h>

#define ever (;;)   
#define MAX_CLIENTS 30
#define PORT 8888
#define MAX_MESSAGE_SIZE 1024
#define MAX_USERNAME_LEN 1024 //To account for concatenation(to become a message)

char usernames[MAX_CLIENTS][MAX_USERNAME_LEN];
int number_users = 0;

void *print_recieved_messages(void* socket); //Thread function
void *menu(void *socket); //Thread function
void *update_users(void *socket); //Thread function
void init_client(int sock);

pthread_mutex_t m_print_message; //Mutex start with m_
void mutex_init();

int connect_to_server();
void create_printer(int sock);
void create_menu(int sock);
void create_username_updater(int sock);
void choose_username(int sock);
void read_and_send_messages(int sock, char *recipent);


// WINDOW *win;

int main(int argc, char *argv[]) {
    mutex_init();
    int sock = connect_to_server();
    choose_username(sock);
    init_client(sock);

    
    for ever {
        //Termination is in read_and_send_message()
    }
    
    endwin();
    pthread_mutex_destroy(&m_print_message); //Just in case
    return 0;
}

void mutex_init() {
    if (pthread_mutex_init(&m_print_message, NULL) != 0) { 
        printf("Mutex init failed.\n"); 
        exit( 1 ); 
    } 
}

void read_and_send_message(int sock, char *recipient) {
    char message[MAX_MESSAGE_SIZE];

    getstr(message);
    strtok(message, "\n");

    if(strcmp(message, "exit") == 0) {
        endwin();
        pthread_mutex_destroy(&m_print_message);
        exit(0);
    }

    strcat(recipient, "#");
    strcat(recipient, message);

    send(sock, message, strlen(message), 0);
}

void choose_username(int sock) {
    char username[MAX_USERNAME_LEN];

    printf("Choose username: ");
    fgets(username, MAX_USERNAME_LEN, stdin);
    strtok(username, "\n");

    printf("done\n");

    send(sock, username, strlen(username), 0);
}

void init_client(int sock) {
    create_username_updater(sock);
    sleep(1);
    create_menu(sock);
    create_printer(sock);
}

void create_username_updater(int sock) {
    pthread_t thread;

    int rc = pthread_create(&thread, NULL, update_users, (void *) (intptr_t)sock);

    if (rc) {
        printf("ERROR; pthread_create(update_users) return %d\n", rc);
        exit(-1);
    }
}

void create_menu(int sock) {
    pthread_t thread;

    int rc = pthread_create(&thread, NULL, menu, (void *) (intptr_t)sock);

    if (rc) {
        printf("ERROR; pthread_create(menu) return %d\n", rc);
        exit(-1);
    }
}

void create_printer(int sock) {
    pthread_t thread;

    int rc = pthread_create(&thread, NULL, print_recieved_messages, (void *) (intptr_t)sock);

    if (rc) {
        printf("ERROR; pthread_create(printer) return %d\n", rc);
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

void *update_users(void *socket) {
    int sock = (intptr_t) socket;


    for ever {
        recv(sock, &number_users, sizeof(number_users), 0);

        // pthread_mutex_lock(&m_print_message);
        number_users = ntohl(number_users);
        printf("%d\n", number_users);

        for (int i = 0; i < number_users; ++i) {
            int len_username;
            recv(sock, &len_username, sizeof(unsigned int), 0);
            recv(sock, usernames[i], len_username, 0);
        }
        // pthread_mutex_unlock(&m_print_message); //we use mutex to ensure that we are not printing obsolete info
        sleep(1);
    }
}

void *menu(void *socket) {
    initscr(); // Curses init
    int sock = (intptr_t) socket;

    int position = 0;
    int input;

    

    for ever {
        // printf("HI\n");
        // update_users(sock);
        pthread_mutex_lock(&m_print_message);
        int pos = 0;
        move(pos,0);
        cbreak(); //Get a character at a time
        noecho();
        keypad(stdscr, TRUE);
        
        addstr("Active users:\n");
        for ( int print = 0; print < number_users; print++ ) {
            pos++;

            move(pos, 0);
            if ( print == position ) addstr(">");
            else addstr(" ");

            addstr(usernames[print]);
        }

        move(++pos, 0);
        addstr("Press arrow keys to move,Enter to choose;\n");

        refresh();

        // pthread_mutex_unlock(&m_print_message);


        input = getch();
        // system("clear");

        // if( input == 224 || input == 0 ){
        //     input = getch();
        // }

        switch( input ){
            case KEY_UP: if( position > 0 ) position -= 1; break;
            case KEY_DOWN: if( position < number_users - 1 ) position+=1; break;
        }

        
        // pthread_mutex_lock(&m_print_message);

        if( input == KEY_ENTER ){
            // system("clear");
            addstr("Enter message:\n");
            read_and_send_message(sock, usernames[position]);
        }

        nocbreak(); //Restore default settings
        pthread_mutex_unlock(&m_print_message);
    }
}

// Cannot clear the recieved msg to show only the menu so
// sleeping for 1s (disabling input) to turn attention to msg
void *print_recieved_messages(void *socket) {
    int sock = (intptr_t) socket;
    const char delim[2] = "#";
    char *token;

    for ever {
        char buffer[1024] = {0};
        if (recv(sock, buffer, 1024, 0) == 0) {
            pthread_exit(0);
        }

        pthread_mutex_lock(&m_print_message);

        if (strcmp("E404", buffer) == 0) {
            addstr("There is no such username.\n");
            continue;
        }

        if (strcmp("E400", buffer) == 0) {
            addstr("Invalid format.[Recipient#Message]\n");
            continue;
        }

        int h, w;
        getmaxyx(stdscr, h, w);
        move( h-4, 0 );
        token = strtok(buffer, delim);
        addstr("Recieved a message from ");
        addstr(token);
        addstr("\n");
        token = strtok(NULL, delim);
        addstr("Message: ");
        addstr(token);
        addstr("\n");
        // sleep(1);

        // printf("\nPress key to continue\n");
        // getchar();
        // printf("");

        pthread_mutex_unlock(&m_print_message);
    }

}