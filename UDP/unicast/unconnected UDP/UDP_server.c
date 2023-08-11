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
#include <netdb.h>
#include <netinet/tcp.h>
#define BUFFLEN 1000

void pipebroke()
{
    printf("\nBroken pipe: write to pipe with no readers\n");
}

void exithandler()
{
    printf("\nExiting....\n");
    exit(EXIT_FAILURE);
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
    int fp = open(buffer, O_RDONLY | O_NONBLOCK);
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
    return offset;
}


int main(int argc, char **argv){
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    
    // Thiêt lập port và các phương thức cơ bản giao tiếp TCP/IP
    int serverSocketfd;
    struct sockaddr_in serveradd, clientadd;
    char *buffer = (char* )malloc(BUFFLEN * sizeof(char));
    socklen_t cli_ad_sz = sizeof(clientadd);
    char *hostaddr;
    struct hostent* host; 
    char domain[512];
    int hostname;
    int val;
    if ((serverSocketfd = socket(AF_INET, SOCK_DGRAM,0))<0){
        perror("Socket create fail");
        exit(1);
    }   

    else printf("Socket created \n");
    bzero (&serveradd, sizeof(serveradd));
    bzero (&clientadd, sizeof(clientadd));
    serveradd.sin_family = AF_INET;
    serveradd.sin_port = htons ( 5175 );
    serveradd.sin_addr.s_addr = htonl(INADDR_ANY);

    const int enable = 1;
    if (setsockopt(serverSocketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (bind (serverSocketfd, (struct sockaddr*) &serveradd, sizeof( serveradd))!=0){
        perror("Server bind fail");
        close(serverSocketfd);
        exit(1);
    }
    else printf("Binding...\n");

    //     int optval = 1;
    // socklen_t optlen = sizeof(optval);
    // if(setsockopt(serverSocketfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
    //   perror("setsockopt()");
    //   close(serverSocketfd);
    //   exit(EXIT_FAILURE);
    // }
    struct timeval tv;
    tv.tv_sec = 20;  /* 20 Secs Timeout */
    tv.tv_usec = 0;
    if(setsockopt(serverSocketfd, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(tv)) < 0)
    {
        printf("Time Out\n");
        return -1;
    }

    while (1){
        memset(buffer,'\0',BUFFLEN);
        if ((recvfrom(serverSocketfd, buffer, BUFFLEN, 0, (struct sockaddr *) &clientadd, &cli_ad_sz))<0){
            perror("Recv error");
            exit(1);
        }
        printf("IP address is: %s\n", inet_ntoa(clientadd.sin_addr));
        printf("port is: %d\n", (int) ntohs(clientadd.sin_port));    
    while(1){
        if (strcmp(buffer,"A")==0){
            memset(buffer,'\0',BUFFLEN);
            strcpy(buffer,"File");
            
            if ((sendto(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr *) &clientadd, cli_ad_sz))<0){
                perror("Send error");
                exit(1);
            }
            memset(buffer,'\0',BUFFLEN);
            if ((recvfrom(serverSocketfd, buffer, BUFFLEN, 0, (struct sockaddr *) &clientadd, &cli_ad_sz))<0){
                perror("Recv error");
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
    char* siz = malloc(10*sizeof(char));
    if (checkfile(path_buffer)==0){
        printf("File dont exist");
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,"Err");
        if ((sendto(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr *) &clientadd, cli_ad_sz))<0){
            perror("Send error");
            break;
        }
    }
    else {
        int op = open(path_buffer, O_RDONLY);
        free(path_buffer);
        lseek(op,0,SEEK_SET);
        int ti = 0;
        int sz = 0;
        while (1){
            memset(buffer,'\0',BUFFLEN);
            sz = readn(op,buffer,BUFFLEN);

            if ((sendto(serverSocketfd,buffer,sz,0, (struct sockaddr *) &clientadd, cli_ad_sz))<0){
                perror("Send error1");
                exit(1);
            }


            if (sz < BUFFLEN){
                    printf("Client disconnect. Transmit: %ld\n",ti*BUFFLEN+sz);
                    // close(serverSocketfd);
                    close(op);
                    break;
                
            }
            else 
            {
                ti++;
                sz = 0;
                lseek(op,ti*BUFFLEN,SEEK_SET);
            }
        }  
    }
free(siz);
// break;
}
free(buffer);
close(serverSocketfd);
}