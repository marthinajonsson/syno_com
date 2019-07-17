#ifndef SYNO_COM_LIBRARY_H
#define SYNO_COM_LIBRARY_H

#define RES_OK 0
#define RES_FAIL 1

int init(char* abspath_conf_file, const char* delim);
int make(char* session, char* url, char* rsp);
void clear_all_conf();

int test_connection(char* session);

#endif //SYNO_COM_LIBRARY_H