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
#include <ctype.h>

#include "tcpServer.h"
#include "neighbour.h"
#include "udpServer.h"

// Function to compare time strings in HH:MM format
// Returns 1 if time1 > time2, -1 if time1 < time2, 0 if equal
int compareTimes(const char *time1, const char *time2) {
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

void handle_TCP_request(int client_socket) {

    //printf("in TCP Handler\n");
    char buffer[4096] = {0};
    char response[4096] = {0};
    char payload_buffer[4096] = {0};
    char* htmlContent = NULL;

    ssize_t total_bytes_received = 0;
    ssize_t bytes_received;
    do {
        bytes_received = recv(client_socket, buffer + total_bytes_received, sizeof(buffer) - total_bytes_received, 0);
        if (bytes_received < 0) {
            perror("Error in receiving data");
            return;
        }
        total_bytes_received += bytes_received;
    } while (bytes_received > 0 && !strstr(buffer, "\r\n\r\n"));

    /// WHAT SHOULD WE DO I DO HERE? with the close()
    if (bytes_received == 0) {
        close(client_socket);
        return;
    }

    buffer[total_bytes_received] = '\0';

    // Find the start of the payload data
    char* payload_start = strstr(buffer, "\r\n\r\n");
    if (payload_start != NULL) {
        payload_start += 4;
        size_t payload_length = strlen(payload_start);
        strncpy(payload_buffer, payload_start, payload_length);
        payload_buffer[payload_length] = '\0';
    }

    char* method = strtok(buffer, " ");
    if (method == NULL) {
        printf("Invalid HTTP request\n");
        return;
    }

    char* path = strtok(NULL, " ");
    if (path == NULL) {
        printf("Invalid HTTP request\n");
        return;
    }

    // Check requested path
    if (strcmp(path, "/") == 0) {
        // Welcome page
        const char* filename = "index.html";
        char* htmlContent = readHTMLFileToBuffer(filename);
        if (htmlContent == NULL) {
            printf("Failed to read HTML file.\n");
            sprintf(response, "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\n<html><head><title> 500 Internal Server Error </title></head><body><h1> 500 Internal Server Error</h1><p> Failed to read HTML file. </p></body></html>");
            return;
        } else {
            char* position = strstr(htmlContent, "<span id=\"station_name\"></span>");
            if (position != NULL) {
                int station_name_length = strlen(station_name);
                int placeholder_length = strlen("<span id=\"station_name\"></span>");
                int new_html_length = strlen(htmlContent) - placeholder_length + station_name_length;
                char* new_htmlContent = malloc(new_html_length + 1);
                strncpy(new_htmlContent, htmlContent, position - htmlContent);
                strcpy(new_htmlContent + (position - htmlContent), station_name);
                strcpy(new_htmlContent + (position - htmlContent) + station_name_length, position + placeholder_length);
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n%s", new_htmlContent);
                free(new_htmlContent);
                // printf("TRYING TO ADD PATH TO REPONSE\n");
                // printf("NumConnections:%d\n", num_connections);
            } else {
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n%s", htmlContent);
            }
        }
        for (int i = 0; i < num_connections; i++) {
            printf("%d\n", message_queues[i].socket);
            printf("%d\n", client_socket);
            if (message_queues[i].socket == client_socket) {
                message_queues[i].response = malloc(strlen(response) + 1);
                if (message_queues[i].response == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                // Copy response into message_queues[i].response
                strcpy(message_queues[i].response, response);
                //printf("RESPONSE:%s", message_queues[i].response);
            }
        }
    } else if (strcmp(path, "/about") == 0) {
        // About page
        const char* filename = "about.html";
        char* htmlContent = readHTMLFileToBuffer(filename);
        if (htmlContent == NULL) {
            printf("Failed to read HTML file.\n");
            sprintf(response, "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\n<html><head><title> 500 Internal Server Error </title></head><body><h1> 500 Internal Server Error</h1><p> Failed to read HTML file. </p></body></html>");
            return;
        } else {
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n%s", htmlContent);
        }
        for (int i = 0; i < num_connections; i++) {
            printf("%d\n", message_queues[i].socket);
            printf("%d\n", client_socket);
            if (message_queues[i].socket == client_socket) {
                message_queues[i].response = malloc(strlen(response) + 1);
                if (message_queues[i].response == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                // Copy response into message_queues[i].response
                strcpy(message_queues[i].response, response);
                //printf("RESPONSE:%s", message_queues[i].response);
            }
        }
    } else if (strcmp(path, "/findJourneyResult") == 0) {
        char stationPreset[100];
        char time[100];
        char payload_copy[4096];
        strcpy(payload_copy, payload_buffer);
        char *position_station = strstr(payload_copy, "stationPreset=");
        if (position_station != NULL) {
            position_station += strlen("stationPreset=");
            char *end_position_station = strchr(position_station, '&');
            if (end_position_station != NULL) {
                int length = end_position_station - position_station;
                strncpy(stationPreset, position_station, length);
                stationPreset[length] = '\0';
            }
        }

        char *position_time = strstr(payload_copy, "time=");
        if (position_time != NULL) {
            position_time += strlen("time=");
            char *end_position_time = strchr(position_time, '&');
            if (end_position_time != NULL) {
                int length = end_position_time - position_time;
                strncpy(time, position_time, length);
                time[length] = '\0';
            } else {
                strcpy(time, position_time);
            }
        }
        printf("%s\n", stationPreset);
        printf("%s\n", time);
        char line[200] = {0};
        int found_station = 0;

        for (int i = 0; i < num_neighbours; i++) {
            if (strcmp(neighbours[i]->name, stationPreset) == 0) {
                found_station = 1;
                for (int j = 0; j < num_tt_lines; j++) {
                    if (strcmp(sd[j].arrival_station, stationPreset) == 0 ) {
                        if (compareTimes(sd[j].departure_time, time) > 0) {
                            sprintf(line, "Departure Time: %s, Route: %s, Departing From: %s, Arrival Time: %s, Arrival Station: %s <br>", sd[j].departure_time, sd[j].route_name, sd[j].departing_from, sd[j].arrival_time, sd[j].arrival_station);
                            strcat(response, line);
                            break;
                        }
                    }
                }
                break;
            }
        }

        // HAVE TO ADD LOGIC TO WHEN YOUR END STATION IS NOT A NEIGHBOUR, AND YOUR 'LEAVE AFTER' IS HIGHER THAN THE LATEST DEPARTURE TIME OF THAT NEIGHBOUR ITS TRYING TO HOP THROUGH. 
        // AND LOGIC FOR TRYING TO SET END STATION AS YOUR CURRENT STATION SERVER.
        if (!found_station) {
            //sprintf(line, "Need to make hops to find a Route.");
            for (int i = 0; i < num_neighbours; i++) {
                char firstLeavingTrain[250] = {0};
                for (int j = 0; j < num_tt_lines; j++) {
                    if (strcmp(sd[j].arrival_station, neighbours[i]->name) == 0 ) {
                        if (compareTimes(sd[j].departure_time, time) > 0) {
                            sprintf(firstLeavingTrain, "GOAL&%s&%s|%s|%s|%s|%s&", stationPreset, sd[j].departure_time, sd[j].route_name, station_name, sd[j].arrival_time, sd[j].arrival_station);
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

                    for (int i = 0; i < num_connections; i++) {
                        if (message_queues[i].socket == client_socket) {
                            message_queues[i].waiting = true;
                            // printf("\n\nWAITING\n\n");
                        }
                    }

                    sendto(udp_server_socket, firstLeavingTrain, strlen(firstLeavingTrain), 0, (const struct sockaddr*)&dest_addr, sizeof(dest_addr));

                    printf("Sent UDP datagram to %s:%s\n", neighbours[i]->ip, neighbours[i]->port);
                }
            }
            //strcat(response, line);
        }
        for (int i = 0; i < num_connections; i++) {
            // printf("%d\n", message_queues[i].socket);
            // printf("%d\n", client_socket);
            if (message_queues[i].socket == client_socket) {
                message_queues[i].response = malloc(strlen(response) + 1);
                if (message_queues[i].response == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                // Copy response into message_queues[i].response
                strcpy(message_queues[i].response, response);
                // printf("RESPONSE:%s", message_queues[i].response);
            }
        }
    } else if (strcmp(path, "/info-neighbours") == 0) {
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");

        char line[200] = {0};
        for (int i = 0; i < num_neighbours; i++) { 
            sprintf(line, "Name: %s, IP: %s, Port: %s <br>", neighbours[i]->name, neighbours[i]->ip, neighbours[i]->port);
            strcat(response, line);
        }
        for (int i = 0; i < num_connections; i++) {
            // printf("%d\n", message_queues[i].socket);
            // printf("%d\n", client_socket);
            if (message_queues[i].socket == client_socket) {
                message_queues[i].response = malloc(strlen(response) + 1);
                if (message_queues[i].response == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                // Copy response into message_queues[i].response
                strcpy(message_queues[i].response, response);
                // printf("RESPONSE:%s", message_queues[i].response);
            }
        }

    } else if (strcmp(path, "/timetable-data") == 0) {
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
        
        char line[200] = {0};
        for (int i = 0; i < num_tt_lines; i++) { 
            sprintf(line, "Departure Time: %s, Route: %s, Departing From: %s, Arrival Time: %s, Arrival Station: %s <br>", sd[i].departure_time, sd[i].route_name, sd[i].departing_from, sd[i].arrival_time, sd[i].arrival_station);
            strcat(response, line);
        }
        for (int i = 0; i < num_connections; i++) {
            printf("%d\n", message_queues[i].socket);
            printf("%d\n", client_socket);
            if (message_queues[i].socket == client_socket) {
                message_queues[i].response = malloc(strlen(response) + 1);
                if (message_queues[i].response == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                // Copy response into message_queues[i].response
                strcpy(message_queues[i].response, response);
                printf("RESPONSE:%s", message_queues[i].response);
            }
        }

    } else if (strcmp(path, "/close-sockets") == 0) {
        printf("Closing %s Sockets\n", station_name);
        shutdown(tcp_server_socket, SHUT_RDWR);
        close(tcp_server_socket);
        shutdown(udp_server_socket, SHUT_RDWR);
        close(udp_server_socket);
        exit(0);
    } else {
        sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><head><title> 404 Not Found </title></head><body><h1> 404 Not Found</h1><p> The requested URL was not found on this server. </p></body></html>");
        for (int i = 0; i < num_connections; i++) {
            // printf("%d\n", message_queues[i].socket);
            // printf("%d\n", client_socket);
            if (message_queues[i].socket == client_socket) {
                message_queues[i].response = malloc(strlen(response) + 1);
                if (message_queues[i].response == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                // Copy response into message_queues[i].response
                strcpy(message_queues[i].response, response);
                //printf("RESPONSE:%s", message_queues[i].response);
            }
        }
    }

    if (htmlContent != NULL) {
        free(htmlContent);
    }
}