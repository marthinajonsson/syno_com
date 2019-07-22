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
#define MAX_URL_SIZE 256
#define RSP_SIZE 1024

typedef struct ConfigurationInfo {
    char username[LOGIN_SIZE];
    char password [LOGIN_SIZE];
    char session[LOGIN_SIZE];
    char server_url [MAX_URL_SIZE];
    char sid[SID_SIZE];
}Config;


static Config config_arr[NUM_SESSIONS];

struct Buffer {
    char *data;
    size_t size;
};

// Configuration utilites
Config* get_new_config();
Config* get_config(char* session, int *err_code);
void set_config(char* session, int *err_code);

// Communication utilities
static size_t callback(void* in, size_t size, size_t num, char* out);
int send_request(const char *url, void *rsp);

// Called by make()
int login(char* session);
void logoff(char* session);
int request(char* session, char* url, void* rsp);


/*
 * PUBLIC
 * */

/// \details Clear all sessions and configurations
void clear_all_conf() {
    memset(config_arr, 0, sizeof(config_arr));
}

/// \details
/// \param abspath_conf_file
/// \param delim
/// \return
int init(char* abspath_conf_file, const char* delim)
{
    FILE *file_p;
    file_p = fopen(abspath_conf_file,"r");
    if (NULL == file_p) {
        fprintf(stderr, "\nFailed to parse configuration file: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    char buffer[MAX_URL_SIZE];
    fread(buffer, MAX_URL_SIZE, 1, file_p);
    fclose(file_p);

    if (NULL == delim) {
        fprintf(stderr, "\nDelimiter missing.\n");
        return EXIT_FAILURE;
    }

    Config* config = get_new_config();
    if (NULL == config) {
        fprintf(stderr, "\nNumber of sessions exceeded: %d\n", CONFIG_TO_MANY_SESSIONS);
        return EXIT_FAILURE;
    }

    void* next_p = malloc(MAX_URL_SIZE * sizeof(char));
    if (NULL == next_p) {
        fprintf(stderr, "\nCould not allocate enough memory to parse the configuration file: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    char *save_p = buffer;
    next_p = strtok_r(buffer, delim, &save_p);
    char* prev_p = NULL;
    //save_p = buffer;
    while (true)
    {
        prev_p = next_p;
        next_p = strtok_r(NULL, delim, &save_p);
        if (NULL == next_p) break;

        if (0 == strcasecmp(prev_p, "username"))
            strncpy(config->username, next_p, sizeof(config->username));

        else if (0 == strcasecmp(prev_p, "password"))
            strncpy(config->password, next_p, sizeof(config->password));

        else if (0 == strcasecmp(prev_p, "server"))
            strncpy(config->server_url, next_p, sizeof(config->server_url));

    }
    prev_p = NULL;
    save_p = NULL;
    free(next_p);

    printf("\n\nLoaded configuration:\n");

    fflush(stderr);
    fprintf(stderr, "USERNAME %s \n", config->username);
    fprintf(stderr, "PASSWORD %s \n", config->password);
    fprintf(stderr,"SERVER %s \n", config->server_url);

    return EXIT_SUCCESS;
}

int make(char* session, char* request_url, void* rsp)
{
    int res = login(session);

    if (HTML_REQUEST_OK != res) {
        fprintf(stderr, "\nLogin failed for session: %s\n", session);
        return res;
    }

    res = request(session, request_url, rsp);
    if (EXIT_SUCCESS != res) {
        fprintf(stderr, "\nRequest failed: %s\n", strerror(errno));
    }

    logoff(session);
    return res;
}

int test_connection(char* session) {
    int err = 0;

    err = login(session);

    if (HTML_REQUEST_OK != err) {
        fprintf(stderr, "\nLogin failed for session: %s\n", session);
        return err;
    }

    Config* config = get_config(session, &err);

    if (CONFIG_OK != err )
        return err;

    logoff(session);

    config = get_config(session, &err);
    if (config != NULL)
    {
        fprintf(stderr, "\nSession was still open: %s\n", strerror(errno));
    }
    return err;
}



/*
 *  Called by make()
 *
 * */

/// \details Login to session. Sid is saved for later request.
/// \param session Key used to identify session.
int login(char* session)
{
    int err;
    int html_err = 0;
    char url[MAX_URL_SIZE] = "";
    set_config(session, &err);

    if (CONFIG_NOT_INITIALIZED == err) {
        fprintf(stderr, "\nNumber of sessions exceeded or not initialized: %d\n", err);
        return EXIT_FAILURE;
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
        if (NULL == rsp) {
            fprintf(stderr, "Failed to allocate memory for response\n");
        }
        html_err = send_request(url, rsp);

#ifdef DEBUG
        fprintf(stdout, "\nRSP: %s\n", (char*)rsp);
#endif
        char* sid = strstr((char*)rsp, "sid");

        char* save_p = sid;
        if (NULL != sid) {
            sid = strtok_r(sid, "}", &save_p);
            strncpy(config->sid, sid+5, SID_SIZE * sizeof(char));
        }
        sid = NULL;
        save_p = NULL;
        free(rsp);
    }
    return html_err;
}

/// \details Logoff from session.
/// \param session Key used to clear session.
void logoff(char* session)
{
    int err;
    char url[MAX_URL_SIZE] = "";
    Config* config = get_config(session, &err);
    if (CONFIG_OK == err) {
        strcpy(url, config->server_url);
        strcat(url, "/webapi/auth.cgi?api=SYNO.API.Auth&version=1&method=logout&session=");
        strcat(url, config->session);
        strtok(url, "\n");
        send_request(url, NULL);
    }
    else {
        fprintf(stderr, "\nConfiguration not available: %d\n", err);
    }

    memset(config->sid, 0, sizeof(config->sid));
    memset(config->session, 0, sizeof(config->session));
}

/// \details Compiles and sends request.
/// \param session Pointer to session key.
/// \param url Pointer to url.
/// \param rsp Pointer to memory allocated for response.
/// \return SUCCESS 0 or FAILURE 1.
int request(char* session, char* url, void* rsp)
{
    int err;
    char total_url[MAX_URL_SIZE] = "";

    Config* config = get_config(session, &err);
    if (0 != err || 0 == strlen(config->sid)) {
        fprintf(stderr, "\nSession lost\n");
        return EXIT_FAILURE;
    }
    strcat(total_url, config->server_url);
    strcat(total_url, url);

    char* sid_request = strstr(url, "sid");
    if (NULL != sid_request) {
        strcat(url, config->sid);
    }
    int res = send_request(total_url, rsp);
    if (res != HTML_REQUEST_OK) {
        fprintf(stderr, "HTML request failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}





/*
 *
 * Communication utilities
 *
 *
 * */


/// \details Send request to SYNO DISK MANAGER.
/// \param url Pointer to complete URL including SID.
/// \param rsp Pointer to memory allocated for the response.
/// \return HTML error code, 200 == OK.
int send_request(const char *url, void *rsp)
{
    CURL* curl = curl_easy_init();
    CURLcode res;

    struct Buffer bufS; // temporary storage for response
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

    if (NULL != rsp) {

        if (RSP_SIZE < bufS.size) {
            fprintf(stdout, "Response too large. Risk of data loss.\n");
            memccpy(rsp, bufS.data, '\0', RSP_SIZE);
        }
        else {
            memccpy(rsp, bufS.data, '\0', bufS.size);
        }
    }
    else {
        char* successful = strstr(bufS.data, "success");
        successful = strtok(successful, "}");
        successful = strstr(bufS.data, "true");
        if (NULL == successful)
            fprintf(stdout, "Returned SYNO data { success : false } for %s.\n", url);
    }

    free(bufS.data);

    return res;
}

/// \details Callback function for CURL WriteFunction.
/// \param in Pointer to response coming in.
/// \param size Size of chunk in bytes.
/// \param num Number of chunks.
/// \param out Pointer to temporary storage for response.
/// \return Total bytes of data chunks.
size_t callback(void* in, size_t size, size_t num, char* out)
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





/*
 *
 * Configuration utilities
 *
 * */

/// \details Searches for available sessions and returns an empty session.
/// \return Pointer to new struct ConfigurationInfo instance if available, otherwise NULL.
Config* get_new_config()
{
    Config *config = config_arr;
    bool found = false;

    for (int i = 0; i < NUM_SESSIONS; ++i, ++config)
    {
        if (0 == strlen(config->username)) {
            found = true;
            break;
        }
    }

    if (!found) {
        return NULL;
    }

    return config;
}

/// \details Searches for ongoing sessions and returns corresponding configuration.
/// \param session Key used to match the searches.
/// \param err_code Pointer to error code.
/// \return Pointer to existing struct ConfigurationInfo instance if available, otherwise NULL.
Config* get_config(char* session, int *err_code)
{
    Config *config = config_arr;
    bool found = false;
    *err_code = CONFIG_NOT_AVAILABLE;

    for (int i = 0; i < NUM_SESSIONS; ++i, ++config)
    {
        if (0 == strcmp(config->session, session)) {
            found = true;
            *err_code = CONFIG_OK;
            break;
        }
    }

    if (!found) {
        return NULL;
    }

    return config;
}

/// \details Save current session
/// \param session Key used to save the session
/// \param err_code Pointer to error code
void set_config(char* session, int *err_code)
{
    Config *config = config_arr;
    *err_code = CONFIG_NOT_INITIALIZED;

    for (int i = 0; i < NUM_SESSIONS; ++i, ++config)
    {
        if (0 == strlen(config->session) && 0 != strlen(config->username)) {
            strncpy(config->session, session, sizeof(config->session));
            *err_code = CONFIG_OK;
            break;
        }
    }
}
