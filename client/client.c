#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8088
#define BUFFER_SIZE 100

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];
    
    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Connect to localhost

    // 3. Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 4. Prepare and send message
    srand(time(NULL));
    int rand_num = rand() % 1000;
    snprintf(send_buf, BUFFER_SIZE, "Hello from client %d", rand_num);

    if (send(sock, send_buf, strlen(send_buf), 0) == -1) {
        perror("send failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 5. Receive server acknowledgment
    int bytes_received = recv(sock, recv_buf, BUFFER_SIZE - 1, 0);
    if (bytes_received == -1) {
        perror("recv failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    recv_buf[bytes_received] = '\0';  // Null-terminate received string
    printf("Received from server: %s\n", recv_buf);

    // 6. Clean up
    close(sock);
    return 0;
}
