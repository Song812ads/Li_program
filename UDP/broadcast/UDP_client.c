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
#define BUFFLEN 50000
#define MAX_CLIENTS 1
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

int main(int argc, char **argv){
    // Thiết lập phương thức nhận dữ liêu và tạo kết nối đến server
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    int socketfd; 
    struct sockaddr_in serveradd;
    socklen_t serlen = sizeof(serveradd);
    unsigned char *buffer = (unsigned char* )malloc(BUFFLEN * sizeof(unsigned char));
    fd_set readfds;
    
    if (argc!=4){
        printf("Wrong type <server addresss> <server port>\n");
        exit(1);
    }

    // Socket create:
    if ((socketfd = socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("Socket create fail\n");
        exit(1);
    }
    else printf("Socket: %d \n",socketfd);

    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    char* port = argv[2];
    serveradd.sin_port = htons ( 5375 );
    serveradd.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    struct timeval tv;
    tv.tv_sec = 20;  /* 20 Secs Timeout */
    tv.tv_usec = 0;
    if(setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(tv)) < 0)
    {
        printf("Time Out\n");
        return -1;
    }
    const int on = 1;
    int ret = setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on));
    if (ret == -1) {
        perror("setsockopt error");
        return 0;
    }


    char* filename=NULL;
    size_t len_file = 0;
    ssize_t rdn;
    printf("Nhap file muon tai: ");
    if ((rdn = getline(&filename,&len_file,stdin))==-1){
        perror("Getline error");
        // break;
    }
    filename[strlen(filename)-1] = '\0';
    memset(buffer,'\0',BUFFLEN);
    strcpy(buffer,filename);
    if(sendto(socketfd,buffer,BUFFLEN,0, (struct sockaddr *)&serveradd, sizeof(serveradd))<0) {
        perror("Send error");
        exit(1);
    }
        FD_ZERO(&readfds);
        FD_SET(socketfd,&readfds);
        while(1){
        int ret = select(socketfd+1,&readfds,NULL,NULL,NULL);
        if ((ret < 0) && (errno!=EINTR)) 
        {
            printf("select error");
            exit(1);
        }
        else if (ret>0){
            printf("IP address is: %s\n", inet_ntoa(serveradd.sin_addr));
            printf("port is: %d\n", (int) ntohs(serveradd.sin_port));
            memset(buffer,'\0',BUFFLEN);           
            if (recvfrom(socketfd,buffer,BUFFLEN,0, (struct sockaddr *)&serveradd, &serlen)<0){
                perror("Recv error");
                exit(1);
            }
            if (strcmp(buffer,"Err")==0){
                printf("File not exist");
                break;
            }
        else  {
            ssize_t t = 0;
            long sz = 0;
            int op = open(filename, O_RDWR | O_CREAT , 0644); 
            lseek(op,0,SEEK_SET);
        while (1){
            char size[10];
            if ((recvfrom(socketfd,size,10,0,(struct sockaddr *)&serveradd, &serlen))<0){
                perror("Recv error");
                exit(1);
            }
            int ret = atol(size);
            writen(op,buffer,ret);
            if (ret==BUFFLEN){
            t++;
            sz = 0;
            lseek(op,t*BUFFLEN,SEEK_SET);
            memset(buffer,'\0',BUFFLEN);
            if ((ret = recvfrom(socketfd,buffer,BUFFLEN,0,(struct sockaddr *)&serveradd, &serlen))<0){
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
        }
        }
        }

    }
    free(buffer);
    return 0;
}