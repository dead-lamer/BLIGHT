#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zlib.h>
#include <stdint.h>

#define MAX_CLIENTS 10
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT "4444"

const char SECRET_DISCONNECT_KEY[] = "DEAD_LAMER_MADE_THIS_SHIT";

uint32_t calculate_disconnect_hash() { // uint32_t
    return crc32(0L, (const Bytef*)SECRET_DISCONNECT_KEY, strlen(SECRET_DISCONNECT_KEY));
}

int main(int argc, char *argv[]) {
    const char *host = DEFAULT_HOST;
    const char *service = DEFAULT_PORT;

    if (argc == 3) {
        host = argv[1];
        service = argv[2];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [<bind_host> <port>]\n", argv[0]);
        return 1;
    }

    struct addrinfo hints = {0};
    struct addrinfo *result;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(host, service, &hints, &result);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    int sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sfd == -1) {
        perror("socket");
        freeaddrinfo(result);
        return 1;
    }

    if (bind(sfd, result->ai_addr, result->ai_addrlen) == -1) {
        perror("bind");
        close(sfd);
        freeaddrinfo(result);
        return 1;
    }

    if (listen(sfd, SOMAXCONN) == -1) {
        perror("listen");
        close(sfd);
        freeaddrinfo(result);
        return 1;
    }

    freeaddrinfo(result);

    int client_sockets[MAX_CLIENTS] = {0};
    fd_set readfds;

    void send_to_clients(const char *cmd, int *client_indices, int count, uint32_t disc_hash) { // uint32_t
        if (strcmp(cmd, "disconnect") == 0) {
             uint32_t network_disc_hash = htonl(disc_hash);
             for (int j = 0; j < count; j++) {
                 int i = client_indices[j];
                 if (i >= 0 && i < MAX_CLIENTS && client_sockets[i] > 0) {
                     write(client_sockets[i], &network_disc_hash, sizeof(network_disc_hash));
                     printf("Sent disconnect signal to Client %d.\n", i);
                 }
             }
        } else {
            for (int j = 0; j < count; j++) {
                int i = client_indices[j];
                if (i >= 0 && i < MAX_CLIENTS && client_sockets[i] > 0) {
                    write(client_sockets[i], cmd, strlen(cmd) + 1);
                }
            }
        }
    }

    uint32_t disconnect_hash = calculate_disconnect_hash(); // uint32_t

    printf("Server listening on %s:%s...\n", host, service);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int max_fd = sfd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &readfds);
                if (client_sockets[i] > max_fd) max_fd = client_sockets[i];
            }
        }

        if (select(max_fd+1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(sfd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);
            int new_socket = accept(sfd, (struct sockaddr*)&client_addr, &len);
            if (new_socket < 0) {
                perror("accept");
                continue;
            }

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("Client %d (%s) connected\n", i, inet_ntoa(client_addr.sin_addr));
                    break;
                }
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char cmd[1024];
            int group[MAX_CLIENTS];
            int group_count = 0;

            if (!fgets(cmd, sizeof(cmd), stdin)) break;
            cmd[strcspn(cmd, "\n")] = '\0';

            if (strncmp(cmd, "cmb", 3) == 0) {
                if (cmd[3] != ' ' && cmd[3] != '\0') {
                    printf("Invalid command format for 'cmb'. Use 'cmb' or 'cmb <id1> <id2> ...'\n");
                } else {
                    char *rest = cmd + 3;
                    while (*rest == ' ') rest++;

                    char *token = strtok(rest, " ");
                    while (token != NULL && group_count < MAX_CLIENTS) {
                        char *endptr;
                        long val = strtol(token, &endptr, 10);
                        if (*endptr == '\0' && val >= 0 && val < MAX_CLIENTS) {
                            group[group_count++] = (int)val;
                        } else {
                             printf("Invalid client ID: %s\n", token);
                             group_count = 0;
                             break;
                        }
                        token = strtok(NULL, " ");
                    }

                    if(group_count > 0) {
                        printf("Group created. Number of clients: %d\n", group_count);
                        printf("Clients: ");
                        for (int i = 0; i < group_count; i++) printf("%d ", group[i]);
                        printf("\n");

                        if (!fgets(cmd, sizeof(cmd), stdin)) break;
                        cmd[strcspn(cmd, "\n")] = '\0';
                        send_to_clients(cmd, group, group_count, disconnect_hash);
                    } else {
                         printf("Failed to create group.\n");
                    }
                }
            } else {
                int all_connected_indices[MAX_CLIENTS];
                int all_count = 0;
                for(int i = 0; i < MAX_CLIENTS; i++) {
                    if(client_sockets[i] > 0) {
                        all_connected_indices[all_count++] = i;
                    }
                }
                send_to_clients(cmd, all_connected_indices, all_count, disconnect_hash);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = client_sockets[i];
            if (sock > 0 && FD_ISSET(sock, &readfds)) {
                int network_exit_status;
                int bytes_read = read(sock, &network_exit_status, sizeof(network_exit_status));
                if (bytes_read <= 0) {
                    printf("Client %d disconnected\n", i);
                    close(sock);
                    client_sockets[i] = 0;
                    continue;
                }
                int exit_status = ntohl(network_exit_status);

                int network_output_len;
                bytes_read = read(sock, &network_output_len, sizeof(network_output_len));
                if (bytes_read <= 0) {
                    printf("Client %d disconnected during output length read\n", i);
                    close(sock);
                    client_sockets[i] = 0;
                    continue;
                }
                int output_len = ntohl(network_output_len);

                if (output_len < 0 || output_len > 1024) {
                     printf("Client %d sent invalid output length: %d\n", i, output_len);
                     close(sock);
                     client_sockets[i] = 0;
                     continue;
                }

                char output_buffer[1025] = {0};
                if (output_len > 0) {
                    int total_read = 0;
                    while (total_read < output_len) {
                        int chunk = read(sock, output_buffer + total_read, output_len - total_read);
                        if (chunk <= 0) {
                            printf("Client %d disconnected during output data read\n", i);
                            close(sock);
                            client_sockets[i] = 0;
                            total_read = output_len + 1;
                            break;
                        }
                        total_read += chunk;
                    }
                    if (total_read > output_len) continue;
                }

                printf("Client %d response (exit code: %d):\n%s\n", i, exit_status, output_buffer);
            }
        }
    }

    close(sfd);
    return 0;
}
