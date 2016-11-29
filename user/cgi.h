#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int tplAlarm(HttpdConnData *connData, char *token, void **arg);
int tplCounter(HttpdConnData *connData, char *token, void **arg);

#endif
