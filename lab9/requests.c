#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                           char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line    = calloc(LINELEN, sizeof(char));

    // Step 1: request line
    if (query_params != NULL)
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    else
        sprintf(line, "GET %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): cookies
    if (cookies != NULL && cookies_count > 0) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i < cookies_count - 1)
                strcat(line, "; ");
        }
        compute_message(message, line);
    }

    // Step 4: blank line — end of headers
    compute_message(message, "");

    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char *content_type,
                            char **body_data, int body_data_fields_count,
                            char **cookies, int cookies_count)
{
    char *message          = calloc(BUFLEN, sizeof(char));
    char *line             = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Step 1: request line
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: build body, then add Content-Type and Content-Length
    for (int i = 0; i < body_data_fields_count; i++) {
        strcat(body_data_buffer, body_data[i]);
        if (i < body_data_fields_count - 1)
            strcat(body_data_buffer, "&");
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    sprintf(line, "Content-Length: %zu", strlen(body_data_buffer));
    compute_message(message, line);

    // Step 4 (optional): cookies
    if (cookies != NULL && cookies_count > 0) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i < cookies_count - 1)
                strcat(line, "; ");
        }
        compute_message(message, line);
    }

    // Step 5: blank line — end of headers
    compute_message(message, "");

    // Step 6: body
    strcat(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}
