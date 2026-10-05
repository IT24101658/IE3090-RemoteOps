#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9410

int main()
{
    int listenfd, connfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len;


    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket created successfully.\n");


    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);


    if (bind(listenfd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    printf("Bind successful.\n");


    if (listen(listenfd, 5) < 0)
    {
        perror("listen");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    printf("RemoteOps Agent listening on port %d...\n", PORT);


    client_len = sizeof(client_addr);

    connfd = accept(listenfd,
                    (struct sockaddr *)&client_addr,
                    &client_len);

    if (connfd < 0)
    {
        perror("accept");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    printf("Controller connected!\n");


    close(connfd);
    close(listenfd);

    return 0;
}
