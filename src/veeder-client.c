#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_REQUEST 50

// Verifies that input string only has numbers, then returns string as integer.
int str_to_int(char* str, int base);

// Validates server response using the message and checksum.
int integrity_check(char* response);

int main(int argc, char **argv) {
    // Handling arguments passed to the command.
    if (argc != 3) {
        printf("Usage: %s <ip address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* host = argv[1];
    int   port = str_to_int(argv[2], 10);

    if (port < 0 || port > 65535) {
        printf("Port must be between 0 and 65535.\n");
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
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Attempting to establish connection...\n");

    // Establishing connection with socket.
    if (connect(socket_fd, (struct sockaddr *) &addr, addr_len) == -1) {
        printf("Connection failed.\n");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s on port %d!\n\n", host, port);

    for (;;) {
        char command[BUFFER_SIZE - 1] = { 0 };
        char send_buffer[BUFFER_SIZE] = { 0 };
        char recv_buffer[BUFFER_SIZE] = { 0 };

        // Represents start of header.
        send_buffer[0] = 0x01;

        // Prompt user for command, handle it, prepend start of header.
        printf("> ");
        fgets(command, sizeof(command), stdin);

        if (strlen(command) == 1) {
            continue;
        }

        if (strstr(command, "exit") != NULL) {
            break;
        }

        if (strstr(command, "help") != NULL) {
            printf("See https://github.com/eredden/Veeder-Root-TLS-Client for"
            " help.\n");
            continue;
        }

        command[strcspn(command, "\n")] = 0;
        strcat(send_buffer, command);

        // Sending command to server.
        if(send(socket_fd, send_buffer, strlen(send_buffer) + 1, 0) == -1) {
            printf("Failed to send command to server.\n");
            continue;
        }

        // Get response from server and print it out to the terminal.
        int   resp_size = BUFFER_SIZE * 2;
        char* resp_data = (char *) calloc(1, resp_size);

        if (resp_data == NULL) {
            printf("Failed to allocate memory for response buffer.\n");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        for (int requests = 0; requests < MAX_REQUEST; requests++) {
            int recv_length = recv(socket_fd, recv_buffer, BUFFER_SIZE, 0);

            if (recv_length == -1) {
                printf("\nReceiving response from server failed.\n");
                break;
            } 

            // Resize the recv_data buffer if it is too small to hold all data.
            if (strlen(resp_data) + strlen(recv_buffer) > resp_size) {
                resp_size = resp_size * 2;
                resp_data = (char *) realloc(resp_data, resp_size);

                if (resp_data == NULL) {
                    printf("Failed to allocate memory for response buffer.\n");
                    close(socket_fd);
                    exit(EXIT_FAILURE);
                }
            }

            // Add new data into the recv_data buffer for integrity check later.
            strncat(resp_data, recv_buffer, BUFFER_SIZE);
            printf("%s", recv_buffer);

            // End if the ETX is present.
            int contains_etx = 0;

            for (int i = 0; i < recv_length; i++) {
                if (recv_buffer[i] == (char) 0x03) {
                    contains_etx = 1;
                }
            }

            if (contains_etx == 1 || recv_length == 0) { 
                break; 
            } else { 
                sleep(1); 
            }
        }

        printf("\n");

        // Ensure that Display Format commands are not integrity checked.
        int is_command_format = command[0] == tolower(command[0]);

        if (is_command_format && integrity_check(resp_data) == 0) {
            printf("Integrity check failed due to an invalid checksum.\n");
        }

        free(resp_data);
    }

    printf("Terminating connection...\n");
    close(socket_fd);

    return EXIT_SUCCESS;
}

int str_to_int(char* str, int base) {
    if (base <= 10) {
        int str_length = strlen(str);

        // Iterate through characters in str and ensure they are digits.
        for (int i = 0; i < str_length; i++) {
            if (isdigit(str[i]) == 0) {
                printf("\"%s\" must contain only numbers.\n", str);
                exit(EXIT_FAILURE);
            }
        }
    }

    return strtol(str, NULL, base);
}

int integrity_check(char* response) {
    unsigned long int integrity_threshold = 0b10000000000000000;
    unsigned long int message = 0;

    // Verify that checksum is present and has the right length.
    char* separator = strstr(response, "&&");
    char  checksum_buffer[5];

    int expected_separator_length = 7;
    int expected_sum_bits = 16;

    if (separator == NULL) {
        return 0;
    }

    if (strlen(separator) != expected_separator_length) {
        return 0;
    }

    // Calculate 16-bit binary sum of all characters in the message.
    int checksum_index = strlen(response) - 5;

    for (int i = 0; i < checksum_index; i++) {
        message += (int) response[i];
    }

    message = message & 0xFFFF;
    message = (message) + (message >> expected_sum_bits);

    // Convert hexadecimal checksum number to integer.
    for (int i = 0; i < 4; i++) {
        checksum_buffer[i] = response[i + checksum_index];
    }

    int checksum = str_to_int(checksum_buffer, expected_sum_bits) & 0xFFFF;

    if (message + checksum == integrity_threshold) {
        return 1;
    }

    return 0;
}