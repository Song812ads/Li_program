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

void file_transfer(char* file, char* buffer, long size, int t, file_mode mode){
    int fp = open(file, O_RDWR | O_APPEND | O_CREAT | O_SYNC, 0644);
    if (fp == -1){
        perror("Error writing file\n");
        exit(1);
    }
    off_t offset = 0;
    for (int i=0; i < size; i++){
        ssize_t readnow = pwrite(fp, buffer + offset, 1,t*BUFFLEN + offset);
        if (readnow < 0){
            printf("Read unsuccessful \n");
            free(buffer);
            close(fp);
            exit(1);
        }
        offset = offset+readnow;
    }
    close(fp);
    printf("File write complete part %d \n",t+1);
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

int main(int argc, char **argv){
    // Thiết lập phương thức nhận dữ liêu và tạo kết nối đến server
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    int socketfd; 
    struct sockaddr_in serveradd;
    unsigned char *buffer = (unsigned char* )malloc((BUFFLEN+1) * sizeof(unsigned char));
    
    if (argc!=3){
        printf("Wrong type <server addresss> <server port>\n");
        exit(1);
    }

    // Socket create:
    if ((socketfd = socket(AF_INET, SOCK_STREAM,0))<0){
        perror("Socket create fail\n");
        exit(1);
    }

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
    
    int maxfd;
    fd_set fset;
    while(1){ 
        fputs("Nhap file muon tai: \n",stdout);
        char filename[100];
        FD_ZERO(&fset);
        FD_SET(fileno(stdin),&fset);
        FD_SET(socketfd,&fset);
        maxfd = fileno(stdin)>socketfd ? fileno(stdin) : socketfd;
        select(maxfd+1,&fset,NULL,NULL,NULL);
        if (FD_ISSET(socketfd,&fset)){
            if (readline(socketfd,buffer,BUFFLEN)==0){
                printf("Server disconnected\n");
                close(socketfd);
                exit(1);
            }
        }
        else if (FD_ISSET(fileno(stdin),&fset)){   
        fgets(filename, 20, stdin);
        filename[strlen(filename)-1] = '\0';
        if (strcmp(filename,"Q")==0){
            close(socketfd);
            exit(1);
        }
        writen(socketfd,filename,strlen(filename));

        if (strcmp(filename,"A")==0){
            memset(buffer,'\0',BUFFLEN);
            int ret = read(socketfd,buffer,BUFFLEN);
            if (ret == 0){
                printf("Server disconnected\n");
                exit(1);
            }
            else if (ret<0){
                perror("Read fail");
                exit(1);
            }
            printf("File available: %s\n",buffer);
        }
    else {
        int ret = 0;
        memset(buffer,'\0',BUFFLEN);
        if ((ret = recv(socketfd,buffer,BUFFLEN,0))<0){
            perror("Recv error");
            exit(1);
            // if (a==0) exit(1);
        }
        //  printf("%s\n",buffer);
        if (strcmp(buffer,"Err")==0){
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
            printf("Size from client: %ld\n",t*BUFFLEN+ret);
            break;
            }
        }
        
        }
        // break;
        }}
}
    free(buffer);
    return 0;
}
