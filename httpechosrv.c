/* 
 * tcpechosrv.c - A concurrent TCP echo server using threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <sys/stat.h> // For file stats
#include <fcntl.h> 

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void echo(int connfd, char *buf, size_t n);
void *thread(void *vargp);

int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
	connfdp = malloc(sizeof(int));
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  
    int connfd = *((int *)vargp), prev_connfd = -1;
    char buf[MAXLINE]; 
    size_t n;
    pthread_detach(pthread_self()); 
    free(vargp);
    time_t start_time = time(NULL);
    while (1) {
        printf("\nConnfd: %d", connfd);
        if ((n = read(connfd, buf, MAXLINE)) > 0) {
            start_time = time(NULL);
            printf("\nIn starttime inc loop\n");
        } 
        // Check if the "Connection: Keep-alive" header is present
        if (strstr(buf, "Connection: Keep-alive") != NULL) {
            printf("\nIn keep alive string check\n");
            // Check if 10 seconds have not passed since the connection was established
            if (time(NULL) - start_time < 10) {
                printf("\nbelow 10 secs\n");
                // Pass the buf to the echo function and show it
                echo(connfd, buf, n);
            } else {
                // Connection timeout, close the connection
                break;
            }
        } else {
            echo(connfd, buf, n);
            // Close the connection if "Connection: Keep-alive" is not present
            break;
        }
    }
    //echo(connfd);
    close(connfd);
    return NULL;
}

void serve_file(int connfd, const char *filename, int postActive) {
    size_t n;
    char buf[MAXLINE];
    char httpmsg[MAXBUF];
    struct stat filestat;
    
    // Check if the file exists
    if (stat(filename, &filestat) < 0) {
        // If the file doesn't exist, return a 404 Not Found response
        sprintf(httpmsg, "HTTP/1.1 404 Not Found\r\n\r\n");
        write(connfd, httpmsg, strlen(httpmsg));
        return;
    }
    printf("\n Filename in serve_file %s: ", filename);

    // Determine the Content-Type based on the file's extension
    const char *extension = strrchr(filename, '.');
    int isHtml = -1;
    const char *content_type = "application/octet-stream";  // Default to binary data

    if (extension) {
        if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
            content_type = "image/jpeg";
        } else if (strcmp(extension, ".png") == 0) {
            content_type = "image/png";
        } else if (strcmp(extension, ".gif") == 0) {
            content_type = "image/gif";
        } else if (strcmp(extension, ".pdf") == 0) {
            content_type = "application/pdf";
        } else if (strcmp(extension, ".html") == 0) {
            content_type = "text/html";
            isHtml = 1;
        } else if (strcmp(extension, ".css") == 0) {
            content_type = "text/css";
        } else if (strcmp(extension, ".js") == 0) {
            content_type = "application/javascript";
        } else if (strcmp(extension, ".txt") == 0) {
            content_type = "text/plain";
        }
        // Add more file types and MIME types as needed
    }

    // Send an HTTP 200 OK response with the correct Content-Type
    sprintf(httpmsg, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: Close\r\n\r\n", content_type, filestat.st_size);
    write(connfd, httpmsg, strlen(httpmsg));
    
    // POST only
    if (postActive == 1 && isHtml == 1) {
        FILE *file = fopen(filename, "r");
        if (file == NULL) {
            perror("Failed to open file");
            return;
        }
        char content[10000];  // Adjust the buffer size as needed
        char line[10000];     // Adjust the buffer size as needed
        int line_count = 0;
        char *post_data = "<h1>This is POST data.</h1>";

        while (fgets(line, sizeof(line), file) != NULL) {
            line_count++;
            strcat(content, line);

            if (line_count == 2) {
                // Insert the POST data on the third line
                strcat(content, post_data);
                strcat(content, "\n");
            }
        }
        fclose(file);
        file = fopen(filename, "w");
        if (file == NULL) {
            perror("Failed to open file for writing");
            return;
        }

        // Write the modified content back to the file
        fputs(content, file);

        // Close the file
        fclose(file);
    }
    int filefd = open(filename, O_RDONLY);
    while ((n = read(filefd, buf, MAXLINE)) > 0) {
    write(connfd, buf, n);
    }
    close(filefd);
    // Open and send the file

}


/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd, char *buf, size_t n) 
{
    
    // char buf[MAXLINE]; 
    // char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>"; 
    char httpmsg[MAXBUF];
    struct stat filestat;
    char request_uri[MAXLINE];
    int postActive = -1;

    //n = read(connfd, buf, MAXLINE);
    printf("server received the following request:\n%s\n",buf);
    if (n <= 0) {
        return;
    }
    
    // Retrieving request URI from buf ----
    char* getLine = strstr(buf, "GET ");
    char* postLine = strstr(buf, "POST ");
    if (getLine == NULL && postLine == NULL) {
        printf("Invalid request: No 'GET' or 'POST' line\n");
        return;
    }

    // Move the pointer past "GET "
    if (postLine == NULL) {
        getLine += 4;
    }
    if (getLine == NULL) {
        postActive = 1;
        postLine +=5;
    }

    // Find the end of the request URI (ends with a space)
    char* uriEnd;
    if (postLine == NULL) {
        uriEnd = strchr(getLine, ' ');
    }
    if (getLine == NULL) {
        uriEnd = strchr(postLine, ' ');
    }

    if (uriEnd == NULL) {
        printf("Invalid request: URI not found\n");
        return;
    }
    int uriLength;
    // Calculate the length of the URI
    if (postLine == NULL) {
        uriLength = uriEnd - getLine;
    }
    if (getLine == NULL) {
        uriLength = uriEnd - postLine;
    }
    //int uriLength = uriEnd - getLine;
    
    // Allocate a buffer to store the URI and copy it
    char requestURI[1024]; // Adjust the buffer size as needed
    if (postLine == NULL) {
        strncpy(requestURI, getLine, uriLength);
    }
    if (getLine == NULL) {
        strncpy(requestURI, postLine, uriLength);
    }
    //strncpy(requestURI, getLine, uriLength);
    requestURI[uriLength] = '\0'; // Null-terminate the string
    memmove(requestURI, requestURI + 1, strlen(requestURI));
    
    printf("Request URI: %s\n", requestURI);

    // Request URI retrieval complete ----

    

    // Parse the request to extract the Request URI

    if (strcmp(requestURI, "") == 0) {
        // Serve the root page
        serve_file(connfd, "www/index.html", postActive);
    } else if (strcmp(requestURI, "www") == 0) {
        // Serve another page based on the URI
        printf("\nIn this case\n");
        serve_file(connfd, "www/index.html", postActive);
    } else {
        // Handle other cases or return a 404 Not Found response
        printf("\nIn ekse case\n");
        // Add www/ for nested requests ----
        char prefix[] = "www/";
        if (strncmp(requestURI, prefix, sizeof(prefix)-1) != 0) {
            // "www/" is not present, so add it
            if (strlen(requestURI) + strlen(prefix) < sizeof(requestURI)) {
                memmove(requestURI + strlen(prefix), requestURI, strlen(requestURI) + 1);
                strncpy(requestURI, prefix, sizeof(prefix) - 1);
            } else {
                // Handle insufficient buffer space error
                printf("Buffer space insufficient for prefix addition.\n");
            }
        }
        printf("\nRequest in ekle case is %s\n", requestURI);
        // Adding www/ done ----
        serve_file(connfd, requestURI, postActive);
    }
}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */

