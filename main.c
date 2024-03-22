#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

/* Leave as is to run the server and get files from the same dir. */
#define PROJECT_DIR "."
void spawn_client_thread(int server_socket);
void *handle_client(void *client);
FILE *get_resource(char *requested_resource);
void process_request(char *request, char *response);

int main() {
    /* The server will run continuosly, and will spawn a thread for each
     * client request, thus each thread will be responsible for serving
     * the client, handling errors, closing the conection and terminating
     * the thread, this way the server will be able to server multiple
     * clients simultaneously. */
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in socket_address;
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(8080);
    socket_address.sin_addr.s_addr = htonl(0x7f000001);

    if (bind(server_socket, (struct sockaddr *)&socket_address, sizeof(socket_address))) {
        return EXIT_FAILURE;
    }

    listen(server_socket, 256);
    printf("server listening at http://127.0.0.1:8080\n");

    while (true) {
        int client = accept(server_socket, NULL, NULL);
        spawn_client_thread(client);
    }
    return EXIT_SUCCESS;
}

void spawn_client_thread(int client) {
    pthread_t thread = malloc(sizeof *thread);
    int err = pthread_create(&thread, NULL, handle_client, &client);
    if (err) {
        recv(client, 0, 0, 0);
        send(client, "HTTP/1.1 500 Internal Server Error\r\n\n", 256, 0);
        printf("err = %d\n", err);
        return;
    }
    pthread_detach(thread);
}

void *handle_client(void *client) {
    int handled_client = *(int *)client;

    char *request = calloc(1024, sizeof *request);
    char *response = calloc(8192, sizeof *response);
    recv(handled_client, request, 1024, 0);
    process_request(request, response);
    send(handled_client, response, 8192, 0);
    free(response);
    free(request);
    request = calloc(1024, sizeof *request);
    response = calloc(8192, sizeof *response);
    close(handled_client);

    return 0;
}

void process_request(char *request, char *response) {
    char *pReader = calloc(1024, sizeof *pReader);
    strcpy(pReader, request);
    if (strncmp(pReader, "GET ", 4)) {
        strcat(response, "HTTP/1.1 405 Method Not Allowed\r\n\n");
        free(pReader);
        return;
    }
    pReader += 4;

    char *requested_resource = calloc(1024, sizeof *requested_resource);
    int i;
    for (i = 0; i < 1024 && *pReader != ' '; i++, pReader++) {
        requested_resource[i] = *pReader;
    }

    FILE *resource = get_resource(requested_resource);
    if (!resource) {
        strcat(response, "HTTP/1.1 404 Not Found\r\n\n");
        free(requested_resource);
        return;
    }

    strcat(response, "HTTP/1.1 200 OK\r\n\n");
    char buffer[1024];
    while (fgets(buffer, sizeof buffer, resource)) {
        strcat(response, buffer);
    }
    free(requested_resource);
    fclose(resource);
    return;
}

FILE *get_resource(char *requested_resource) {
    char resource_path[1024] = PROJECT_DIR;
    if (!strncmp(requested_resource, "/\0", 2)) {
        strcat(resource_path, "/index.html");
        return fopen(resource_path, "r");
    }
    int request_lenght = strlen(requested_resource);

    if (!strcmp(requested_resource + request_lenght - 4, ".css")) {
        strcat(resource_path, "/css/styles.css");
    } else if (!strcmp(requested_resource + request_lenght - 3, ".ico")) {
        strcat(resource_path, "/favicon.ico");
    } else if (!strcmp(requested_resource + request_lenght - 3, ".js")) {
        strcat(resource_path, requested_resource);
    } else {
        strcat(resource_path, requested_resource);
        strcat(resource_path, ".html");
    }
    return fopen(resource_path, "r");
}
