// Student 1: Benjamin Lee 22252344
// Student 2: Olivia Morrison 23176135
// Student 3: Min Thit 23375069
// Student 4: Johnson Che 23403302

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcpServer.h"

#define CHUNK_SIZE 1024

char* readHTMLFileToBuffer(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Allocate initial memory for buffer
    size_t buffer_size = CHUNK_SIZE;
    char* buffer = (char*)malloc(buffer_size * sizeof(char));
    if (buffer == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    // Initialize buffer
    buffer[0] = '\0';

    size_t total_bytes_read = 0;
    size_t bytes_read;

    // Read file in chunks
    while ((bytes_read = fread(buffer + total_bytes_read, sizeof(char), CHUNK_SIZE, file)) > 0) {
        total_bytes_read += bytes_read;

        // Expand buffer if necessary
        if (total_bytes_read >= buffer_size) {
            buffer_size += CHUNK_SIZE;
            buffer = (char*)realloc(buffer, buffer_size * sizeof(char));
            if (buffer == NULL) {
                perror("Memory reallocation failed");
                fclose(file);
                return NULL;
            }
        }
    }

    // Null-terminate the buffer
    buffer[total_bytes_read] = '\0';

    fclose(file);
    return buffer;
}
