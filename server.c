#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <string.h>

// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mswsock.h>
#include "fs.h"
// clang-format on

#define PORT 7000
#define BUF_SIZE 8192

/* Request-Line = Method SP Request-URI SP HTTP-Version CRLF */
typedef struct {
    char* method;
    char* uri;
    char* version;
} http_request_line;

typedef enum {
    HTTP_RES_OK,
    HTTP_RES_BAD_REQUEST,
    HTTP_RES_INTERNAL_SERVER_ERROR
} http_result;

static http_request_line http_request_line_init(void) {
    http_request_line line;
    line.method = NULL;
    line.uri = NULL;
    line.version = NULL;
    return line;
}

static http_result parse_req_line(http_request_line* req_line,
                                  char* buf,
                                  size_t len) {
    if (req_line == NULL || buf == NULL) {
        return HTTP_RES_INTERNAL_SERVER_ERROR;
    }
    if (len == 0) {
        return HTTP_RES_BAD_REQUEST;
    }

    char* method = buf;
    char* sp1 = strchr(method, ' ');
    if (sp1 == NULL) {
        return HTTP_RES_BAD_REQUEST;
    }
    *sp1 = '\0';

    char* uri = sp1 + 1;
    char* sp2 = strchr(uri, ' ');
    if (sp2 == NULL) {
        return HTTP_RES_BAD_REQUEST;
    }
    *sp2 = '\0';

    char* version = sp2 + 1;
    char* cr = strpbrk(version, "\r\n");
    if (cr != NULL) {
        *cr = '\0';
    }

    req_line->method = method;
    req_line->uri = uri;
    req_line->version = version;
    return HTTP_RES_OK;
}

static void send_response(SOCKET client_socket,
                          const char* status,
                          const char* body) {
    char response[512];
    int len = snprintf(response,
                       sizeof(response),
                       "HTTP/1.0 %s\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: %d\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "%s",
                       status,
                       (int)strlen(body),
                       body);
    if (len > 0) {
        (void)send(client_socket, response, len, 0);
    }
}

static void send_file(SOCKET client_socket,
                      const char* path) {
    file_handle file;
    if (fs_open(path, &file) != 0) {
        send_response(
            client_socket, "404 Not Found", "<h1>404 Not Found</h1>");
        return;
    }

    char header[256];
    int len = snprintf(header,
                       sizeof(header),
                       "HTTP/1.0 200 OK\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: %lld\r\n"
                       "Connection: close\r\n"
                       "\r\n",
                       file.size);

    TRANSMIT_FILE_BUFFERS buffers = {0};
    buffers.Head = header;
    buffers.HeadLength = (DWORD)len;

    if (!TransmitFile(
            client_socket, file.handle, 0, 0, NULL, &buffers, 0)) {
        (void)fprintf(
            stderr, "TransmitFile failed: %lu\n", GetLastError());
    }

    fs_close(&file);
}

/* Routing */

typedef void (*route_handler)(SOCKET client_socket);

typedef struct {
    const char* path;
    route_handler handler;
} route;

static void route_root(SOCKET client_socket) {
    send_response(client_socket, "200 OK", "<h1>Hello!</h1>");
}

static void route_about(SOCKET client_socket) {
    send_response(client_socket, "200 OK", "<h1>About</h1>");
}

static void route_file(SOCKET client_socket) {
    send_file(client_socket, "index.html");
}

static const route ROUTES[] = {
    {"/", route_root},
    {"/about", route_about},
    {"/file", route_file},
};

static void route_request(SOCKET client_socket,
                          const char* uri) {
    size_t path_len = strcspn(uri, "?");

    for (size_t i = 0; i < sizeof(ROUTES) / sizeof(ROUTES[0]); i++) {
        if (strlen(ROUTES[i].path) == path_len &&
            strncmp(uri, ROUTES[i].path, path_len) == 0) {
            ROUTES[i].handler(client_socket);
            return;
        }
    }

    send_response(
        client_socket, "404 Not Found", "<h1>404 Not Found</h1>");
}

/*  Server  */
static int handle_client(SOCKET client_socket) {
    char buf[BUF_SIZE] = {0};
    int total = 0;

    while (total < (int)sizeof(buf) - 1) {
        int n = recv(client_socket,
                     buf + total,
                     (int)sizeof(buf) - 1 - total,
                     0);
        if (n <= 0) {
            break;
        }
        total += n;
        if (strstr(buf, "\r\n\r\n") != NULL) {
            break;
        }
    }

    http_request_line req_line = http_request_line_init();
    http_result res = parse_req_line(&req_line, buf, (size_t)total);

    if (res == HTTP_RES_BAD_REQUEST) {
        send_response(client_socket,
                      "400 Bad Request",
                      "<h1>400 Bad Request</h1>");
        return 0;
    }
    if (res != HTTP_RES_OK) {
        send_response(client_socket,
                      "500 Internal Server Error",
                      "<h1>500 Internal Server Error</h1>");
        return 0;
    }

    printf("%s %s %s\n",
           req_line.method,
           req_line.uri,
           req_line.version);
    route_request(client_socket, req_line.uri);

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
