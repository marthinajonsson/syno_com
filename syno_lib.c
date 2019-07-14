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

Config config_arr[NUM_SESSIONS];

Config* get_config(char* session)
{
    Config *config = config_arr;
    bool found = false;

    if (0 == strlen(config->session))
    {
        return config;
    }

    for (int i = 0; i < NUM_SESSIONS; i++)
    {
        if (0 == strcmp(config->session, session)) {
            found = true;
            break;
        }
        config++;
    }
    if (!found) {
        return NULL;
    }

    return config;
}

void set_config(char* session)
{
    Config *config = config_arr;

    for (int i = 0; i < NUM_SESSIONS; i++)
    {
        if (strlen(config->session) == 0) {
            strncpy(config->session, session, sizeof(config->session));
        }
        config++;
    }
}

void init(char* abspath_conf_file)
{
    char buffer[1024];
    char delim[] = "|";
    FILE *fp;
    fp = fopen(abspath_conf_file,"r");
    fread(buffer, 1024, 1, fp);
    fclose(fp);

    char* ch_p = strtok(buffer, delim);
    while (NULL != ch_p)
    {
        printf("'%s'\n", ch_p);
        ch_p = strtok(NULL, delim);
    }
#ifdef DEBUG
    printf("%s\n", ch_p[1]);
    printf("%s\n", ch_p[3]);
    printf("%s\n", ch_p[5]);
#endif
    memset(config_arr, 0, NUM_SESSIONS * (sizeof config_arr[0]) );
    for (int i = 0; i < NUM_SESSIONS; i++) {
        config_arr[i];
        strncpy(config_arr[i].username,ch_p[1], sizeof(config_arr[i].username));
        strncpy(config_arr[i].password,ch_p[3], sizeof(config_arr[i].password));
        strncpy(config_arr[i].server_url,ch_p[5], sizeof(config_arr[i].server_url));
    }
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

void* login(char* session)
{
    char url[50];
    set_config(session);
    Config* config = get_config(session);

    if (NULL == config)
        return NULL;

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
    if (NULL != sid) {
        sid = strtok(sid, ": ");
        sid = strtok(NULL, " ");
        strcpy(config->sid, sid);
    }
    free(rsp);
}

void* logoff(char* session)
{
    char url[50];
    Config* config = get_config(session);
    if (NULL == config)
        return NULL;

    strcpy(url, config->server_url);
    strcat(url, "/webapi/auth.cgi?api=SYNO.API.Auth&version=1&method=logout&session=");
    strcat(url, config->session);
    strtok(url, "\n");
    send_request(url, NULL);
    memset(config->sid, 0, sizeof config->sid);
    memset(config->session, 0, sizeof config->session);
}

int request(char* session, char* url, char* rsp)
{
    Config* config = get_config(session);

    if (0 == strlen(config->sid))
    {
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