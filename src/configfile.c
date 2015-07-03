/*
   read ini configure.
   for check_realtime only.
   yxf 2013/9/25
 */

#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "configfile.h"
#define my_free(ptr) do { if(ptr) { free(ptr); ptr = NULL; } } while(0)
#define TRUE 1
#define FALSE 0
#define OK  0
#define ERROR -1

extern void strip(char *buffer);

/* for the sake of simplicity, i use a list to save the parameters */
struct parameter {
    char *name;
    char *value;
    struct parameter* next;
};

struct parameter param_head = {0};


void add_param(char *name, char *value)
{
    struct parameter *next = param_head.next;
    struct parameter *param = (struct parameter *)malloc(sizeof(struct parameter));
    if(NULL == param) {
        return;
    }
    param->name = name;
    param->value = value;
    param->next = next;
    param_head.next = param;
}

char *get_config_param(const char *name)
{
    struct parameter *ptr;
    for(ptr = param_head.next; ptr != NULL; ptr = ptr->next) {
        if(strcmp(ptr->name, name) == 0) {
            return ptr->value;
        }
    }
    return NULL;
}

/* support for continuation lines */
char *fgets_multiline(FILE *fp) {
    char *buf = NULL;
    char *ptr = NULL;
    char temp[4096];
    char *stripped = NULL;
    int len = 0;
    int len2 = 0;
    int end = 0;

    if(fp == NULL)
        return NULL;

    while(1) {

        if((ptr = fgets(temp, sizeof(temp), fp)) == NULL)
            break;

        if(buf == NULL) {
            len = strlen(temp);
            if((buf = (char *)malloc(len + 1)) == NULL)
                break;
            memcpy(buf, temp, len);
            buf[len] = '\x0';
        }
        else {
            /* strip leading white space from continuation lines */
            stripped = temp;
            while(*stripped == ' ' || *stripped == '\t')
                stripped++;
            len = strlen(stripped);
            len2 = strlen(buf);
            if((buf = (char *)realloc(buf, len + len2 + 1)) == NULL)
                break;
            strcat(buf, stripped);
            len += len2;
            buf[len] = '\x0';
        }

        if(len == 0)
            break;

        /* handle Windows/DOS CR/LF */
        if(len >= 2 && buf[len - 2] == '\r')
            end = len - 3;
        /* normal Unix LF */
        else if(len >= 1 && buf[len - 1] == '\n')
            end = len - 2;
        else
            end = len - 1;

        /* two backslashes found. unescape first backslash first and break */
        if(end >= 1 && buf[end - 1] == '\\' && buf[end] == '\\') {
            buf[end] = '\n';
            buf[end + 1] = '\x0';
            break;
        }

        /* one backslash found. continue reading the next line */
        else if(end > 0 && buf[end] == '\\')
            buf[end] = '\x0';

        /* no continuation marker was found, so break */
        else
            break;
    }

    return buf;
}


void strip(char *buffer) {
    register int x, z;
    int len;

    if(buffer == NULL || buffer[0] == '\x0')
        return;


    len = (int)strlen(buffer);
    for(x = len - 1; x >= 0; x--) {
        switch(buffer[x]) {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
            buffer[x] = '\x0';
            continue;
        }
        break;
    }

    if(!x)
        return;

    z = x;

    for(x = 0;; x++) {
        switch(buffer[x]) {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
            continue;
        }
        break;
    }

    if(x > 0 && z > 0) {
        len = z + 1;

        for(z = x; z < len; z++)
            buffer[z - x] = buffer[z];
        buffer[len - x] = '\x0';
    }
}

int read_config_file(const char *config_file)
{
    FILE *fp;
    char *input = NULL;
    char *value = NULL;
    char *variable = NULL;
    char *temp_ptr = NULL;
    int rc = 0;

    if((fp = fopen(config_file, "r")) == NULL) {
        printf("Error: cannot open main configuration file '%s' for reading!", config_file);
        return -1;
    }

    while(1) {
        my_free(input);
        if((input = fgets_multiline(fp)) == NULL)
            break;
        strip(input);

        /* skip blank lines and comments */
        if(input[0] == '\x0' || input[0] == '#')
            continue;
        if((temp_ptr = strtok(input, "=")) == NULL) {
            rc = -1;
            break;
        }
        if((variable = (char *)strdup(temp_ptr)) == NULL) {
            rc = -1;
            break;
        }
        if((temp_ptr = strtok(NULL, "\n")) == NULL) {
            rc = -1;
            break;
        }
        if((value = (char *)strdup(temp_ptr)) == NULL) {
            rc = -1;
            break;
        }

        strip(variable);
        strip(value);
        add_param(variable, value);
    }
    my_free(input);
    fclose(fp);

    return rc;
}
