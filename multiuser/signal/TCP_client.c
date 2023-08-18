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
#define BUFFLEN 1000
typedef enum {FIRST, AFTER} file_mode;
char* filename=NULL;
int socketfd;



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


void signio_handler(int signo){
    size_t len_file = 0;
    ssize_t rdn;
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET; 
    lock.l_start = 0;        
    lock.l_len = 0;          
    lock.l_pid = getpid();
    char buffer[BUFFLEN];
    int ret = recv(socketfd, buffer, BUFFLEN, 0);
    if (ret == 0) {
        printf("Client disconnect\n");
        exit(1);
    }
    else if (ret<0){
        perror("Recv error");
        exit(1);
    }
    else {
    if (strcmp(buffer,"Err")==0){
            while (fcntl(1,F_GETLK,&lock) == F_UNLCK){;}
            printf("File not exist\n");
        }
        else  {
            ssize_t t = 0;
            long sz = 0;
            int op = open(filename, O_RDWR | O_CREAT , 0644); 
            lseek(op,0,SEEK_SET);
        while (1){
            writen(op,buffer,ret);
            if (ret==BUFFLEN){
            t++;
            sz = 0;
            lseek(op,t*BUFFLEN,SEEK_SET);
            memset(buffer,'\0',BUFFLEN);
            if ((ret = recv(socketfd,buffer,BUFFLEN,0))<0){
                perror("Recv error");
                exit(1);
            }
            }
            else {
            close(op);
            // close(socketfd);
            printf("Size from client: %ld\n",t*BUFFLEN+ret);
            break;
            }
        }}
}}


int main(int argc, char **argv){
    // Thiết lập phương thức nhận dữ liêu và tạo kết nối đến server
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    struct sockaddr_in serveradd;
    unsigned char *buffer = (unsigned char* )malloc((BUFFLEN+1) * sizeof(unsigned char));
    
    if (argc!=4){
        printf("Wrong type <server addresss> <server port>\n");
        exit(1);
    }

    // Socket create:
    if ((socketfd = socket(AF_INET, SOCK_STREAM,0))<0){
        perror("Socket create fail\n");
        exit(1);
    }
    else printf("Socket: %d \n",socketfd);

    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    char* port = argv[2];
    serveradd.sin_port = htons ( atoi(port) );
    if (inet_pton (AF_INET, argv[1], &serveradd.sin_addr) <= 0) {
        perror("Convert binary fail"); 
        close(socketfd);
        exit(1);
    }

    if (connect(socketfd,(struct sockaddr *)&serveradd, sizeof(serveradd))<0){
        perror("Connection fail");
        close(socketfd);
        exit(1);
    }

    int optval = 1;
    socklen_t optlen = sizeof(optval);
    if(setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
      perror("setsockopt()");
      close(socketfd);
      exit(EXIT_FAILURE);
    }
    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;
        if( setsockopt (socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0 )
        printf( "setsockopt fail\n" );


    if(fcntl(socketfd, F_SETFL, O_NONBLOCK|O_ASYNC))
        printf("Error in setting socket to async, nonblock mode");  
    
    signal(SIGIO, signio_handler); // assign SIGIO to the handler

    if (fcntl(socketfd, F_SETOWN, getpid()) <0)
        printf("Error in setting own to socket");

    while(1){
        size_t len_file = 0;
        ssize_t rdn;
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET; 
        lock.l_start = 0;        
        lock.l_len = 0;          
        lock.l_pid = getpid();
    while(1){
        int re = fcntl(1, F_GETLK,&lock);
        while (lock.l_type == F_UNLCK) {;}
        printf("Nhap file muon tai: ");
        if ((rdn = getline(&filename,&len_file,stdin))==-1){
            perror("Getline error");
            break;
        }
        filename[strlen(filename)-1] = '\0';
        if (strcmp(filename,"Q")==0){
            close(socketfd);
            exit(1);
        }
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,filename);
        if (send(socketfd,buffer,BUFFLEN,0)<0){
            perror("Send error");
            exit(1);
        }

        if (strcmp(filename,"A")==0){
            memset(buffer,'\0',BUFFLEN);
            if (recv(socketfd,buffer,BUFFLEN,0)<0){
                perror("Recv error");
                exit(1);
            }
            printf("File available: %s\n",buffer);
        }
        else break;
    }
    }
    free(buffer);
    return 0;
}