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
#define BUFFLEN 256
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
    unsigned char *buffer = (unsigned char* )malloc(BUFFLEN * sizeof(unsigned char));
    int ret = 0;
    
    if (argc!=3){
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
    
    // int optval = 1;
    // socklen_t optlen = sizeof(optval);
    // if(setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
    //   perror("setsockopt()");
    //   close(socketfd);
    //   exit(EXIT_FAILURE);
    // }

    while(1){
        char* filename=NULL;
        size_t len_file = 0;
        ssize_t rdn;
    // while(1){
        printf("Nhap file muon tai: ");
        if ((rdn = getline(&filename,&len_file,stdin))==-1){
            perror("Getline error");
            break;
        }
        filename[strlen(filename)-1] = '\0';
        
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,filename);
        if (send(socketfd,buffer,BUFFLEN,0)<0){
            perror("Send error");
            exit(1);
        }

    //     if (strcmp(filename,"A")==0){
    //         memset(buffer,'\0',BUFFLEN);
    //         if (recv(socketfd,buffer,BUFFLEN,0)<0){
    //             perror("Recv error");
    //             exit(1);
    //         }
    //         printf("File available: %s\n",buffer);
    //     }
    //     else break;
    // }
        memset(buffer,'\0',BUFFLEN);
        if ((recv(socketfd,buffer,BUFFLEN,0))<0){
            perror("Recv error");
            exit(1);
            // if (a==0) exit(1);
        }
        //  printf("%s\n",buffer);
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
            sz = atol(buffer);
            // printf("%ld\n",sz);

            while (1){
            memset(buffer,'\0',BUFFLEN);
            if ((ret = recv(socketfd,buffer,BUFFLEN,0))<0){
                perror("Recv error");
                exit(1);
            }
            
            if (ret == 0){
                printf("Return: %d\n",ret);
                char message[] = "Again";
                ret = send(socketfd,message,sizeof(message),0);
            }
            else {
                printf("Return: %d\n",ret);
                char mes[] = "Ok"; 
                ret = send(socketfd,mes,sizeof(mes),0);
                break;
            }
            }}
            writen(op,buffer,sz);
            // printf("%ld\n",sz);
            if (sz>=BUFFLEN){
            t++;
            sz = 0;
            lseek(op,t*BUFFLEN,SEEK_SET);
            memset(buffer,'\0',BUFFLEN);
            if (recv(socketfd,buffer,BUFFLEN,0)<0){
                perror("Recv error");
                exit(1);
            }
            else {
            close(op);
            close(socketfd);
            printf("Size from client: %ld\n",t*BUFFLEN+sz);
            break;
            }
        }
        
        }
        break;
        }

    free(buffer);
    return 0;
}