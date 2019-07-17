#include "syno_lib.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#define NUM_SESSIONS 10
#define SID_SIZE 20
#define LOGIN_SIZE 10
#define MAX_URL_SIZE 50

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

Config* get_config(char* session, int *error_code)
{
    Config *config = config_arr;
    bool found = false;

    if ((0 == strlen(config->session) || 0 != strlen(config->username)) && 0 != strcmp(config->session, "new"))
    {
        *error_code = 0;
        return config;
    }

    for (int i = 0; i < NUM_SESSIONS; i++)
    {
        if (0 == strcmp(config->session, session) ||
                ((0 == strlen(config->username) && 0 == strcmp(session, "new")) )) {
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

    fread(buffer, 1024, 1, file_p);
    fclose(file_p);

    static char *save_p;
    char* next_p = malloc(MAX_URL_SIZE * sizeof(char));
    if (NULL == next_p) {
        fprintf(stderr, "\nCould not allocate enough memory to parse the configuration file: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    next_p = strtok_r(buffer, delim, &save_p);
    char* prev_p = NULL;

    int err;
    Config* config = get_config("new", &err);
    if (0 != err) {
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

int send_request(const char *url, char *rsp)
{
    CURL* curl = curl_easy_init();
    CURLcode res;

    struct Buffer bufS;
    bufS.data = malloc(1);
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

    if (CURLE_OK != res)
        fprintf(stderr, "\nRequest %s failed with error code: %d", url, res);

#ifdef DEBUG
    printf("%s", bufS.data);
#endif

    if (NULL != rsp)
        strncpy(rsp, bufS.data, sizeof(&rsp));

    free(bufS.data);

    return res;
}

void login(char* session)
{
    int err;
    char url[300];
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

        char* rsp = malloc(sizeof(char) * SID_SIZE);
        send_request(url, rsp);

#ifdef DEBUG
        rsp = "sid: abc123zxy987__";
#endif
        char* sid = strstr(rsp, "sid");
        static char* save_p;
        if (NULL != sid) {
            sid = strtok_r(sid, ": ", &save_p);
            sid = strtok_r(NULL, " ", &save_p);
            strncpy(config->sid, sid, sizeof(config->sid));
        }
        free(rsp);
    }
}

void logoff(char* session)
{
    int err;
    char url[50];
    Config* config = get_config(session, &err);
    if (0 == err) {
        strcpy(url, config->server_url);
        strcat(url, "/webapi/auth.cgi?api=SYNO.API.Auth&version=1&method=logout&session=");
        strcat(url, config->session);
        strtok(url, "\n");
        send_request(url, NULL);
    }
    memset(config->sid, 0, sizeof config->sid);
    memset(config->session, 0, sizeof config->session);
}

void clear_all_conf() {
    memset(config_arr, 0, sizeof config_arr);
}

int request(char* session, char* url, char* rsp)
{
    int err;
    Config* config = get_config(session, &err);
    if (0 != err || 0 == strlen(config->sid)) {
        fprintf(stderr, "\nSession lost: %d\n", err);
        return EXIT_FAILURE;
    }
    strcat(url, config->sid);
    return send_request(url, rsp);
}

int make(char* session, char* url, char* rsp)
{
    login(session);

    int res = request(session, url, rsp);
    if (EXIT_FAILURE != res) {
        fprintf(stderr, "\nRequest failed: %s\n", strerror(errno));
        return res;
    }

    logoff(session);
    return res;
}

int test_connection(char* session) {
    int err;
    login(session);
    Config* config = get_config(session, &err);
    fprintf(stdout, "%s\t", config->username);
    fprintf(stdout, "%s\n", config->session);

    logoff(session);
    config = get_config(session, &err);
    fprintf(stdout, "%s\t", config->username);
    fprintf(stdout, "%s\n", config->session);
    return err;
}