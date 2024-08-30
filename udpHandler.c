// Student 1: Benjamin Lee 22252344
// Student 2: Olivia Morrison 23176135
// Student 3: Min Thit 23375069
// Student 4: Johnson Che 23403302

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "udpServer.h"
#include "tcpServer.h"
#include "neighbour.h"

#define MAX_BUFFER_SIZE 5000

// Function to compare time strings in HH:MM format
// Returns 1 if time1 > time2, -1 if time1 < time2, 0 if equal
int compareTimesTwo(const char *time1, const char *time2) {
    int hours1, hours2, minutes1, minutes2;

    sscanf(time1, "%d:%d", &hours1, &minutes1);
    sscanf(time2, "%d:%d", &hours2, &minutes2);

    if (hours1 > hours2)
        return 1;
    else if (hours1 < hours2)
        return -1;
    else {
        if (minutes1 > minutes2)
            return 1;
        else if (minutes1 < minutes2)
            return -1;
        else
            return 0;
    }
}



void handle_UDP_request(int udp_socket, socklen_t addr_size) {
    struct sockaddr_in client_addr;
    char buffer[4096];

    // Receive UDP packet
    ssize_t bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_size);
    if (bytes_received < 0) {
        perror("Error in receiving UDP data");
        exit(EXIT_FAILURE);
    }

    buffer[bytes_received] = '\0';

    // Convert client IP address to string
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    int client_port = ntohs(client_addr.sin_port);

    // Print the received UDP packet along with the source address
    printf("Received UDP packet from %s:%d: %s\n", client_ip, client_port, buffer);

    if (strstr(buffer, "My name is ") != NULL) {
        // Extract station name from the message
        char *station_name_start = strstr(buffer, "My name is ") + strlen("My name is ");
        char *station_name_end = strchr(station_name_start, '\0');
        char received_station_name[100]; // Assuming station name won't exceed 100 characters
        strncpy(received_station_name, station_name_start, station_name_end - station_name_start);
        received_station_name[station_name_end - station_name_start] = '\0';

        // Find the neighbour with matching IP address and port
        for (int i = 0; i < num_neighbours; i++) {
            
            if (atoi(neighbours[i]->port) == client_port) {
                // Update the neighbour's name
                free(neighbours[i]->name);
                neighbours[i]->name = malloc(strlen(received_station_name) + 1);
                if (neighbours[i]->name == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                strcpy(neighbours[i]->name, received_station_name);
                printf("Updated neighbour's name: %s\n", neighbours[i]->name);
                break; // Exit the loop after updating the name
            }
        }

    } 
    // UDP DATAGRAM SENDING OUT TO FIND THE GOAL STATION.
    else if (strstr(buffer, "GOAL&") != NULL) {

        // COMPLETED: Add ability to have more than one station data (so it has the ability to read and store more than one hop).
        // COMPLETED: Add ability to send UDP datagram to the next hop.
        // COMPLETED: Add ability to go back.
        // COMPLETED: Add visited nodes, so you dont send UDP datagrams to stations you have already been to through the route.
        // COMPLETED: Basic UDP strucutre, adding more data on, figuring out if we are at the End Goal and there is a valid route to it.
        // COMPLETE: SEND 'RETURNNOTVALID' and 'RETURNVALID' back one station.

        #define MAX_ROUTES 20 // Maximum number of routes you expect to parse

        char station_goal[20];

        // Define a structure to store route information
        struct route {
            char depart_time[10];
            char route_name[20];
            char source_station[20];
            char arrival_time[10];
            char arrival_station[20];
        };

        // Array to store parsed routes
        struct route routes[MAX_ROUTES];
        char visited[MAX_ROUTES][20];
        int num_routes = 0; // Counter for the number of parsed routes

        char buffer_copy[strlen(buffer) + 1];
        strcpy(buffer_copy, buffer);

        // Parse buffer3
        char *token = strtok(buffer_copy, "&");
        int j = 0;
        while (token != NULL) {
            // Skip the first token (station goal)
            if (j == 1) {
                strcpy(station_goal, token);
            }

            // Parse the route segment
            if (j > 1 && num_routes < MAX_ROUTES) {
                // Parse the route segment and store its components into the route structure
                sscanf(token, "%[^|]|%[^|]|%[^|]|%[^|]|%[^&]", 
                    routes[num_routes].depart_time, 
                    routes[num_routes].route_name, 
                    routes[num_routes].source_station, 
                    routes[num_routes].arrival_time, 
                    routes[num_routes].arrival_station);
                num_routes++; // Increment the number of parsed routes
            }

            // Move to the next token
            token = strtok(NULL, "&");
            j++; // Increment token counter
        }
        // Print parsed routes
        //for (int i = 0; i < num_routes; i++) {
        //    printf("Departure Time: %s\n", routes[i].depart_time);
        //    printf("Route Name: %s\n", routes[i].route_name);
        //    printf("Source Station: %s\n", routes[i].source_station);
        //    printf("Arrival Time: %s\n", routes[i].arrival_time);
        //    printf("Arrival Station: %s\n", routes[i].arrival_station);
        //    printf("\n");
        //}

        for (int i = 0; i < num_routes; i++) {
            strcpy(visited[i], routes[i].source_station);
        }
        // printf("Visted Stations:");
        // for (int i = 0; i < num_routes; i++){
        //     printf("%s,", visited[i]);
        // }
        printf("\n");
        char time[10];
        char terminal[20];
        strcpy(terminal, routes[num_routes - 1].arrival_station);
        strcpy(time, routes[num_routes - 1].arrival_time);
        printf("Final Arrival time: %s\n", time);
        printf("Station Goal:%s\n", station_goal);

        printf("The buffer:%s\n", buffer);

        char firstLeavingTrain[MAX_BUFFER_SIZE] = {0};
        char modified_str[MAX_BUFFER_SIZE] = {0};
        int found_station = 0;
        // Check if Station Goal is a neighbour, if it is, find the next departure time to that station.
        for (int i = 0; i < num_neighbours; i++) {
            if (strcmp(neighbours[i]->name, station_goal) == 0) {
                found_station = 1;
                int found_route = 0;
                for (int j = 0; j < num_tt_lines; j++) {
                    if (strcmp(sd[j].arrival_station, station_goal) == 0 ) {
                        if (compareTimesTwo(sd[j].departure_time, time) > 0) {
                            found_route = 1;
                            sprintf(firstLeavingTrain, "%s%s|%s|%s|%s|%s&", buffer, sd[j].departure_time, sd[j].route_name, station_name, sd[j].arrival_time, sd[j].arrival_station);

                            // REPLACE GOAL WITH RETURNVALID.
                            // A VALID ROUTE HAS BEEN FOUND!
                            char *pos = strstr(firstLeavingTrain, "GOAL");                            
                            if (pos != NULL) {
                                int before_length = pos - firstLeavingTrain;
                                strncpy(modified_str, firstLeavingTrain, before_length);
                                modified_str[before_length] = '\0';
                                strcat(modified_str, "RETURNVALID");
                                strcat(modified_str, pos + strlen("GOAL"));
                                //printf("Modified string: %s\n", modified_str);
                            } else {
                                //printf("String does not contain 'GOAL'.\n");
                                break;
                            }
                            int udp_datagram_sent = 0;
                            for (int n = 0; n < num_neighbours; n++) {
                                printf("the route:%s\n", routes[num_routes - 1].source_station);
                                if (strcmp(neighbours[n]->name, routes[num_routes - 1].source_station) == 0) {

                                    if (modified_str[0] != '\0' && !udp_datagram_sent){
                                        struct sockaddr_in dest_addr;
                                        memset(&dest_addr, 0, sizeof(dest_addr));
                                        dest_addr.sin_family = AF_INET;
                                        dest_addr.sin_port = htons(atoi(neighbours[n]->port));

                                        // if (inet_pton(AF_INET, neighbours[n]->ip, &dest_addr.sin_addr) <= 0) {
                                        //     fprintf(stderr, "Invalid address: %s\n", neighbours[n]->ip);
                                        //     continue;
                                        // }

                                        sendto(udp_server_socket, modified_str, strlen(modified_str), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

                                        printf("Sent RETURNVALID UDP datagram to %s:%s\n", neighbours[n]->ip, neighbours[n]->port);
                                        udp_datagram_sent = 1;
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    } 
                }
                if (!found_route) {
                    // If neighbour = Destination Station, but there is no valid train. Return NO route.
                    // COMPLETED: needs to be in the form of: "RETURNNOTVALID&".

                    char no_route_copy[strlen(buffer) + 1];
                    strcpy(no_route_copy, buffer);

                    // REPLACE GOAL WITH RETURNNOTVALID.
                    // THE ROUTE HAS BEEN FOUND TO HAVE NO 'VALID' DEPARTURE TIME!
                    // RETURN TO START SERVER SAYING NO GOOD ROUTE.
                    char *pos = strstr(no_route_copy, "GOAL");                            
                    if (pos != NULL) {
                        int before_length = pos - no_route_copy;
                        strncpy(modified_str, no_route_copy, before_length);
                        modified_str[before_length] = '\0';
                        strcat(modified_str, "RETURNNOTVALID");
                        strcat(modified_str, pos + strlen("GOAL"));
                        //printf("Modified string: %s\n", modified_str);
                    } else {
                        //printf("String does not contain 'GOAL'.\n");
                        break;
                    }
                    int udp_datagram_sent = 0;
                    for (int n = 0; n < num_neighbours; n++) {
                        printf("the route:%s\n", routes[num_routes - 1].source_station);
                        if (strcmp(neighbours[n]->name, routes[num_routes - 1].source_station) == 0) {

                            if (modified_str[0] != '\0' && !udp_datagram_sent){
                                struct sockaddr_in dest_addr;
                                memset(&dest_addr, 0, sizeof(dest_addr));
                                dest_addr.sin_family = AF_INET;
                                dest_addr.sin_port = htons(atoi(neighbours[n]->port));

                                // if (inet_pton(AF_INET, neighbours[n]->ip, &dest_addr.sin_addr) <= 0) {
                                //     fprintf(stderr, "Invalid address: %s\n", neighbours[n]->ip);
                                //     continue;
                                // }

                                sendto(udp_server_socket, modified_str, strlen(modified_str), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

                                printf("Sent RETURNNOTVALID UDP datagram to %s:%s\n", neighbours[n]->ip, neighbours[n]->port);
                                udp_datagram_sent = 1;
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }

        // Find a neighbour that has not been visited and attempting another UDP hop to find the `station_goal`.
        if (!found_station) {
            for (int i = 0; i < num_neighbours; i++) {
                for (int k = 0; k < num_routes; k++){
                    if (strcmp(neighbours[i]->name, visited[k]) == 0 ) {
                        break;
                    }
                    for (int j = 0; j < num_tt_lines; j++) {
                        if (strcmp(sd[j].arrival_station, neighbours[i]->name) == 0 ) {
                            if (compareTimesTwo(sd[j].departure_time, time) > 0) {
                                sprintf(firstLeavingTrain, "%s%s|%s|%s|%s|%s&", buffer, sd[j].departure_time, sd[j].route_name, station_name, sd[j].arrival_time, sd[j].arrival_station);
                                break;
                            }
                        }
                    }
                    
                    if (firstLeavingTrain[0] != '\0'){
                        struct sockaddr_in dest_addr;
                        memset(&dest_addr, 0, sizeof(dest_addr));
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = htons(atoi(neighbours[i]->port));

                        // if (inet_pton(AF_INET, neighbours[i]->ip, &dest_addr.sin_addr) <= 0) {
                        //     fprintf(stderr, "Invalid address: %s\n", neighbours[i]->ip);
                        //     continue;
                        // }

                        sendto(udp_server_socket, firstLeavingTrain, strlen(firstLeavingTrain), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

                        printf("Sent UDP datagram to %s:%s\n", neighbours[i]->ip, neighbours[i]->port);
                    }
                }
            }
        }
    } 
    // UDP DATAGRAM TO BE SENT BACK TO THE ORIGINAL SENDER, SAYING ROUTE IS VALID, AND WITH ALL THE ROUTE DETAILS.
    else if (strstr(buffer, "RETURNVALID&") != NULL) {

        // COMPLETE: SEND 'RETURNNOTVALID' and 'RETURNVALID' back one station.
        // COMPLETE: If station_name = visited[0] then post to HTTP/TCP.

        #define MAX_ROUTES 20 // Maximum number of routes you expect to parse

        char station_goal[20];

        // Define a structure to store route information
        struct route {
            char depart_time[10];
            char route_name[20];
            char source_station[20];
            char arrival_time[10];
            char arrival_station[20];
        };

        // Array to store parsed routes
        struct route routes[MAX_ROUTES];
        char visited[MAX_ROUTES][20];
        int num_routes = 0; // Counter for the number of parsed routes

        char buffer_copy[strlen(buffer) + 1];
        strcpy(buffer_copy, buffer);

        // Parse buffer3
        char *token = strtok(buffer_copy, "&");
        int j = 0;
        while (token != NULL) {
            // Skip the first token (station goal)
            if (j == 1) {
                strcpy(station_goal, token);
            }

            // Parse the route segment
            if (j > 1 && num_routes < MAX_ROUTES) {
                // Parse the route segment and store its components into the route structure
                sscanf(token, "%[^|]|%[^|]|%[^|]|%[^|]|%[^&]", 
                    routes[num_routes].depart_time, 
                    routes[num_routes].route_name, 
                    routes[num_routes].source_station, 
                    routes[num_routes].arrival_time, 
                    routes[num_routes].arrival_station);
                num_routes++; // Increment the number of parsed routes
            }

            // Move to the next token
            token = strtok(NULL, "&");
            j++; // Increment token counter
        }
        // Print parsed routes
        //for (int i = 0; i < num_routes; i++) {
        //    printf("Departure Time: %s\n", routes[i].depart_time);
        //    printf("Route Name: %s\n", routes[i].route_name);
        //    printf("Source Station: %s\n", routes[i].source_station);
        //    printf("Arrival Time: %s\n", routes[i].arrival_time);
        //    printf("Arrival Station: %s\n", routes[i].arrival_station);
        //    printf("\n");
        //}

        for (int i = 0; i < num_routes; i++) {
            strcpy(visited[i], routes[i].source_station);
        }
        //printf("Visted Stations:");
        // for (int i = 0; i < num_routes; i++){
        //     printf("%s,", visited[i]);
        // }
        //printf("\n");
        char time[10];
        char terminal[20];
        strcpy(terminal, routes[num_routes - 1].arrival_station);
        strcpy(time, routes[num_routes - 1].arrival_time);
        printf("Final Arrival time: %s\n", time);
        printf("Station Goal:%s\n", station_goal);

        printf("The buffer:%s\n", buffer);
        printf("Station Name:%s\n", station_name);
        printf("Visited:%s\n", visited[0]);


        if (strcmp(station_name, visited[0]) == 0) {
            char line[5000] = {0};
            char response[5000] = {0};
            // Post to HTTP/TCP
            // Replace this comment with your HTTP/TCP post code
            printf("Station %s is equal to visited[0], posting to HTTP/TCP...\n", station_name);
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
            
            sprintf(line, "%s", buffer);
            strcat(response, line);

            printf("RESPONSE FINAL: %s", response);
            for (int i = 0; i < num_connections; i++) {
                if (message_queues[i].waiting == true) {
                    message_queues[i].response = malloc(strlen(response) + 1);
                    if (message_queues[i].response == NULL) {
                        perror("Memory allocation error");
                        exit(EXIT_FAILURE);
                    }

                    ////////////////////////////////////////////////////////////////////////////////
                    // Define a structure to store route information
                    struct DisplayRoute {
                        char depart_time[10];
                        char route_name[20];
                        char source_station[20];
                        char arrival_time[10];
                        char arrival_station[20];
                    };

                    // Array to store parsed routes
                    struct DisplayRoute DisplayRoutes[MAX_ROUTES];
                    int num_Display_routes = 0; // Counter for the number of parsed routes

                    char response_copy[strlen(response) + 1];
                    strcpy(response_copy, response);

                    // Parse buffer3
                    char *token = strtok(response_copy, "&");
                    int j = 0;
                    while (token != NULL) {
                        // Skip the first token (station goal)
                        if (j == 1) {
                            strcpy(station_goal, token);
                        }

                        // Parse the route segment
                        if (j > 1 && num_Display_routes < MAX_ROUTES) {
                            // Parse the route segment and store its components into the route structure
                            sscanf(token, "%[^|]|%[^|]|%[^|]|%[^|]|%[^&]", 
                                DisplayRoutes[num_Display_routes].depart_time, 
                                DisplayRoutes[num_Display_routes].route_name, 
                                DisplayRoutes[num_Display_routes].source_station, 
                                DisplayRoutes[num_Display_routes].arrival_time, 
                                DisplayRoutes[num_Display_routes].arrival_station);
                            num_Display_routes++; // Increment the number of parsed routes
                        }

                        // Move to the next token
                        token = strtok(NULL, "&");
                        j++; // Increment token counter
                    }
                    char display[2048] = {0};
                    sprintf(display, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
                    // Print parsed routes
                    for (int i = 0; i < num_Display_routes; i++) {
                        printf("Departure Time: %s\n", DisplayRoutes[i].depart_time);
                        printf("Route Name: %s\n", DisplayRoutes[i].route_name);
                        printf("Source Station: %s\n", DisplayRoutes[i].source_station);
                        printf("Arrival Time: %s\n", DisplayRoutes[i].arrival_time);
                        printf("Arrival Station: %s\n", DisplayRoutes[i].arrival_station);
                        printf("\n");
                        //Departure Time: , Route: busA_C, Departing From: stopA, Arrival Time: 08:24, Arrival Station: BusportC 
                        sprintf(display + strlen(display),  "Departure Time: %s, Route: %s, Departing From: %s, Arrival Time: %s, Arrival Station: %s <br>", DisplayRoutes[i].depart_time, DisplayRoutes[i].route_name, DisplayRoutes[i].source_station, DisplayRoutes[i].arrival_time, DisplayRoutes[i].arrival_station);
                    }

                    printf("Final Arrival time: %s\n", DisplayRoutes[num_Display_routes - 1].arrival_time);

                    message_queues[i].response = malloc(strlen(display) + 1);
                    if (message_queues[i].response == NULL) {
                        perror("Memory allocation error");
                        exit(EXIT_FAILURE);
                    }

                    // Copy response into message_queues[i].response
                    strcpy(message_queues[i].response, display);
                    printf("RESPONSE:%s", message_queues[i].response);
                    message_queues[i].waiting = false;
                    printf("\n\n NOT WAITING\n\n");
                    break;
                }
            }

        } else {
            // Iterate through visited to find station_name
            for (int i = 1; i < num_routes; i++) {
                if (strcmp(station_name, visited[i]) == 0) {
                    // Send UDP datagram to visited[i-1]
                    int udp_datagram_sent = 0;
                    for (int n = 0; n < num_neighbours; n++) {
                        printf("the route:%s\n", visited[i - 1]);
                        if (strcmp(neighbours[n]->name, visited[i - 1]) == 0) {

                            if (buffer[0] != '\0' && !udp_datagram_sent){
                                struct sockaddr_in dest_addr;
                                memset(&dest_addr, 0, sizeof(dest_addr));
                                dest_addr.sin_family = AF_INET;
                                dest_addr.sin_port = htons(atoi(neighbours[n]->port));

                                // if (inet_pton(AF_INET, neighbours[n]->ip, &dest_addr.sin_addr) <= 0) {
                                //     fprintf(stderr, "Invalid address: %s\n", neighbours[n]->ip);
                                //     continue;
                                // }

                                sendto(udp_server_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

                                printf("Sent RETURNVALID UDP datagram to %s:%s\n", neighbours[n]->ip, neighbours[n]->port);
                                udp_datagram_sent = 1;
                            }
                            break;
                        }
                    }

                    break;
                }
            }
        }

    
    } 
    // UDP DATAGRAM TO BE SENT BACK TO THE ORIGINAL SENDER, SAYING NOT A VALID ROUTE.
    // BASICALLY WHEN YOU FIND THE GOAL, BUT THE ARRIVAL TIME IS NOT TODAY.
    else if (strstr(buffer, "RETURNNOTVALID&") != NULL) {

        // COMPLETE: SEND 'RETURNNOTVALID' and 'RETURNVALID' back one station.
        // COMPLETE: If station_name = source_station[0][20] then post to HTTP/TCP.

        #define MAX_ROUTES 20 // Maximum number of routes you expect to parse

        char station_goal[20];

        // Define a structure to store route information
        struct route {
            char depart_time[10];
            char route_name[20];
            char source_station[20];
            char arrival_time[10];
            char arrival_station[20];
        };

        // Array to store parsed routes
        struct route routes[MAX_ROUTES];
        char visited[MAX_ROUTES][20];
        int num_routes = 0; // Counter for the number of parsed routes

        char buffer_copy[strlen(buffer) + 1];
        strcpy(buffer_copy, buffer);

        // Parse buffer3
        char *token = strtok(buffer_copy, "&");
        int j = 0;
        while (token != NULL) {
            // Skip the first token (station goal)
            if (j == 1) {
                strcpy(station_goal, token);
            }

            // Parse the route segment
            if (j > 1 && num_routes < MAX_ROUTES) {
                // Parse the route segment and store its components into the route structure
                sscanf(token, "%[^|]|%[^|]|%[^|]|%[^|]|%[^&]", 
                    routes[num_routes].depart_time, 
                    routes[num_routes].route_name, 
                    routes[num_routes].source_station, 
                    routes[num_routes].arrival_time, 
                    routes[num_routes].arrival_station);
                num_routes++; // Increment the number of parsed routes
            }

            // Move to the next token
            token = strtok(NULL, "&");
            j++; // Increment token counter
        }
        // Print parsed routes
        //for (int i = 0; i < num_routes; i++) {
        //    printf("Departure Time: %s\n", routes[i].depart_time);
        //    printf("Route Name: %s\n", routes[i].route_name);
        //    printf("Source Station: %s\n", routes[i].source_station);
        //    printf("Arrival Time: %s\n", routes[i].arrival_time);
        //    printf("Arrival Station: %s\n", routes[i].arrival_station);
        //    printf("\n");
        //}

        for (int i = 0; i < num_routes; i++) {
            strcpy(visited[i], routes[i].source_station);
        }
        // printf("Visted Stations:");
        // for (int i = 0; i < num_routes; i++){
        //     printf("%s,", visited[i]);
        // }
        // printf("\n");
        char time[10];
        char terminal[20];
        strcpy(terminal, routes[num_routes - 1].arrival_station);
        strcpy(time, routes[num_routes - 1].arrival_time);
        printf("Final Arrival time: %s\n", time);
        printf("Station Goal:%s\n", station_goal);

        printf("The buffer:%s\n", buffer);
        
        printf("The buffer:%s\n", buffer);
        printf("Station Name:%s\n", station_name);
        printf("Visited:%s\n", visited[0]);


        if (strcmp(station_name, visited[0]) == 0) {
            char line[5000] = {0};
            char response[5000] = {0};
            // Post to HTTP/TCP
            // Replace this comment with your HTTP/TCP post code
            printf("Station %s is equal to visited[0], posting to HTTP/TCP...\n", station_name);
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
            
            sprintf(line, "%s", buffer);
            strcat(response, line);

            printf("RESPONSE FINAL: %s", response);
            printf("RESPONSE FINAL: %s", response);
            for (int i = 0; i < num_connections; i++) {
                if (message_queues[i].waiting == true) {
                    message_queues[i].response = malloc(strlen(response) + 1);
                    if (message_queues[i].response == NULL) {
                        perror("Memory allocation error");
                        exit(EXIT_FAILURE);
                    }

                    ////////////////////////////////////////////////////////////////////////////////
                    // Define a structure to store route information
                    struct DisplayRoute {
                        char depart_time[10];
                        char route_name[20];
                        char source_station[20];
                        char arrival_time[10];
                        char arrival_station[20];
                    };

                    // Array to store parsed routes
                    struct DisplayRoute DisplayRoutes[MAX_ROUTES];
                    int num_Display_routes = 0; // Counter for the number of parsed routes

                    char response_copy[strlen(response) + 1];
                    strcpy(response_copy, response);

                    // Parse buffer3
                    char *token = strtok(response_copy, "&");
                    int j = 0;
                    while (token != NULL) {
                        // Skip the first token (station goal)
                        if (j == 1) {
                            strcpy(station_goal, token);
                        }

                        // Parse the route segment
                        if (j > 1 && num_Display_routes < MAX_ROUTES) {
                            // Parse the route segment and store its components into the route structure
                            sscanf(token, "%[^|]|%[^|]|%[^|]|%[^|]|%[^&]", 
                                DisplayRoutes[num_Display_routes].depart_time, 
                                DisplayRoutes[num_Display_routes].route_name, 
                                DisplayRoutes[num_Display_routes].source_station, 
                                DisplayRoutes[num_Display_routes].arrival_time, 
                                DisplayRoutes[num_Display_routes].arrival_station);
                            num_Display_routes++; // Increment the number of parsed routes
                        }

                        // Move to the next token
                        token = strtok(NULL, "&");
                        j++; // Increment token counter
                    }
                    char display[2048] = {0};
                    sprintf(display, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
                    sprintf(display + strlen(display), "Route Not Time Valid <br>");
                    // Print parsed routes
                    for (int i = 0; i < num_Display_routes; i++) {
                        printf("Departure Time: %s\n", DisplayRoutes[i].depart_time);
                        printf("Route Name: %s\n", DisplayRoutes[i].route_name);
                        printf("Source Station: %s\n", DisplayRoutes[i].source_station);
                        printf("Arrival Time: %s\n", DisplayRoutes[i].arrival_time);
                        printf("Arrival Station: %s\n", DisplayRoutes[i].arrival_station);
                        printf("\n");
                        //Departure Time: , Route: busA_C, Departing From: stopA, Arrival Time: 08:24, Arrival Station: BusportC 
                        sprintf(display + strlen(display),  "Departure Time: %s, Route: %s, Departing From: %s, Arrival Time: %s, Arrival Station: %s <br>", DisplayRoutes[i].depart_time, DisplayRoutes[i].route_name, DisplayRoutes[i].source_station, DisplayRoutes[i].arrival_time, DisplayRoutes[i].arrival_station);
                    }

                    printf("Final Arrival time: %s\n", DisplayRoutes[num_Display_routes - 1].arrival_time);

                    message_queues[i].response = malloc(strlen(display) + 1);
                    if (message_queues[i].response == NULL) {
                        perror("Memory allocation error");
                        exit(EXIT_FAILURE);
                    }

                    // Copy response into message_queues[i].response
                    strcpy(message_queues[i].response, display);
                    printf("RESPONSE:%s", message_queues[i].response);
                    message_queues[i].waiting = false;
                    printf("\n\n NOT WAITING\n\n");
                    break;
                }
            }


            //send(tcp_server_socket, response, strlen(response), 0);
        } else {
            // Iterate through visited to find station_name
            for (int i = 1; i < num_routes; i++) {
                if (strcmp(station_name, visited[i]) == 0) {
                    // Send UDP datagram to visited[i-1]
                    int udp_datagram_sent = 0;
                    for (int n = 0; n < num_neighbours; n++) {
                        printf("the route:%s\n", visited[i - 1]);
                        if (strcmp(neighbours[n]->name, visited[i - 1]) == 0) {

                            if (buffer[0] != '\0' && !udp_datagram_sent){
                                struct sockaddr_in dest_addr;
                                memset(&dest_addr, 0, sizeof(dest_addr));
                                dest_addr.sin_family = AF_INET;
                                dest_addr.sin_port = htons(atoi(neighbours[n]->port));

                                // if (inet_pton(AF_INET, neighbours[n]->ip, &dest_addr.sin_addr) <= 0) {
                                //     fprintf(stderr, "Invalid address: %s\n", neighbours[n]->ip);
                                //     continue;
                                // }

                                sendto(udp_server_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

                                printf("Sent RETURNNOTVALID UDP datagram to %s:%s\n", neighbours[n]->ip, neighbours[n]->port);
                                udp_datagram_sent = 1;
                            }
                            break;
                        }
                    }

                    break;
                }
            }
        }
    
    }
}