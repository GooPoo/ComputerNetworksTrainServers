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

#include "neighbour.h"

Neighbour *createNeighbour(char *name, char *ip, char *port) {
    Neighbour *neighbour = malloc(sizeof(Neighbour));
    if (neighbour == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    neighbour->name = malloc(strlen(name) + 1);
    strcpy(neighbour->name, name);
    neighbour->ip = malloc(strlen(ip) + 1);
    strcpy(neighbour->ip, ip);
    neighbour->port = malloc(strlen(port) + 1);
    strcpy(neighbour->port, port);
    return neighbour;
}
