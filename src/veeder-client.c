#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 64

/* Verifies that input string "str" only contains numbers, then returns 
   the integer representation of that string, e.g. "50" becomes 50. */
int strtoint(char* str) {
    for (int i = 0; i < strlen(str); i++) {
        if (isdigit(str[i]) == 0) {
            printf("\"%s\" must contain only numbers.\n", str);
            exit(EXIT_FAILURE);
        }
    }

    return strtol(str, NULL, 10);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("IP address, port number, and timeout in seconds "
            "must be supplied.\n");
        exit(EXIT_FAILURE);
    }

    char* host         = argv[1];
    char* port_char    = argv[2];
    char* timeout_char = argv[3];

    int port    = strtoint(port_char);
    int timeout = strtoint(timeout_char);

    if (port < 0 || port > 65535) {
        printf("Port must be between 0 and 65535.\n");
        exit(EXIT_FAILURE);
    }

    if (timeout < 0) {
        printf("Timeout cannot be less than 0 seconds.\n");
        exit(EXIT_FAILURE);
    }

    // Creating socket and verifying it was made successfully.
    int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socket_fd == -1) {
        perror("Failed to create socket.\n");
        exit(EXIT_FAILURE);
    }

    // Setting up address/port info to bind to the newly made socket.
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);

    socklen_t addr_len = sizeof(addr);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) < 1) {
        printf("Address %s could not be converted to network binary.\n", host);
        exit(EXIT_FAILURE);
    }

    printf("Attempting to establish connection...\n");

    // Establish connection with socket. 
    // TO-DO: Invalid host/port can cause program hang. Handle this.

    if (connect(socket_fd, (struct sockaddr *) &addr, addr_len) == -1) {
        printf("Connection failed.\n");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s on port %d!\n\n", host, port);

    const char soh = 0x01; // Must be appended to the front of all commands.

    for (;;) {
        char command[BUFFER_SIZE] = { 0 };
        char send_buffer[BUFFER_SIZE] = { 0 };
        char recv_buffer[BUFFER_SIZE] = { 0 };

        send_buffer[0] = soh;

        // Prompt user for command, store it, parse it, add SOH.
        printf("> ");
        fgets(command, sizeof(command), stdin);

        if (strstr(command, "exit") != NULL) {
            break;
        }

        command[strcspn(command, "\n")] = 0;
        strcat(send_buffer, command);

        // Sending command to server.
        if(send(socket_fd, send_buffer, strlen(send_buffer) + 1, 0) == -1) {
            printf("Failed to send command to server.\n");
            continue;
        }

        // Briefly wait to ensure a response comes before we check for it.
        sleep(timeout);

        // Get response from server and print it out to the terminal.
        // TO-DO: Validate checksum at the end of the response.
        for (;;) {
            int recv_length = recv(socket_fd, recv_buffer, BUFFER_SIZE, 0);

            if (recv_length == -1) {
                printf("Receiving response from server failed.\n");
                break;
            } 

            printf("%s", recv_buffer);

            if (recv_length < BUFFER_SIZE) {
                break;
            }
        }

        printf("\n");
    }

    printf("Terminating connection...\n");
    close(socket_fd);

    return EXIT_SUCCESS;
}