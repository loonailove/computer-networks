// iperf_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT      5001
#define BUF_SIZE  (128 * 1024)   // 128 KB chunks

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <duration_sec>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ip  = argv[1];
    int duration    = atoi(argv[2]);

    // Conectare la server
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT),
    };
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); return EXIT_FAILURE;
    }
    printf("Connected to %s:%d, sending for %d seconds...\n", ip, PORT, duration);

    // Buffer umplut cu date
    char *buf = malloc(BUF_SIZE);
    memset(buf, 'A', BUF_SIZE);

    long long total_bytes = 0;
    time_t start = time(NULL);

    while (time(NULL) - start < duration) {
        ssize_t sent = send(sock, buf, BUF_SIZE, 0);
        if (sent < 0) { perror("send"); break; }
        total_bytes += sent;
    }

    double elapsed   = (double)(time(NULL) - start);
    double mbps      = (total_bytes * 8.0) / (elapsed * 1e6);

    printf("\n--- Results ---\n");
    printf("Sent:       %.2f MB\n", total_bytes / 1e6);
    printf("Duration:   %.1f s\n", elapsed);
    printf("Throughput: %.2f Mbps\n", mbps);

    free(buf);
    close(sock);
    return EXIT_SUCCESS;
}
