// Student 1: Benjamin Lee 22252344
// Student 2: Olivia Morrison 23176135
// Student 3: Min Thit 23375069
// Student 4: Johnson Che 23403302

// tcpServer.h

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "neighbour.h"

// Struct to store timetable data
struct SCHEDULE_DATA {
    char departure_time[6];
    char route_name[10];
    char departing_from[10];
    char arrival_time[6];
    char arrival_station[20];
};

// Array to store timetable data
extern struct SCHEDULE_DATA *sd;

// Number of schedule data elements in sd
extern int num_tt_lines;

void handle_TCP_request(int);
char* readHTMLFileToBuffer(const char* filename);


#endif /* TCPSERVER_H */
