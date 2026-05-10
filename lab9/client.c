#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

#define MAIN_SERVER_IP   "3.248.203.233"
#define MAIN_SERVER_PORT  8080
#define OWM_HOST         "api.openweathermap.org"
#define OWM_PORT          80

static char *resolve_host(const char *hostname)
{
    struct hostent *he = gethostbyname(hostname);
    if (!he) {
        fprintf(stderr, "Nu pot rezolva hostname-ul: %s\n", hostname);
        exit(1);
    }
    return strdup(inet_ntoa(*(struct in_addr *)he->h_addr_list[0]));
}

static char *extract_cookie(const char *response)
{
    const char *marker = "Set-Cookie: ";
    char *start = strstr(response, marker);
    if (!start) return NULL;
    start += strlen(marker);
    char *end = start;
    while (*end && *end != ';' && *end != '\r' && *end != '\n')
        end++;
    int len = (int)(end - start);
    char *cookie = calloc(len + 1, 1);
    memcpy(cookie, start, len);
    return cookie;
}

static const char *extract_body(const char *response)
{
    const char *sep = strstr(response, "\r\n\r\n");
    if (!sep) return NULL;
    return sep + 4;
}

/* Extrage valoarea unui camp numeric/string dintr-un JSON flat.
   Ex: extract_json_field(json, "lat") -> "44.43" */
static void extract_json_field(const char *json, const char *field,
                                char *out, int out_size)
{
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\":", field);
    const char *p = strstr(json, needle);
    if (!p) return;
    p += strlen(needle);
    /* sari spatii */
    while (*p == ' ') p++;
    /* daca e string, sari ghilimelele */
    int is_string = (*p == '"');
    if (is_string) p++;
    const char *end = p;
    while (*end && *end != (is_string ? '"' : ',') && *end != '}' && *end != '\n')
        end++;
    int len = (int)(end - p);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, p, len);
    out[len] = '\0';
}

