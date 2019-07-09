#include "library.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ConfigInfo {
    char* username;
    char* password;
    char* server;
}info;

struct Buffer {
    char *data_p;
    size_t size;
};

static size_t callback(void* in, size_t size, size_t num, char* out)
{
    size_t totalBytes = size * num;
    struct Buffer *bufS_p = (struct Buffer *)out;
    char *mem_ptr = realloc(bufS_p->data_p, bufS_p->size + totalBytes + 1);
    if(NULL == mem_ptr) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
    bufS_p->data_p = mem_ptr;
    memcpy(&(bufS_p->data_p[bufS_p->size]), in, totalBytes);
    bufS_p->size += totalBytes;
    bufS_p->data_p[bufS_p->size] = 0;

    return totalBytes;
}

void send_request(const char *url) {

    CURL* curl = curl_easy_init();
    CURLcode res;

    struct Buffer bufS;
    bufS.data_p = malloc(1);
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

    printf("%s", bufS.data_p);
    free(bufS.data_p);
}

void login(char* session) {

}

void logoff(char* session) {

}

void send(char* session, char* url) {

}

void make(char* session, char* url) {
    login(session);
    send(url, session);
    logoff(session);
}