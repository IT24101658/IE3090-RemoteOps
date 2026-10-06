/* controller_658.c - RemoteOps Controller for IT24101658 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/stat.h>

#define DEFAULT_PORT 9410
#define BUF_SIZE     8192
#define MAX_LINE     4096

int tcp_fd = -1;
int monitor_udp_fd = -1;
volatile int monitor_running = 0;

ssize_t send_all(int fd, const void *buf, size_t len) {
    size_t sent = 0;
    const char *p = buf;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n <= 0) return n;
        sent += n;
    }
    return sent;
}

ssize_t recv_all(int fd, void *buf, size_t len) {
    size_t got = 0;
    char *p = buf;
    while (got < len) {
        ssize_t n = recv(fd, p + got, len - got, 0);
        if (n <= 0) return n;
        got += n;
    }
    return got;
}

ssize_t read_line(int fd, char *buf, size_t max) {
    size_t pos = 0;
    while (pos < max - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0) return n;
        if (c == '\n') {
            buf[pos] = '\0';
            return pos;
        }
        if (c != '\r') buf[pos++] = c;
    }
    buf[pos] = '\0';
    return pos;
}

void *udp_listener(void *arg) {
    int port = *(int *)arg;
    monitor_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (monitor_udp_fd < 0) return NULL;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(monitor_udp_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("udp bind");
        close(monitor_udp_fd);
        return NULL;
    }

    char buf[512];
    while (monitor_running) {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(monitor_udp_fd, buf, sizeof(buf)-1, 0,
                             (struct sockaddr *)&from, &fromlen);
        if (n > 0) {
            buf[n] = '\0';
            printf("\n[UDP MONITOR] %s\n> ", buf);
            fflush(stdout);
        }
    }
    close(monitor_udp_fd);
    return NULL;
}

int connect_to_agent(const char *host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(host);
        if (!he) {
            close(fd);
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

void print_help(void) {
    printf(
        "Commands:\n"
        "  AUTH <token>              - authenticate (token = OPS-1658)\n"
        "  SYSINFO                   - system info\n"
        "  LISTPROC                  - list processes\n"
        "  EXEC <name>               - DATE|UPTIME|DISKFREE|HOSTNAME|WHOAMI\n"
        "  PUT <localfile> <remotename>\n"
        "  GET <remotename> <localfile>\n"
        "  MONITOR START <udp_port>\n"
        "  MONITOR STOP\n"
        "  QUIT\n"
        "  help                      - this help\n"
    );
}

int main(int argc, char *argv[]) {
    const char *host = "127.0.0.1";
    int port = DEFAULT_PORT;
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    printf("Connecting to %s:%d ...\n", host, port);
    tcp_fd = connect_to_agent(host, port);
    if (tcp_fd < 0) {
        perror("connect");
        return 1;
    }
    printf("Connected. Type 'help' for commands.\n");

    char line[MAX_LINE];
    char resp[MAX_LINE];

    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (line[0] == '\0') continue;

        if (strcmp(line, "help") == 0) {
            print_help();
            continue;
        }

        /* ---------- PUT (special handling) ---------- */
        if (strncmp(line, "PUT ", 4) == 0) {
            char local[256], remote[256];
            if (sscanf(line + 4, "%255s %255s", local, remote) != 2) {
                printf("Usage: PUT <localfile> <remotename>\n");
                continue;
            }
            FILE *fp = fopen(local, "rb");
            if (!fp) {
                perror("fopen");
                continue;
            }
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "PUT %s %ld\n", remote, size);
            send_all(tcp_fd, cmd, strlen(cmd));

            char *buf = malloc(size);
            fread(buf, 1, size, fp);
            send_all(tcp_fd, buf, size);
            free(buf);
            fclose(fp);

            if (read_line(tcp_fd, resp, sizeof(resp)) > 0)
                printf("%s\n", resp);
            continue;
        }

        /* ---------- GET (special handling) ---------- */
        if (strncmp(line, "GET ", 4) == 0) {
            char remote[256], local[256];
            if (sscanf(line + 4, "%255s %255s", remote, local) != 2) {
                printf("Usage: GET <remotename> <localfile>\n");
                continue;
            }
            char cmd[300];
            snprintf(cmd, sizeof(cmd), "GET %s\n", remote);
            send_all(tcp_fd, cmd, strlen(cmd));

            if (read_line(tcp_fd, resp, sizeof(resp)) <= 0) {
                printf("No response\n");
                continue;
            }
            printf("%s\n", resp);

            if (strncmp(resp, "OK FILE_SEND ", 13) == 0) {
                char fname[256];
                long fsize = 0;
                /* format: OK FILE_SEND <name> <size> SID:3333 */
                sscanf(resp + 13, "%255s %ld", fname, &fsize);

                char *buf = malloc(fsize);
                if (recv_all(tcp_fd, buf, fsize) == fsize) {
                    FILE *fp = fopen(local, "wb");
                    if (fp) {
                        fwrite(buf, 1, fsize, fp);
                        fclose(fp);
                        printf("Saved %ld bytes to %s\n", fsize, local);
                    }
                }
                free(buf);
            }
            continue;
        }

        /* ---------- MONITOR START ---------- */
        if (strncmp(line, "MONITOR START ", 14) == 0) {
            int udp_port = atoi(line + 14);
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "MONITOR START %d\n", udp_port);
            send_all(tcp_fd, cmd, strlen(cmd));
            if (read_line(tcp_fd, resp, sizeof(resp)) > 0)
                printf("%s\n", resp);

            if (strncmp(resp, "OK MONITOR_STARTED", 18) == 0) {
                monitor_running = 1;
                pthread_t tid;
                int *pport = malloc(sizeof(int));
                *pport = udp_port;
                pthread_create(&tid, NULL, udp_listener, pport);
                pthread_detach(tid);
            }
            continue;
        }

        /* ---------- MONITOR STOP ---------- */
        if (strcmp(line, "MONITOR STOP") == 0) {
            send_all(tcp_fd, "MONITOR STOP\n", 13);
            if (read_line(tcp_fd, resp, sizeof(resp)) > 0)
                printf("%s\n", resp);
            monitor_running = 0;
            continue;
        }

        /* ordinary text command */
        char cmd[MAX_LINE];
        snprintf(cmd, sizeof(cmd), "%.4090s\n", line);
        send_all(tcp_fd, cmd, strlen(cmd));

        if (read_line(tcp_fd, resp, sizeof(resp)) > 0)
            printf("%s\n", resp);

        if (strcmp(line, "QUIT") == 0) break;
    }

    monitor_running = 0;
    if (tcp_fd >= 0) close(tcp_fd);
    return 0;
}
