#include <glob.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <iostream>

#include "mailman.h"

#define VALIDBANNER		"220"
#define VALIDMAIL		"250"
#define VALIDDATA		"354"
#define VALIDAUTH       "334"
#define AUTHOK          "235"

#define SMTP_PORT	25
#define HELO_MSG 		"Helo world\r\n"
#define MAIL_FROM		"Mail From: <%s>\r\n"
#define RCPT_TO			"Rcpt To: <%s>\r\n"
#define DATA_MSG 		"DATA\r\n"
#define FROM			"From: file checker <%s>\r\n"
#define TO			    "To: <%s>\r\n"
#define CC			    "Cc: <%s>\r\n"
#define SUBJECT			"Subject: %s\r\n"
#define END_DATA			"\r\n.\r\n"
#define QUIT_MSG 		"QUIT\r\n"

#define MAIL_DEBUG(x,y,z)
#define merror printf

#define charp(s) (const_cast<char *>((s).c_str()))

using std::cout;
using std::endl;

int connect_tcp(unsigned int _port, char *_ip)
{
    int ossock;
    struct sockaddr_in server;

    if((ossock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
        return -1;

    if((_ip == NULL)||(_ip[0] == '\0'))
        return -1;

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons( _port );
    server.sin_addr.s_addr = inet_addr(_ip);

    if(connect(ossock,(struct sockaddr *)&server, sizeof(server)) < 0)
        return -1;

    return ossock;
}

int send_tcp(int socket, const char *msg)
{
    if((send(socket, msg, strlen(msg),0)) <= 0) {
        return -1;
    }
    return(0);
}

char *recv_tcp(int socket, char *ret, int size)
{
    int len;
    if((len = recv(socket, ret, size-1,0)) <= 0) {
        return(NULL);
    }
    ret[len] = '\0';
    return ret;
}

void match(const char *token , char* str)
{
    char buf[512];
    if(NULL == str) {
        throw string("ddd");
    }
    unsigned int len = strlen(token);
    if (len > strlen(str)) {
        snprintf(buf, sizeof(buf), "mismatch %s:%s", token, str);
        throw string(buf);
    }
    if (strncmp(token, str, len) == 0) {
        return;
    }
    snprintf(buf, sizeof(buf), "mismatch %s:%s", token, str);
    throw string(buf);
}

// ulgy code
int Mailman::sendmail(const string& subject, const string& body)
{
    int socket;
    char *msg;
    char snd_msg[512];
    char buf[1024];

    try {
        cout << "begin to send mail\n";
        /* Connecting to the smtp server */
        socket = connect_tcp(SMTP_PORT, charp(_smtpserver));
        if(socket < 0) {
            cout << "connect smtp server "<< _smtpserver << " failed." << endl;
            return(socket);
        }

        /* Receiving the banner */
        msg = recv_tcp(socket, buf, 1024);
        match(VALIDBANNER, msg);

        printf ("DEBUG: Received banner: '%s' %s", msg, "\n");

        /* Send HELO message */
        send_tcp(socket,HELO_MSG);
        msg = recv_tcp(socket, buf, 1024);
        printf ("DEBUG: hello banner: '%s' %s", msg, "\n");
        match(VALIDMAIL, msg);

        snprintf(snd_msg,127, "%s\r\n", "AUTH LOGIN");
        send_tcp(socket, snd_msg);
        msg = recv_tcp(socket, buf, 1024);
        match(VALIDAUTH, msg);

        /* Build "Mail from" msg */

        /* username base64 encode*/
        snprintf(snd_msg,127, "%s\r\n", charp(_username));
        send_tcp(socket, snd_msg);
        msg = recv_tcp(socket, buf, 1024);
        match(VALIDAUTH, msg);
        /* password base64 encode */
        snprintf(snd_msg,127, "%s\r\n", charp(_password));
        send_tcp(socket, snd_msg);
        msg = recv_tcp(socket, buf, 1024);
        match(AUTHOK, msg);

        /* Build "Mail from" msg */
        snprintf(snd_msg,127, MAIL_FROM, charp(_from));
        send_tcp(socket, snd_msg);
        msg = recv_tcp(socket, buf, 1024);
        match(VALIDMAIL, msg);
        for(vector<string>::iterator it = _to.begin(); it != _to.end(); ++it) {
            snprintf(snd_msg,127,RCPT_TO, charp(*it));
            send_tcp(socket,snd_msg);
            msg = recv_tcp(socket, buf, 1024);
            match(VALIDMAIL, msg);
        }
        /* Send the "DATA" msg */
        send_tcp(socket,DATA_MSG);
        msg = recv_tcp(socket, buf, 1024);
        match(VALIDDATA, msg);

        /* Building "From" and "To" in the e-mail header */
        for(vector<string>::iterator it = _to.begin(); it != _to.end(); ++it) {
            snprintf(snd_msg,127, TO, charp(*it));
            send_tcp(socket, snd_msg);
        }
        snprintf(snd_msg,127, FROM, charp(_from));
        send_tcp(socket, snd_msg);

        /* Sending date */
        memset(snd_msg,'\0',128);
        time_t now = time(NULL);
        struct tm tmval;
        gmtime_r(&now, &tmval);
        strftime(snd_msg, 127, "Date: %a, %d %b %Y %T %z\r\n",&tmval);

        send_tcp(socket, snd_msg);

        /* Sending subject */
        snprintf(snd_msg, 127, SUBJECT, charp(subject));
        send_tcp(socket,snd_msg);

        /* Sending body */
        snprintf(snd_msg, sizeof(snd_msg), "\r\n%s\r\n", charp(body));
        send_tcp(socket, snd_msg);
        /* Sending end of data \r\n.\r\n */
        send_tcp(socket, END_DATA);

        msg = recv_tcp(socket, buf, 1024);
        match(VALIDMAIL, msg);

        send_tcp(socket,QUIT_MSG);
        msg = recv_tcp(socket, buf, 1024);

        close(socket);
    }
    catch (string& errormsg) {
        std::cout << errormsg << std::endl;
    }
    return(0);
}
