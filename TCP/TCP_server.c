#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <dirent.h>
#define BUFFLEN 1000
#define check(expr) if (!(expr)) { perror(#expr); kill(0, SIGTERM); }

void pipebroke()
{
    printf("\nBroken pipe: write to pipe with no readers\n");
}

void exithandler()
{
    printf("\nExiting....\n");
    exit(EXIT_FAILURE);
}

ssize_t  readn(int fd, void *vptr, size_t n){    
    size_t  nleft= n;; ssize_t nread;
    char   *ptr; ptr = vptr;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) nread = 0;      /* and call read() again */
            else  return (-1);
        } 
        else if (nread == 0) break;              /* EOF */
    nleft -= nread;
    ptr += nread; }
     return (n - nleft);         /* return >= 0 */
}

ssize_t  writen(int fd, const void *vptr, size_t n) {
    size_t nleft = n; ssize_t nwritten; const char *ptr;
    ptr = vptr;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
               nwritten = 0;   /* and call write() again */
           return (-1);    /* error */
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
  return (n);}

char *sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
	char	portstr[8];
	static char str[128];

	switch (sa->sa_family) {
	case AF_INET: {
		      struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		      if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str))
				      == NULL)
			      return NULL;
		      if (ntohs(sin->sin_port) != 0) {
			      snprintf(portstr, sizeof(portstr), ":%d", 
					      ntohs(sin->sin_port));
			      strcat(str, portstr);
		      }
		      return str;
	      }
	}
	return NULL;
}

 ssize_t readline(int fd, void *vptr, size_t maxlen){
    ssize_t n, rc;char    c, *ptr;
    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
      again:
        if ( (rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
           if (c == '\n')
                break;          /* newline is stored, like fgets() */
        } else if (rc == 0) {
           *ptr = 0;
            return (n - 1);     /* EOF, n - 1 bytes were read */
        } else {
            if (errno == EINTR)
                 goto again;
            return (-1);        /* error, errno set by read() */
         }
    }
    *ptr = 0;                   /* null terminate like fgets() */
    return (n);}

int checkfile(unsigned char* buffer){
    if (access(buffer, F_OK) == -1){
        return 0;
    }
    else if (access(buffer,R_OK) == -1){
        return 0;
    }
    else {
        return 1;
    }
}


void checkfolder(unsigned char* buffer){
    DIR *d;
    struct dirent *dir;
    d = opendir(buffer);
    memset(buffer,0,BUFFLEN);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
        strcat(buffer,dir->d_name);
        strcat(buffer,"     ");
        }
        closedir(d);
    }
}


int main(int argc, char **argv){
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);

    // Thiêt lập port và các phương thức cơ bản giao tiếp TCP/IP
    int serverSocketfd, clientSocketfd, valread;
    struct sockaddr_in serveradd, clientadd;
    char *buffer = (char*)malloc(BUFFLEN);
    int clientlength = sizeof(clientadd);
    struct timeval tv;

    if ((serverSocketfd = socket(AF_INET, SOCK_STREAM,0))<0){
        perror("Socket create fail");
        exit(1);
    }

    else printf("Socket: %d \n",serverSocketfd);


    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    serveradd.sin_port = htons ( 6185 );
    serveradd.sin_addr.s_addr = htonl(INADDR_ANY);

    const int enable = 1;
if (setsockopt(serverSocketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    error("setsockopt(SO_REUSEADDR) failed");

    if (bind (serverSocketfd, (struct sockaddr*) &serveradd, sizeof( serveradd))!=0){
        perror("Server bind fail");
        close(serverSocketfd);
        exit(1);
    }
    else printf("Binding...\n");
    if (listen(serverSocketfd,10)!=0)
    {
        perror("Server listen error");
        close(serverSocketfd);
        exit(1);
    }
    else printf("Listening...\n");
    bzero(&clientadd,sizeof(clientadd));
begin:
while(1){
    if ((clientSocketfd = accept(serverSocketfd, 
    (struct sockaddr*) &clientadd, &clientlength))==-1){
        printf("Server accept fail");
        exit(1);
    }
    else printf("Server Accepted\n");
    printf("Client: %s\n", sock_ntop((struct sockaddr*)&clientadd,
                                                INET_ADDRSTRLEN));
    char* path = "C:/cygwin64/home/MSI/storage/";
start:
while(1){
    memset(buffer,'\0',BUFFLEN);
    int ret = read(clientSocketfd,buffer,BUFFLEN);
    if (ret <0){  
        perror("Rcv error");
        exit(1);
    }
    else if (ret == 0){
        printf("Client disconnect\n");
        goto begin;
    }
    if (strcmp(buffer,"A")==0){
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,path);
        checkfolder(buffer);
        writen(clientSocketfd,buffer,strlen(buffer));
        goto start;
    }
    else break;
}
    printf("File client want: %s\n",buffer);
    size_t len = strlen(path);
    char* path_buffer = malloc(len+strlen(buffer));
    memset(path_buffer,'\0',sizeof(path_buffer));
    strcpy(path_buffer,path);
    strcpy(path_buffer+len,buffer);
    if (checkfile(path_buffer)==0){
        printf("File dont exist\n");
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,"Err");
        writen(clientSocketfd,buffer,strlen(buffer));
        goto start;
    }
    else {
        int op = open(path_buffer, O_RDONLY);
        free(path_buffer);
        lseek(op,0,SEEK_SET);
        ssize_t ti = 0; 
        long sz = 0;
        while (1){
        memset(buffer,'\0',BUFFLEN);
        sz = readn(op,buffer,BUFFLEN);
        writen(clientSocketfd,buffer,sz);
        if (sz < BUFFLEN){
        printf("Transmit: %ld\n",ti*BUFFLEN+sz);
        goto start;
        }
        else 
        {
            ti++;
            sz = 0;
            lseek(op,ti*BUFFLEN,SEEK_SET);
        }
    }  
}}
close(serverSocketfd);
}