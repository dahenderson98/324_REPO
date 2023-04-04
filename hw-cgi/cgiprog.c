/* $begin cgiprogc */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 1024

int main(int argc, char *argv[]){
    // char a1[MAX_LINE];
    // strcpy(a1, argv[1]);
    // setenv("QUERY_STRING", a1, 0);

    // COMMANDS
    // ./cgiprog foo=bar
    // ./cgiprog abs=123&def=456
    // make; ./driver.py

    char *buf;
    char arg1[512], content[MAX_LINE];

    /* Generate the response body */
    buf = getenv("QUERY_STRING");
    sprintf(content, "The query string is: %s", buf);
    // fprintf(stderr,"%s\n",content);
    // fprintf(stderr,"%d\n",(int)strlen(content));

    /* Generate the response headers */
    fprintf(stdout, "Content-type: text/plain\r\n");
    fprintf(stdout, "Content-length: %d\r\n", (int)strlen(content));
    fprintf(stdout, "\r\n");

    fprintf(stdout, "%s", content);
    fflush(stdout);

    return 0;
}