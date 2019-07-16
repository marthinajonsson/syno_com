#ifndef SYNO_COM_LIBRARY_H
#define SYNO_COM_LIBRARY_H

#define RES_OK 0
#define RES_FAIL 1

void init(char* abspath_conf_file);
int make(char* session, char* url, char* rsp);


#endif //SYNO_COM_LIBRARY_H