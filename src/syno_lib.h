#ifndef SYNO_COM_LIBRARY_H
#define SYNO_COM_LIBRARY_H

#define CONFIG_OK 0
#define CONFIG_NOT_AVAILABLE 1
#define CONFIG_TO_MANY_SESSIONS 2
#define CONFIG_NOT_INITIALIZED 3

#define HTML_REQUEST_OK 200
#define RSP_NUM_BYTES (1024 * sizeof(char))

int init(char* abspath_conf_file, const char* delim);
int make(char* session, char* url, void* rsp);
void clear_all_conf();

int test_connection(char* session);






#endif //SYNO_COM_LIBRARY_H
