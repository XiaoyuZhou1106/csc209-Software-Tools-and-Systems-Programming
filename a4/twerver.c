#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
    #define PORT 51147
#endif

#define LISTEN_SIZE 5
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username: "
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define MSG_LIMIT_WAR "Arrived message limit\r\n"
#define INVALID_NAME_WAR "Sorry, you input an invalid username\r\n"
#define INVALID_FOLLOW_NAME "Sorry, we cannot find the user you input\r\n"
#define FOLLOWING_LIMIT_WAR "Sorry, your following list is full\r\n"
#define FOLLOWER_LIMIT_WAR "Sorry, that user's follower list is full\r\n"
#define REPEAT_FOLLOW "Sorry, you have already followed that user\r\n"
#define INVALID_COMMAND "Invalid command\r\n"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define FOLLOW_LIMIT 5

struct client {
    int fd;
    struct in_addr ipaddr;
    char username[BUF_SIZE];
    char message[MSG_LIMIT][BUF_SIZE];
    struct client *following[FOLLOW_LIMIT]; // Clients this user is following
    struct client *followers[FOLLOW_LIMIT]; // Clients who follow this user
    char inbuf[BUF_SIZE]; // Used to hold input from the client
    char *in_ptr; // A pointer into inbuf to help with partial reads
    struct client *next;
};


// Provided functions. 
void add_client(struct client **clients, int fd, struct in_addr addr);
void remove_client(struct client **clients, int fd);

//helper functions:
void activate_client(struct client *c, struct client **active_clients_ptr, struct client **new_clients_ptr);
void announce(struct client *active_clients, char *s);
int read_from_client(int cur_fd, char *client_inbuf);
int find_network_newline(const char *buf, int n);
int follow(struct client *client, struct client *active_client, char *username);
int unfollow(struct client *client, struct client *active_client, char *username);
int send_message(struct client *client, char *s, struct client *active_clients);
void show(struct client *client, struct client *active_client);


// The set of socket descriptors for select to monitor.
// This is a global variable because we need to remove socket descriptors
// from allset when a write to a socket fails. 
fd_set allset;

/* 
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client(struct client **clients, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));
    p->fd = fd;
    p->ipaddr = addr;
    p->username[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *clients;
    
    //initialize following and follower
    for (int j = 0; j < FOLLOW_LIMIT; j++) {
        p->followers[j] = NULL;
        p->following[j] = NULL;
    }

    // initialize messages to empty strings
    for (int i = 0; i < MSG_LIMIT; i++) {
        p->message[i][0] = '\0';
    }

    *clients = p;
}

/* 
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client(struct client **clients, int fd) {
    struct client **p;

    for (p = clients; *p && (*p)->fd != fd; p = &(*p)->next)
        ;

    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        // Remove the client from other clients' following lists
        int i = 0;
        while (i < FOLLOW_LIMIT && (*p)->following[i] != NULL) {
            //find the client in the followers
            int j;
            for (j = 0; (*p)->following[i]->followers[j]->fd != fd; j++);
            //delete the client from the list
            for (j = j; (j < FOLLOW_LIMIT - 1) && ((*p)->following[i]->followers[j] != NULL); j++) {
                (*p)->following[i]->followers[j] = (*p)->following[i]->followers[j + 1];
            }
            (*p)->following[i]->followers[j] = NULL;
            i++;
        }

        // Remove the client from other clients' follower lists
        i = 0;
        while (i < FOLLOW_LIMIT && (*p)->followers[i] != NULL) {
            //find the client in the following
            int j;
            for (j = 0; (*p)->followers[i]->following[j]->fd != fd; j++);
            //delete the client from the list
            for (j = j; (j < FOLLOW_LIMIT - 1) && ((*p)->followers[i]->following[j] != NULL); j++) {
                (*p)->followers[i]->following[j] = (*p)->followers[i]->following[j + 1];
            }
            (*p)->followers[i]->following[j] = NULL;
            i++;
        }
        //tell all users that the client quit
        char quit_msg[strlen((*p)->username) + 10];
        memset(quit_msg, '\0', sizeof(quit_msg));
        strcpy(quit_msg, "Goodbye ");
        strncat(quit_msg, (*p)->username, strlen((*p)->username));
        strcat(quit_msg, "\r\n");

        // Remove the client
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;

        //tell all users that the client quit
        announce(*clients, quit_msg);        
    } else {
        fprintf(stderr, 
            "Trying to remove fd %d, but I don't know about it\n", fd);
    }
}

/* 
 * Move client c from new_clients list to active_clients list. 
 */
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr) {
    struct client **p;

    //find the client that before c
    for (p = new_clients_ptr; *p && (*p)->fd != c->fd; p = &(*p)->next)
        ;
  
    //append p's next client to t, which is delecting c/p from the new_clients
    struct client *t = (*p)->next;
    *p = t;

    //add c to the beignning of active_clients.
    c->next = *active_clients_ptr;

    //change the active_clients_ptr points to c
    *active_clients_ptr = c;

    //tell all users and server that there is a new user
    printf("%s has just joined\r\n", c->username);
    char message[strlen(c->username) + 18];
    memset(message, '\0', sizeof(message));
    strncpy(message, c->username, strlen(c->username));
    strcat(message, " has just joined\r\n");
    announce(*active_clients_ptr, message);
    }

