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
#define BUFFLEN 500
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

ssize_t  readn(int fd, void *vptr, size_t n)
{    size_t  nleft;
    ssize_t nread;
    char   *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return (-1);
        } else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
       ptr += nread;
    }
     return (n - nleft);         /* return >= 0 */
}

ssize_t  writen(int fd, const void *vptr, size_t n)
 {
    size_t nleft;
     ssize_t nwritten;
    const char *ptr;

  ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
               nwritten = 0;   /* and call write() again */
           return (-1);    /* error */
        }

      nleft -= nwritten;
        ptr += nwritten;
    }
  return (n);
}

long file_transfer(char* buffer, int t){
    int fp = open(buffer, O_RDONLY);
    if (fp == -1){
        perror("Error reading file\n");
        exit(1);
    }
    long offset = 0;
    while (offset < BUFFLEN){
        ssize_t readnow = pread(fp, buffer+offset, 1, t*BUFFLEN + offset);
        if (readnow == 0){
            break;
        }
        else offset = offset+readnow;
    }
    close(fp);
    printf("Continue sending from server part %d \n",t+1);

    return offset;
}

int checkfile(unsigned char* buffer){
    if (access(buffer, F_OK) == -1){
        printf("File don't exist\n");
        return 0;
    }
    else if (access(buffer,R_OK) == -1){
        printf("Cant read file\n");
        return 0;
    }
    else {
        printf("File prepare to read\n");
        return 1;
    }
}


void enable_keepalive(int sock) {
    int yes = 1;
    check(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) != -1);

    int idle = 1;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) != -1);

    int interval = 1;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int)) != -1);

    int maxpkt = 10;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int)) != -1);
}

int main(int argc, char **argv){
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);

    // Thiêt lập port và các phương thức cơ bản giao tiếp TCP/IP
    int serverSocketfd, clientSocketfd, valread;
    struct sockaddr_in serveradd, clientadd;
    unsigned char *buffer = (unsigned char* )malloc((BUFFLEN) * sizeof(unsigned char));
    int clientlength = sizeof(clientadd);
    struct timeval tv;
    // tv.tv_sec = 5;
    // tv.tv_usec = 0;
    ssize_t ti = 0; 
    long sz = 0;

    // Socket create:
    if ((serverSocketfd = socket(AF_INET, SOCK_STREAM,0))<0){
        perror("Socket create fail");
        exit(1);
    }

    else printf("Socket: %d \n",serverSocketfd);


    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    serveradd.sin_port = htons ( 6185 );
    serveradd.sin_addr.s_addr = htonl(INADDR_ANY);

    // int optval = 1;
    // socklen_t optlen = sizeof(optval);
    // if(setsockopt(serverSocketfd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen) < 0) {
    //   perror("setsockopt()");
    //   close(clientSocketfd);
    //   exit(EXIT_FAILURE);
    // }
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
    // memset(buffer,'\0',BUFFLEN);
    if ((clientSocketfd = accept(serverSocketfd, (struct sockaddr*) &clientadd, &clientlength))==-1){
        printf("Server accept fail");
        exit(1);
    }
    else printf("Server Accepted\n");
    int optval = 1;
    socklen_t optlen = sizeof(optval);
    if(setsockopt(clientSocketfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
      perror("setsockopt()");
      close(clientSocketfd);
      exit(EXIT_FAILURE);
    }
    printf("IP address is: %s\n", inet_ntoa(clientadd.sin_addr));
    printf("port is: %d\n", (int) ntohs(clientadd.sin_port));


start:
while(1){
    memset(buffer,'\0',BUFFLEN);
    int ret= read(clientSocketfd,buffer,BUFFLEN);
    if (ret <0){  
        perror("Rcv error");
        exit(1);
    }
    else if (ret ==0){
        printf("Client disconnect\n");
        close(clientSocketfd);
        goto begin;
    }
    if (strcmp(buffer,"A")==0){
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,"File");
        if (send(clientSocketfd,buffer,BUFFLEN,0)<0){
            perror("Send error");
            exit(1);
        }
    }
    else break;}
    printf("File client want: %s\n",buffer);
    char* path = "/home/phuongnam/transmit/";
    size_t len = strlen(path);
    char* path_buffer = malloc(len+strlen(buffer));
    memset(path_buffer,'\0',sizeof(path_buffer));
    strcpy(path_buffer,path);
    strcpy(path_buffer+len,buffer);
    if (checkfile(path_buffer)==0){
        printf("File dont exist");
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,"Err");
        if (send(clientSocketfd,buffer,BUFFLEN,0)<0){
            perror("Send error");
            break;
        }
        goto start;
    }
    
    else {
        int op = open(path_buffer, O_RDONLY);
        free(path_buffer);
        lseek(op,0,SEEK_SET);
        while (1){
        memset(buffer,'\0',BUFFLEN);
        sz = readn(op,buffer,BUFFLEN);
        printf("%ld\n",sz);

        if (send(clientSocketfd,buffer,sz,0)<0){
            perror("Send error1");
            exit(1);
        }
        if (sz < BUFFLEN){
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,"OK");
        if (send(clientSocketfd,buffer,BUFFLEN,0)<0){
            perror("Send err");
            exit(1);
        }
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
}


// break;
}
free(buffer);
close(serverSocketfd);
}