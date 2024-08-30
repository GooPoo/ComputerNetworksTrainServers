// Student 1: Benjamin Lee 22252344
// Student 2: Olivia Morrison 23176135
// Student 3: Min Thit 23375069
// Student 4: Johnson Che 23403302

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <arpa/inet.h>


#include "tcpServer.h"
#include "udpServer.h"
#include "neighbour.h"

#define MAXLINE 1024
#define MAX_NEIGHBOURS 20

int TCP_PORT;
int UDP_PORT;
char *station_name;
Neighbour **neighbours;
int num_neighbours = 0;

int tcp_server_socket;
int udp_server_socket;

fd_set read_fds;             // set of file descriptors monitored for reading.

fd_set output_fds;
struct Messagequeue *message_queues;
int num_connections;
fd_set fds_waiting;
// Queue **queues;

int FD_MAX;                  // maximum file descriptor number
int N_READY;                 // number of ready sockets

char buffer[MAXLINE];        // buffer for incoming messages
ssize_t msg_size;

struct SCHEDULE_DATA *sd;
int num_tt_lines;

struct sockaddr_in tcp_server_addr, tcp_client_addr;
struct sockaddr_in udp_server_addr, udp_client_addr;
socklen_t tcp_addr_size, udp_addr_size;

// Read the timetable information for the Station and Stores it into a strucutre. 
void readTimetable() {

    const char *prefix = "tt-";
    char *filename = malloc(strlen(prefix) + strlen(station_name) + 1);
    if (filename == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    strcpy(filename, prefix);
    strcat(filename, station_name);

    FILE *fp;
    char line[MAXLINE];
    int linecount = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error in opening file");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line), fp) && linecount < 2) {
        linecount++;
    }

    sd = (struct SCHEDULE_DATA*) malloc(sizeof(struct SCHEDULE_DATA));
    if (sd == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    num_tt_lines = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';

        num_tt_lines++;

        sd = (struct SCHEDULE_DATA*)realloc(sd, (sizeof(struct SCHEDULE_DATA) * num_tt_lines));
        if (sd == NULL) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }

        struct SCHEDULE_DATA current; 
        char *token;
        char comma[] = ",";
        int token_index = 0;

        token = strtok(line, comma);

        while (token != NULL) {
            switch (token_index) {
            case 0:
                strcpy(current.departure_time, token);
                break;
            case 1:
                strcpy(current.route_name, token);
                break;
            case 2:
                strcpy(current.departing_from, token);
                break;
            case 3:
                strcpy(current.arrival_time, token);
                break;
            case 4:
                strcpy(current.arrival_station, token);
            default:
                break;
            }
            token = strtok(NULL, comma);
            token_index++;
        }
        sd[num_tt_lines-1] = current;
    }

    fclose(fp);
}