/*
 * Send the message in s to all clients in active_clients. 
 */
void announce(struct client *active_clients, char *s) {
    struct client *temp_client = active_clients;
    while (temp_client != NULL) {
        if (write(temp_client->fd, s, strlen(s)) == -1) {
            perror("error annouce");
            remove_client(&active_clients, temp_client->fd);
        }
        temp_client = temp_client->next;
    }
}


/*
 * Read the client's input by using the buffer.
 */
int read_from_client(int cur_fd, char *client_inbuf) {
    char buf[BUF_SIZE] = {'\0'};
    int inbuf = 0;           // How many bytes currently in buffer?
    int room = sizeof(buf);  // How many bytes remaining in buffer?
    char *after = buf;       // Pointer to position after the data in buf
    int nbytes;

    //use loop keep reading until find an entire line.
    while ((nbytes = read(cur_fd, after, room)) > 0) {
        inbuf += nbytes;
        printf("[%d] Read %d bytes\n", cur_fd, nbytes);

        //find a complete new line
        int where;
        while ((where = find_network_newline(buf, inbuf)) > 0) {
            buf[where - 2] = '\0';
            strncpy(client_inbuf, buf, where - 1);
            printf("[%d] Found newline: %s\n", cur_fd, buf);
            return 1;
        }
        after = &buf[inbuf];
        room = sizeof(buf) - inbuf;
    }
    return 0;
}


/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 */
int find_network_newline(const char *buf, int n) {
    for (int i = 0; i < n - 1; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            return i + 2;
        }
    }
    return -1;
}

/*
 * follow the user according to his/her username
 */
int follow(struct client *client, struct client *active_client, char *username) {
    //find the person that will be followed.
    struct client *aim_client = active_client;
    while (aim_client != NULL && strcmp(aim_client->username, username) != 0) {
        aim_client = aim_client->next;
    }
    //if there does not have the person
    if (aim_client == NULL) {
        if (write(client->fd, INVALID_FOLLOW_NAME, strlen(INVALID_FOLLOW_NAME)) == -1) {
            perror("error invalid follow name");
            remove_client(&active_client, client->fd);
        }
        return 1;
    } else {
        //test whether the client have already that user
        for (int a = 0; client->following[a] != NULL; a++) {
            if (client->following[a]->fd == aim_client->fd) {
                if (write(client->fd, REPEAT_FOLLOW, strlen(REPEAT_FOLLOW)) == -1) {
                    perror("error repeat follow");
                    remove_client(&active_client, client->fd);
                }
                return 1;
            }
        }

        //find that user's empty follower space.
        int i;
        for (i = 0; i < FOLLOW_LIMIT && aim_client->followers[i] != NULL; i++);
        //if that user's follower list is full:
        if (i == FOLLOW_LIMIT) {
            if (write(client->fd, FOLLOWER_LIMIT_WAR, strlen(FOLLOWER_LIMIT_WAR)) == -1) {
                perror("error follower limit");
                remove_client(&active_client, client->fd);
            }
            return 1;
        }
        //find the client's empty following space.
        int j;
        for (j = 0; j < FOLLOW_LIMIT && client->following[j] != NULL; j++);
        //if the client's following list is full.
        if (j == FOLLOW_LIMIT) {
            if (write(client->fd, FOLLOWING_LIMIT_WAR, strlen(FOLLOWING_LIMIT_WAR)) == -1) {
                perror("error follower limit");
                remove_client(&active_client, client->fd);
            }
            return 1;
        }
        
        aim_client->followers[i] = client;
        client->following[j] = aim_client;
        printf("%s is following %s\n", client->username, username);
        printf("%s has %s as a follower\n", username, client->username);
        return 0;
    }
}