int main(void)
{
    char *message;
    char *response;
    int   sockfd;

    char *owm_ip = resolve_host(OWM_HOST);
    printf("OWM IP rezolvat: %s\n\n", owm_ip);

    /* ============================================================== */
    /* Ex 1.1 - GET /api/v1/dummy                                      */
    /* ============================================================== */
    printf("=== Ex 1.1: GET dummy ===\n");
    sockfd   = open_connection(MAIN_SERVER_IP, MAIN_SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    message  = compute_get_request(MAIN_SERVER_IP, "/api/v1/dummy", NULL, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);
    free(message); free(response);
    close_connection(sockfd);

    /* ============================================================== */
    /* Ex 1.2 - POST /api/v1/dummy                                     */
    /* ============================================================== */
    printf("=== Ex 1.2: POST dummy ===\n");
    sockfd = open_connection(MAIN_SERVER_IP, MAIN_SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    char *dummy_fields[] = {"field1=value1", "field2=value2"};
    message = compute_post_request(
        MAIN_SERVER_IP, "/api/v1/dummy",
        "application/x-www-form-urlencoded",
        dummy_fields, 2, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);
    free(message); free(response);
    close_connection(sockfd);

    /* ============================================================== */
    /* Ex 2 - POST /api/v1/auth/login                                  */
    /* ============================================================== */
    printf("=== Ex 2: Login ===\n");
    sockfd = open_connection(MAIN_SERVER_IP, MAIN_SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    char *login_fields[] = {"username=student", "password=student"};
    message = compute_post_request(
        MAIN_SERVER_IP, "/api/v1/auth/login",
        "application/x-www-form-urlencoded",
        login_fields, 2, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n", response);

    char *session_cookie = extract_cookie(response);
    printf("Cookie retinut: %s\n\n", session_cookie ? session_cookie : "(none)");
    free(message); free(response);
    close_connection(sockfd);

    char *cookies[1] = {session_cookie};
    int   n_cookies   = (session_cookie != NULL) ? 1 : 0;

    /* ============================================================== */
    /* Ex 3 - GET /api/v1/weather/key                                  */
    /* ============================================================== */
    printf("=== Ex 3: GET weather key ===\n");
    sockfd  = open_connection(MAIN_SERVER_IP, MAIN_SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(
        MAIN_SERVER_IP, "/api/v1/weather/key",
        NULL, cookies, n_cookies);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n", response);

    /* Raspunsul e {"key":"<valoare>"} - extragem doar valoarea */
    char api_key[256] = {0};
    const char *key_body = extract_body(response);
    if (key_body)
        extract_json_field(key_body, "key", api_key, sizeof(api_key));
    printf("API key: [%s]\n\n", api_key);
    free(message); free(response);
    close_connection(sockfd);

    /* ============================================================== */
    /* Ex 4 - GET weather de la OpenWeatherMap                        */
    /* ============================================================== */
    printf("=== Ex 4: GET weather data de la OWM ===\n");

    /* Coordonate initiale trimise la OWM */
    const char *lat = "44.4268";
    const char *lon = "26.1025";

    char owm_params[512];
    snprintf(owm_params, sizeof(owm_params),
             "lat=%s&lon=%s&appid=%s", lat, lon, api_key);

    sockfd  = open_connection(owm_ip, OWM_PORT, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(OWM_HOST, "/data/2.5/weather", owm_params, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n", response);

    char *weather_json = NULL;
    char *json_ptr = basic_extract_json_response(response);
    if (json_ptr)
        weather_json = strdup(json_ptr);

    /*
     * OWM rotunjeste coordonatele in raspuns (ex: 44.4268 -> 44.43).
     * Serverul principal verifica daca coordonatele din URL se potrivesc
     * cu cele din JSON, deci extragem lat/lon DIN raspunsul OWM.
     */
    char lat_real[32] = {0}, lon_real[32] = {0};
    if (weather_json) {
        /* "coord":{"lon":26.1,"lat":44.43} - extragem din subsectiunea coord */
        char *coord = strstr(weather_json, "\"coord\":");
        if (coord) {
            extract_json_field(coord, "lat", lat_real, sizeof(lat_real));
            extract_json_field(coord, "lon", lon_real, sizeof(lon_real));
        }
    }
    printf("Coordonate reale din OWM: lat=%s lon=%s\n\n", lat_real, lon_real);

    free(message); free(response);
    close_connection(sockfd);

    /* ============================================================== */
    /* Ex 5 - POST /api/v1/weather/{lat}/{lon}                        */
    /* ============================================================== */
    printf("=== Ex 5: POST weather data pentru verificare ===\n");
    if (weather_json && lat_real[0] && lon_real[0]) {
        char verify_url[256];
        snprintf(verify_url, sizeof(verify_url),
                 "/api/v1/weather/%s/%s", lat_real, lon_real);

        char *verify_fields[] = {weather_json};

        sockfd  = open_connection(MAIN_SERVER_IP, MAIN_SERVER_PORT, AF_INET, SOCK_STREAM, 0);
        message = compute_post_request(
            MAIN_SERVER_IP, verify_url,
            "application/json",
            verify_fields, 1,
            cookies, n_cookies);
        send_to_server(sockfd, message);
        response = receive_from_server(sockfd);
        printf("%s\n\n", response);
        free(message); free(response);
        close_connection(sockfd);

        free(weather_json);
    } else {
        printf("Date insuficiente pentru Ex 5, sarim.\n\n");
    }

    /* ============================================================== */
    /* BONUS - Login din nou cu cookie activ (inainte de logout!)     */
    /* ============================================================== */
    printf("=== BONUS: Login din nou cu cookie activ ===\n");
    sockfd = open_connection(MAIN_SERVER_IP, MAIN_SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    message = compute_post_request(
        MAIN_SERVER_IP, "/api/v1/auth/login",
        "application/x-www-form-urlencoded",
        login_fields, 2,
        cookies, n_cookies);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);
    free(message); free(response);
    close_connection(sockfd);

    /* ============================================================== */
    /* Ex 6 - GET /api/v1/auth/logout  (ultimul pas!)                 */
    /* ============================================================== */
    printf("=== Ex 6: Logout ===\n");
    sockfd  = open_connection(MAIN_SERVER_IP, MAIN_SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(
        MAIN_SERVER_IP, "/api/v1/auth/logout",
        NULL, cookies, n_cookies);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("%s\n\n", response);
    free(message); free(response);
    close_connection(sockfd);

    /* ============================================================== */
    /* Curatare memorie                                                */
    /* ============================================================== */
    if (session_cookie) free(session_cookie);
    free(owm_ip);

    return 0;
}
