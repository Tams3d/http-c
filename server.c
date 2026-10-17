#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define PORT 7000
#define BUF_SIZE 100

static int handle_client(SOCKET client_socket) {
    char buf[BUF_SIZE] = {0};
    int n = recv(client_socket, buf, sizeof(buf) - 1, 0);
    if (n == SOCKET_ERROR) {
        (void)fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
        return -1;
    }
    if (n > 0) {
        buf[n] = '\0';
        printf("received: %s\n", buf);
        
    }
    return 0;
}

static SOCKET create_listen_socket(void) {
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET) {
        (void)fprintf(
            stderr, "socket failed: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_socket,
             (struct sockaddr*)&address,
             sizeof(address)) == SOCKET_ERROR) {
        (void)fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        (void)fprintf(
            stderr, "listen failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }

    return listen_socket;
}

int main(void) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return 1;
    }

    SOCKET listen_socket = create_listen_socket();
    if (listen_socket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    printf("listening on port %d\n", PORT);

    while (1) {
        SOCKET client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            (void)fprintf(
                stderr, "accept failed: %d\n", WSAGetLastError());
            break;
        }

        printf("client connected\n");
        (void)handle_client(client_socket);
        closesocket(client_socket);
    }

    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