/*
 * unfollow the user according to his/her username
 */
int unfollow(struct client *client, struct client *active_client, char *username) {
    //find the person that will be unfollowed.
    int a;
    for (a = 0; a < FOLLOW_LIMIT && client->following[a] != NULL && strcmp(client->following[a]->username, username) != 0; a++);

    //if there does not have the person
    if (a == FOLLOW_LIMIT || client->following[a] == NULL) {
        if (write(client->fd, INVALID_FOLLOW_NAME, strlen(INVALID_FOLLOW_NAME)) == -1) {
            perror("error invalid follow name");
            remove_client(&active_client, client->fd);
        }
        return 1;
    } else {
        struct client *aim_client = client->following[a];
        //find the place that client in the user's follower list.
        int i;
        for (i = 0; aim_client->followers[i]->fd != client->fd; i++);
        //find the olace that the user in client's following list.
        int j;
        for (j = 0; client->following[j]->fd != aim_client->fd; j++);

        //delete client from the follower list
        for (i = i; (i < FOLLOW_LIMIT - 1) && (aim_client->followers[i] != NULL); i++) {
                aim_client->followers[i] = aim_client->followers[i + 1];
            }
        aim_client->followers[i] = NULL;

        //delete the client from the list
        for (j = j; (j < FOLLOW_LIMIT - 1) && (client->following[j] != NULL); j++) {
            client->following[j] = client->following[j + 1];
        }
        client->following[j] = NULL;
        printf("%s no longer has %s as a follower\n", username, client->username);
        printf("%s unfollows %s\n", client->username, username);
        
        return 0;
    }
}

/*
 * Send all the followers of the client the message s.
 */
int send_message(struct client *client, char *s, struct client *active_clients) {
    // Find an empty message.
    int j;
    for (j = 0; j < MSG_LIMIT && client->message[j][0] != '\0'; j++);
    
    // if the message list is full, end user a warning.
    if (j == MSG_LIMIT) {
        if (write(client->fd, MSG_LIMIT_WAR, strlen(MSG_LIMIT_WAR)) == -1) {
            perror("error warning message");
            remove_client(&active_clients, client->fd);
        }
        return 1;
    } else {
        //store the message
        char stored_msg[strlen(s) + strlen(client->username) + 8];
        memset(stored_msg, '\0', sizeof(stored_msg));
        strncpy(stored_msg, client->username, strlen(client->username));
        strcat(stored_msg, " wrote: ");
        strncat(stored_msg, s, strlen(s));
        strcpy(client->message[j], stored_msg);
        strcat(client->message[j], "\r\n");
    }
    
    //tell the follower that the client send message
    for (int i = 0; i < FOLLOW_LIMIT && client->followers[i] != NULL; i++) {
        struct client *follower = client->followers[i];
        char received_msg[strlen(s) + strlen(client->username) + 4];
        memset(received_msg, '\0', sizeof(received_msg));
        strncpy(received_msg, client->username, strlen(client->username));
        strcat(received_msg, ": ");
        strncat(received_msg, s, strlen(s));
        strcat(received_msg, "\r\n");
        if (write(follower->fd, received_msg, strlen(received_msg)) == -1) {
            perror("error receiving message");
            remove_client(&active_clients, follower->fd);
        }
    }    
    return 0;   
}

/*
 * the function that show all messages that the person, which the client follows, sent
 */
void show(struct client *client, struct client *active_client) {
    for (int i = 0; i < FOLLOW_LIMIT && client->following[i] != NULL; i++) {
        struct client *following_client = client->following[i];
        for (int j = 0; j < MSG_LIMIT && following_client->message[j][0] != '\0'; j++) {
            if (write(client->fd, following_client->message[j], strlen(following_client->message[j])) == -1) {
                perror("error show message");
                remove_client(&active_client, client->fd);
            }
        }
    }
} 




