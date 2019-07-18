#include "syno_lib.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#define NUM_SESSIONS 10
#define SID_SIZE 30
#define LOGIN_SIZE 20
#define MAX_URL_SIZE 300
#define RSP_SIZE 1024

typedef struct ConfigurationInfo {
    char username[LOGIN_SIZE];
    char password [LOGIN_SIZE];
    char session[LOGIN_SIZE];
    char server_url [MAX_URL_SIZE];
    char sid[SID_SIZE];
}Config;

struct Buffer {
    char *data;
    size_t size;
};

static Config config_arr[NUM_SESSIONS];

Config* get_new_config(int *error_code)
{
    Config *config = config_arr;
    bool found = false;

    for (int i = 0; i < NUM_SESSIONS; i++)
    {
        if (0 == strlen(config->username)) {
            found = true;
            *error_code = 0;
            break;
        }
        config++;
    }

    if (!found) {
        *error_code = -1;
        return NULL;
    }

    return config;
}

Config* get_config(char* session, int *error_code)
{
    Config *config = config_arr;
    bool found = false;

    for (int i = 0; i < NUM_SESSIONS; i++)
    {
        if (0 == strcmp(config->session, session)) {
            found = true;
            *error_code = 0;
            break;
        }
        config++;
    }
    if (!found) {
        *error_code = -1;
        return NULL;
    }

    return config;
}

void set_config(char* session, int *error_code)
{
    Config *config = config_arr;
    *error_code = -1;
    for (int i = 0; i < NUM_SESSIONS; i++)
    {
        if (0 == strlen(config->session) && 0 != strlen(config->username)) {
            strncpy(config->session, session, sizeof(config->session));
            *error_code = 0;
            break;
        }
        config++;
    }
}

int init(char* abspath_conf_file, const char* delim)
{
    char buffer[1024];
    FILE *file_p;
    file_p = fopen(abspath_conf_file,"r");
    if (NULL == file_p) {
        fprintf(stderr, "\nFailed to parse configuration file: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    fread(buffer, 512, 1, file_p);
    fclose(file_p);

    char *save_p;
    char* next_p = malloc(MAX_URL_SIZE * sizeof(char));
    if (NULL == next_p) {
        fprintf(stderr, "\nCould not allocate enough memory to parse the configuration file: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    next_p = strtok_r(buffer, delim, &save_p);
    char* prev_p = NULL;

    int err;
    Config* config = get_new_config(&err);
    if (0 != err) {
        free(next_p);
        fprintf(stderr, "\nNumber of sessions exceeded: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    while (NULL != next_p)
    {
        prev_p = next_p;
        next_p = strtok_r(NULL, delim, &save_p);

        if (0 == strcasecmp(prev_p, "username"))
            strncpy(config->username, next_p, sizeof(config->username));

        else if (0 == strcasecmp(prev_p, "password"))
            strncpy(config->password, next_p, sizeof(config->password));

        else if (0 == strcasecmp(prev_p, "server"))
            strncpy(config->server_url, next_p, sizeof(config->server_url));
    }
    free(next_p);

    printf("Loaded configuration:\n");
    fprintf(stdout,"%s \t", config->username);
    fprintf(stdout, "%s \t", config->password);
    fprintf(stdout, "%s \t", config->server_url);

    return EXIT_SUCCESS;
}

static size_t callback(void* in, size_t size, size_t num, char* out)
{
    size_t totalBytes = size * num;
    struct Buffer *bufS_p = (struct Buffer *)out;
    char *mem_ptr = realloc(bufS_p->data, bufS_p->size + totalBytes + 1);
    if(NULL == mem_ptr) {
        fprintf(stderr, "\nNot enough memory, %s\n", strerror(errno));
        return 0;
    }
    bufS_p->data = mem_ptr;
    memcpy(&(bufS_p->data[bufS_p->size]), in, totalBytes);
    bufS_p->size += totalBytes;
    bufS_p->data[bufS_p->size] = 0;
    return totalBytes;
}

int send_request(const char *url, void *rsp)
{
    CURL* curl = curl_easy_init();
    CURLcode res;

    struct Buffer bufS;
    bufS.data = malloc(32);
    bufS.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&bufS);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res);
    curl_easy_cleanup(curl);

    if (200 != res)
        fprintf(stderr, "\nRequest %s failed with error code: %d", url, res);

#ifdef DEBUG
    printf("%s", bufS.data);
#endif
    if (NULL != rsp)
        memccpy(rsp, bufS.data, '\n', bufS.size);

    free(bufS.data);

    return res;
}

void login(char* session)
{
    int err;
    char url[MAX_URL_SIZE];
    set_config(session, &err);

    if (0 != err) {
        fprintf(stderr, "\nNumber of sessions exceeded: %d\n", err);
        exit(err);
    }

    Config* config = get_config(session, &err);

    if (NULL != config)
    {
        strcpy(url, config->server_url);
        strcat(url, "/webapi/auth.cgi?api=SYNO.API.Auth&version=6&method=login&account=");
        strcat(url, config->username);
        strcat(url, "&passwd=");
        strcat(url, config->password);
        strcat(url, "&session=");
        strcat(url, config->session);
        strcat(url, "&format=sid");
        strtok(url, "\n");

        void* rsp = malloc(RSP_SIZE);
        send_request(url, rsp);

#ifdef DEBUG
        fprintf(stdout, "\nRSP: %s\n", (char*)rsp);
#endif
        char* sid = strstr((char*)rsp, "sid");

        char* save_p;
        if (NULL != sid) {
            sid = strtok_r(sid, "}", &save_p);
            strncpy(config->sid, sid+5, SID_SIZE * sizeof(char));
        }
        free(rsp);
    }
}

void logoff(char* session)
{
    int err;
    char url[MAX_URL_SIZE];
    Config* config = get_config(session, &err);
    if (0 == err) {
        strcpy(url, config->server_url);
        strcat(url, "/webapi/auth.cgi?api=SYNO.API.Auth&version=1&method=logout&session=");
        strcat(url, config->session);
        strtok(url, "\n");
        send_request(url, NULL);
    }
    memset(config->sid, 0, sizeof(config->sid));
    memset(config->session, 0, sizeof(config->session));
}

void clear_all_conf() {
    memset(config_arr, 0, sizeof(config_arr));
}

int request(char* session, char* url, void* rsp)
{
    int err;
    char total_url[MAX_URL_SIZE];

    Config* config = get_config(session, &err);
    if (0 != err || 0 == strlen(config->sid)) {
        fprintf(stderr, "\nSession lost: %d\n", err);
        return EXIT_FAILURE;
    }
    strcat(total_url, config->server_url);
    strcat(total_url, url);

    char* sid_request = strstr(url, "sid");
    if (NULL != sid_request) {
        strcat(url, config->sid);
    }
    int res = send_request(total_url, rsp);
    if (res != 200) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int make(char* session, char* url, void* rsp)
{
    login(session);

    int res = request(session, url, rsp);
    if (EXIT_SUCCESS != res) {
        fprintf(stderr, "\nRequest failed: %s\n", strerror(errno));
        return res;
    }

    logoff(session);
    return res;
}

int test_connection(char* session) {
    int err = 0;
    login(session);
    Config* config = get_config(session, &err);
    if (0 != err )
        return err;

    logoff(session);
    config = get_config(session, &err);
    if (config != NULL)
    {
        fprintf(stderr, "\nSession was still open: %s\n", strerror(errno));
    }
    return err;
}