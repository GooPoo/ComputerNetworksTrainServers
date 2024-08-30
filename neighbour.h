// Student 1: Benjamin Lee 22252344
// Student 2: Olivia Morrison 23176135
// Student 3: Min Thit 23375069
// Student 4: Johnson Che 23403302

// neighbour.h

#include <stdbool.h> 
#include <sys/select.h>

#ifndef NEIGHBOUR_H
#define NEIGHBOUR_H

typedef struct Neighbour {
    char *name;
    char *ip;
    char *port;
} Neighbour;

struct Messagequeue {
    int socket;
    char *response;
    bool waiting;
    char *destination;
    bool done;
};
extern int num_connections;

extern fd_set output_fds;
extern struct Messagequeue *message_queues;
extern fd_set fds_waiting;

extern int tcp_server_socket;
extern int udp_server_socket;
extern Neighbour **neighbours;
extern int num_neighbours;

Neighbour *createNeighbour(char *name, char *ip, char *port);
void handle_TCP_request(int);



#endif /* NEIGHBOUR_H */