// Function to send UDP datagram for 'Station name Handshake' at start of program.
void send_UDP_datagram_on_startup(Neighbour **neighbours, int num_neighbours, int udp_server_socket) { 
    for (int i = 0; i < num_neighbours; i++) {
        if (strcmp(neighbours[i]->name, "Unknown") == 0) {
            printf("Found an unknown station name:\n");
            struct sockaddr_in dest_addr;
            memset(&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(atoi(neighbours[i]->port));

            // if (inet_pton(AF_INET, neighbours[i]->ip, &dest_addr.sin_addr) <= 0) {
            //     fprintf(stderr, "Invalid address: %s\n", neighbours[i]->ip);
            //     continue;
            // }

            char udp_message[100];
            sprintf(udp_message, "My name is %s", station_name);
            sendto(udp_server_socket, udp_message, strlen(udp_message), 0, (const struct sockaddr*)&dest_addr, sizeof(dest_addr));

            printf("Sent UDP datagram to %s:%s\n", neighbours[i]->ip, neighbours[i]->port);
        }
    }
}

// Main function
int main(int argc, char *argv[]) {
    // Argument Handlers
    if (argc < 4) {
        fprintf(stderr, "Usage: %s station-name browser-port query-port neighbour1 [neighbour2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    station_name = argv[1];
    TCP_PORT = atoi(argv[2]);
    UDP_PORT = atoi(argv[3]);

    printf("\n----------------------------------\n");
    printf("Station Name: %s\n", station_name);
    printf("TCP Port: %d\n", TCP_PORT);
    printf("UDP Port: %d\n", UDP_PORT);

    if (argc >= 5) {
        neighbours = malloc(MAX_NEIGHBOURS * sizeof(Neighbour *));
        if (neighbours == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        for (int i = 4; i < argc; i++) {
            if (strcmp(argv[i], "&") == 0) {
                break;
            }
            char *ip_port = argv[i];
            char *colon_ptr = strchr(ip_port, ':');
            if (colon_ptr == NULL) {
                fprintf(stderr, "Invalid neighbour format: %s\n", ip_port);
                exit(EXIT_FAILURE);
            }

            char *ip = malloc(colon_ptr - ip_port + 1);
            if (ip == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
            strncpy(ip, ip_port, colon_ptr - ip_port);
            ip[colon_ptr - ip_port] = '\0';

            char *port = malloc(strlen(colon_ptr + 1) + 1);
            if (port == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
            strcpy(port, colon_ptr + 1);

            if (num_neighbours < MAX_NEIGHBOURS) {
                neighbours[num_neighbours++] = createNeighbour("Unknown", ip, port);
            } else {
                fprintf(stderr, "Exceeded maximum number of neighbours (%d)\n", MAX_NEIGHBOURS);
                exit(EXIT_FAILURE);
            }
            free(ip);
        }
    } else {
        printf("No neighbours provided.\n");
    }


    printf("----------------------------------\n");
    readTimetable();

    int yes = 1;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TCP SOCKET CREATION
    memset(buffer, 0, sizeof(buffer));
    tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_server_socket < 0) {
        perror("Error in TCP socket creation");
        exit(EXIT_FAILURE);
    }

    memset(&tcp_server_addr, 0, sizeof(tcp_server_addr));
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_server_addr.sin_port = htons(TCP_PORT);

    setsockopt(tcp_server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(tcp_server_socket, (struct sockaddr*)&tcp_server_addr, sizeof(tcp_server_addr)) < 0) {
        perror("Error in binding TCP socket");
        exit(EXIT_FAILURE);
    }

    if (listen(tcp_server_socket, 5) < 0) {
        perror("Error in listening for TCP connections");
        exit(EXIT_FAILURE);
    }

    printf("TCP server listening on port %d\n", TCP_PORT);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // UDP SOCKET CREATION
    udp_server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_server_socket < 0) {
        perror("Error in UDP socket creation");
        exit(EXIT_FAILURE);
    }

    memset(&udp_server_addr, 0, sizeof(udp_server_addr));
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_addr.s_addr = INADDR_ANY;
    udp_server_addr.sin_port = htons(UDP_PORT);

    setsockopt(udp_server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(udp_server_socket, (struct sockaddr*)&udp_server_addr, sizeof(udp_server_addr)) < 0) {
        perror("Error in binding UDP socket");
        exit(EXIT_FAILURE);
    }

    printf("UDP Server bound to port %d\n", UDP_PORT);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    FD_ZERO(&read_fds);
    FD_ZERO(&output_fds);
    FD_ZERO(&fds_waiting);

    // Allocate memory for Messagequeue 
    message_queues = (struct Messagequeue*) malloc(sizeof(struct Messagequeue));
    if (message_queues == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    num_connections = 0;


    if (tcp_server_socket < udp_server_socket) {
        FD_MAX = udp_server_socket + 1;
    } else {
        FD_MAX = tcp_server_socket + 1;
    }

    // Pause Program so it guarentees all stations/servers are up and running for initial 'Station name Handshake'
    sleep(5);
    send_UDP_datagram_on_startup(neighbours, num_neighbours, udp_server_socket);

    // Use select to handle both TCP and UDP sockets
    for (;;) {
        // Sets the sockets to set for reading
        FD_SET(tcp_server_socket, &read_fds);
        FD_SET(udp_server_socket, &read_fds);

        // use select on a ready socket.
        if ((N_READY = select(FD_MAX, &read_fds, &output_fds, NULL, NULL)) < 0) {
            perror("Error in select");
            exit(EXIT_FAILURE);   
        } 

        if (FD_ISSET(tcp_server_socket, &read_fds)) {

            // Accept new connection request 
            tcp_addr_size = sizeof(tcp_client_addr);
            int tcp_client_socket = accept(tcp_server_socket, (struct sockaddr*)&tcp_client_addr, &tcp_addr_size);
            if (tcp_client_socket < 0) {
                perror("Error in accepting TCP connection");
                exit(EXIT_FAILURE);
            }

            if (FD_MAX < tcp_client_socket) {
                FD_MAX = tcp_client_socket + 1;
            }

            printf("CLIENT SOCKET: %i\n", tcp_client_socket);

            // Make a new element in message queues
            num_connections++;
            size_t msgqsize = sizeof(struct Messagequeue);
            message_queues = (struct Messagequeue*) realloc(message_queues, (msgqsize * num_connections));
            if (message_queues == NULL) {
                perror("realloc failed");
                exit(EXIT_FAILURE);
            }
            
            struct Messagequeue new_connection;
            new_connection.socket = tcp_client_socket;
            new_connection.response= NULL;
            new_connection.waiting = false;
            new_connection.destination = NULL;
            new_connection.done = false;

            message_queues[num_connections-1] = new_connection;

            // printf("New connection: %i\n", tcp_client_socket);

            FD_SET(tcp_client_socket, &read_fds);
        }

        if (FD_ISSET(udp_server_socket, &read_fds)) {
            udp_addr_size = sizeof(udp_client_addr);
            handle_UDP_request(udp_server_socket, udp_addr_size);
        }

        // printf("num connections: %i\n", num_connections);
        for (int i = 0; i < num_connections; i++) {
            // printf("num connections loop: %i\n", i);
            // printf("message_queues[%i].socket: %i\n", i, message_queues[i].socket);

            if (FD_ISSET(message_queues[i].socket, &read_fds)) {

                // printf("FD_ISSET(message_queues[i].socket, &read_fds)\n");

                FD_SET(message_queues[i].socket, &output_fds);
                    
                // if (FD_ISSET(message_queues[i].socket, &output_fds)) {
                //     printf("OUTPUT FDS IS SET WITH I\n");
                // } else {
                //     printf("OUTPUT FDS NOT SET WITH I\n");
                // }
                
                // printf("TCP Connection established.\n");
                handle_TCP_request(message_queues[i].socket);
                // printf("TCP Connection handled.\n");
            }

            if (FD_ISSET(message_queues[i].socket, &output_fds)) {
                // printf("FD_ISSET(message_queues[i].socket, &output_fds)\n");

                if (message_queues[i].waiting == true) {
                    // printf("Breaking\n");
                    FD_SET(message_queues[i].socket, &output_fds);
                    //continue;
                    break;
                } else {

                    // //message_queues[i].response = "hello";

                    // printf("Entering\n");
                    // char *msg = malloc(strlen(message_queues[i].response) + 1);
                    // if (msg  == NULL) {
                    //     perror("Memory allocation error");
                    //     exit(EXIT_FAILURE);
                    // }
                    // strcpy(msg, message_queues[i].response);

                    // // Send to TCP client
                    // size_t bytes_sent = send(message_queues[i].socket, msg, strlen(msg), 0);
                    // if (bytes_sent < 0) {
                    //     perror("failed to send to client");
                    // } else {
                    //     // if (FD_ISSET(i, &output_fds)) {
                    //     //     FD_CLR(i, &output_fds);
                    //     // } 
                    //     // Set to done - indicating the connection should be closed (?)
                        message_queues[i].done = true;

                    //     printf("%zu bytes sent\n", bytes_sent);
                    // }
                    // printf("%d\n", message_queues[i].socket);
                    // free(msg);

                }
            }

            if (message_queues[i].done == true) {
                // close the connection 
                // printf("Closing the connection: %d\n", message_queues[i].socket);
                // printf("CLOSING WAITING MESSAGE: %d\n", message_queues[i].waiting);
                // printf("CLOSING REPONSE MESSAGE: %s\n", message_queues[i].response);
                printf("problem here?!");
                char *msg = malloc(strlen(message_queues[i].response) + 1);
                if (msg  == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                strcpy(msg, message_queues[i].response);

                // Send to TCP client
                size_t bytes_sent = send(message_queues[i].socket, msg, strlen(msg), 0);
                if (bytes_sent < 0) {
                    perror("failed to send to client");
                } else {
                    // if (FD_ISSET(i, &output_fds)) {
                    //     FD_CLR(i, &output_fds);
                    // } 
                    // Set to done - indicating the connection should be closed (?)
                    message_queues[i].done = true;

                    printf("%zu bytes sent\n", bytes_sent);
                }
                // printf("%d\n", message_queues[i].socket);
                free(msg);
                if (FD_ISSET(message_queues[i].socket, &read_fds)) {
                    FD_CLR(message_queues[i].socket, &read_fds);
                }
                if (FD_ISSET(message_queues[i].socket, &output_fds)) {
                    FD_CLR(message_queues[i].socket, &output_fds);
                } 
                shutdown(message_queues[i].socket, SHUT_RDWR);
                close(message_queues[i].socket);
                continue;
            }
        }
    }

    printf("done\n");

    free(sd);

    shutdown(tcp_server_socket, SHUT_RDWR);
    close(tcp_server_socket);
    shutdown(udp_server_socket, SHUT_RDWR);
    close(udp_server_socket);

    return 0;

}