int main (int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    // If the server writes to a socket that has been closed, the SIGPIPE
    // signal is sent and the process is terminated. To prevent the server
    // from terminating, ignore the SIGPIPE signal. 
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // A list of active clients (who have already entered their names). 
    struct client *active_clients = NULL;

    // A list of clients who have not yet entered their names. This list is
    // kept separate from the list of active clients, because until a client
    // has entered their name, they should not issue commands or 
    // or receive announcements. 
    struct client *new_clients = NULL;

    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, LISTEN_SIZE);

    // Initialize allset and add listenfd to the set of file descriptors
    // passed into select 
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            exit(1);
        } else if (nready == 0) {
            continue;
        }

        // check if a new client is connecting
        if (FD_ISSET(listenfd, &rset)) {
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd, &q);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_client(&new_clients, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if (write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, 
                    "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_client(&new_clients, clientfd);
            }
        }

        // Check which other socket descriptors have something ready to read.
        // The reason we iterate over the rset descriptors at the top level and
        // search through the two lists of clients each time is that it is
        // possible that a client will be removed in the middle of one of the
        // operations. This is also why we call break after handling the input.
        // If a client has been removed, the loop variables may no longer be 
        // valid.
        int cur_fd, handled;
        for (cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if (FD_ISSET(cur_fd, &rset)) {
                handled = 0;

                // Check if any new clients are entering their names
                for (p = new_clients; p != NULL; p = p->next) {
                    if (cur_fd == p->fd) {
                        // TODO: handle input from a new client who has not yet
                        // entered an acceptable name
                        if (read_from_client(cur_fd, p->inbuf) == 1) {
                            int test = 0;
                            //test whether input is empty. 
                            if (strcmp(p->inbuf, "") == 0) {
                                if (write(cur_fd, INVALID_NAME_WAR, strlen(INVALID_NAME_WAR)) == -1) {
                                    perror("invalid name");
                                    remove_client(&new_clients, p->fd);
                                }
                                test = 1;
                            }
                            //test whether a repeat username
                            struct client *temp = active_clients;
                            while (temp != NULL) {
                                if (strcmp(temp->username, p->inbuf) == 0) {
                                    if (write(cur_fd, INVALID_NAME_WAR, strlen(INVALID_NAME_WAR)) == -1) {
                                        perror("invalid name");
                                        remove_client(&new_clients, p->fd);
                                    }
                                    test = 1;
                                }
                                temp = temp->next;
                            }
                            //if not an empty string nor repeat username, then actiave it.
                            if (test == 0) {
                                strcpy(p->username, p->inbuf);
                                activate_client(p, &active_clients, &new_clients);
                            }

                        } else {
                        remove_client(&new_clients, p->fd);
                        break;
                        }
                        handled = 1;
                        break;
                    } 
                }

                if (!handled) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {
                            // TODO: handle input from an active client
                            if (read_from_client(cur_fd, p->inbuf) == 1) {
                                printf("%s: %s\n", p->username, p->inbuf);
                                //if the user input 'send' command
                                if (strstr(p->inbuf, SEND_MSG) == p->inbuf) {
                                    send_message(p, strstr(p->inbuf, " ") + 1, active_clients);
                                }
                                //if the user input 'follow' command.
                                else if (strstr(p->inbuf, FOLLOW_MSG) == p->inbuf) {
                                    if (strlen(p->inbuf) <= strlen(FOLLOW_MSG) + 1) {
                                        if (write(p->fd, INVALID_FOLLOW_NAME, strlen(INVALID_FOLLOW_NAME)) == -1) {
                                            perror("error invalid follow name");
                                            remove_client(&active_clients, p->fd);
                                        }
                                    } else {
                                        char *following_name = strstr(p->inbuf, " ") + 1;
                                        follow(p, active_clients, following_name);
                                    }
                                }
                                //if the user input 'unfollow' command.
                                else if (strstr(p->inbuf, UNFOLLOW_MSG) == p->inbuf) {
                                    if (strlen(p->inbuf) <= strlen(FOLLOW_MSG) + 1) {
                                        if (write(p->fd, INVALID_FOLLOW_NAME, strlen(INVALID_FOLLOW_NAME)) == -1) {
                                            perror("error invalid follow name");
                                            remove_client(&active_clients, p->fd);
                                        }
                                    } else {
                                        char *following_name = strstr(p->inbuf, " ") + 1;
                                        unfollow(p, active_clients, following_name);      
                                    }
                                }    
                                //if the user input 'show' command.
                                else if (strstr(p->inbuf, SHOW_MSG) == p->inbuf) {
                                    show(p, active_clients);   
                                }
                                //if the user input 'quit' command.
                                else if (strstr(p->inbuf, "quit") == p->inbuf) {

                                    remove_client(&active_clients, p->fd);

                                }
                                // if user input an invalid command.
                                else {
                                    printf("Invalid command\n");
                                    if (write(cur_fd, INVALID_COMMAND, strlen(INVALID_COMMAND)) == -1) {
                                        perror("invalid command");
                                        remove_client(&active_clients, p->fd);
                                    }
                                }

                            }else {
                                remove_client(&active_clients, p->fd);
                                break;
                            }

                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
