#include "syno_lib.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

    if (strncmp(config->session, "\0", sizeof(config->session)) == 0)
    {
        return config;
    }

    for (int i = 0; i < NUM_SESSIONS; i++)
    {
        if (strcmp(config->session, session) == 0) {
            break;
        }
        config++;
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
    while(ch_p != NULL)
    {
        printf("'%s'\n", ch_p);
        ch_p = strtok(NULL, delim);
    }

    printf("%s\n", ch_p[1]);
    printf("%s\n", ch_p[3]);
    printf("%s\n", ch_p[5]);

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
        printf("not enough memory (realloc returned NULL)\n");
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

    if (res != CURLE_OK)
        printf("Request %s failed with error code: %d", url, res);


    printf("%s", bufS.data);

    if (NULL != rsp)
        strncpy(rsp, bufS.data, sizeof(&rsp));

    free(bufS.data);

    return res;
}

void login(char* session)
{

    char url[50];
    set_config(session);
    Config* config = get_config(session);

    strcpy(url, config->server_url);
    strcat(url, "/webapi/auth.cgi?api=SYNO.API.Auth&version=6&method=login&account=");
    strcat(url, config->username);
    strcat(url, "&passwd=");
    strcat(url, config->password);
    strcat(url, "&session=");
    strcat(url, config->session);
    strcat(url, "&format=sid");
    strtok(url, "\n");

    char* rsp = config->sid;
    send_request(url, rsp);
}

void logoff(char* session)
{
    char url[50];
    Config* config = get_config(session);

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

    if (strlen(config->sid) == 0)
    {
        return 1;
    }

    return send_request(url, &rsp);
}

int make(char* session, char* url, char* rsp)
{
    login(session);
    int res = request(session, url, &rsp);
    logoff(session);
    return res;
}