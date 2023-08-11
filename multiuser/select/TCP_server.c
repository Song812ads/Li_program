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
#define MAX_CLIENTS 2

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

int main(int argc, char **argv){
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    int new_socket;
    fd_set readfds;
    // Thiêt lập port và các phương thức cơ bản giao tiếp TCP/IP
    int serverSocketfd,  valread, sd;
    int clientSocketfd[MAX_CLIENTS];
    struct sockaddr_in serveradd, clientadd;
    char *buffer = (char* )malloc(BUFFLEN * sizeof(char));
    int clientlength = sizeof(clientadd);
    struct timeval tv;
    tv.tv_sec = 200;
    tv.tv_usec = 0;

    for (int i=0;i < MAX_CLIENTS + 1;i++){
        clientSocketfd[i] = 0;
    }
    // Socket create:
    if ((serverSocketfd = socket(AF_INET, SOCK_STREAM,0))<0){
        perror("Socket create fail");
        exit(1);
    }

    else printf("Socket: %d \n",serverSocketfd);


    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    serveradd.sin_port = htons ( 6315 );
    serveradd.sin_addr.s_addr = htonl(INADDR_ANY);

   const int enable = 1;
if (setsockopt(serverSocketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");
if (setsockopt(serverSocketfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");
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

while (1){
    FD_ZERO(&readfds);
    FD_SET(serverSocketfd,&readfds);
    int max_sd = serverSocketfd;

    for (int i =0; i<MAX_CLIENTS; i++){
        sd = clientSocketfd[i];
        if (sd>0){
            FD_SET(sd, &readfds);
        }
        if (sd > max_sd){
            max_sd = sd;
        }
    }

    int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    if ((activity < 0) && (errno!=EINTR)) 
    {
        printf("select error");
    }

    if (FD_ISSET(serverSocketfd, &readfds)) 
        {
            if (( new_socket = accept(serverSocketfd, (struct sockaddr *)&clientadd, (socklen_t*)&clientlength))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            struct timeval tv;
            tv.tv_sec = 200;  /* 20 Secs Timeout */
            tv.tv_usec = 0;
            if(setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(tv)) < 0)
            {
                printf("Time Out\n");
                return -1;
            }

            if(setsockopt(new_socket, SOL_SOCKET, SO_SNDTIMEO,(char *)&tv,sizeof(tv)) < 0)
            {
                printf("Time Out\n");
                return -1;
            }
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(clientadd.sin_addr) 
            , ntohs(clientadd.sin_port));
              
            //add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) 
            {
                //if position is empty
                if( clientSocketfd[i] == 0 )
                {
                    clientSocketfd[i] = new_socket;

                    printf("Adding to list of sockets as %d\n" , i+1);
                    break;
                }
            }
        }

        for (int i =0; i<MAX_CLIENTS; i++){
            sd = clientSocketfd[i];
            if (FD_ISSET(sd,&readfds)){
                if (valread = read(sd,buffer,BUFFLEN)==0){
                    getpeername(sd , (struct sockaddr*)&clientadd , (socklen_t*)&clientlength);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(clientadd.sin_addr) , ntohs(clientadd.sin_port));
                    close(sd);
                    clientSocketfd[i] = 0;
                }
                else{
                    // do{
                    if (strcmp(buffer,"A")==0){
                        memset(buffer,'\0',BUFFLEN);
                        strcpy(buffer,"File");
                        if (send(sd,buffer,BUFFLEN,0)<0){
                            perror("Send error");
                            exit(1);
                        }
                    }
                    else {
                    // else break;
                    // memset(buffer,'\0',BUFFLEN);
                    // if (recv(sd,buffer,BUFFLEN,0)<0){  
                    //     perror("Rcv error");
                    //     exit(1);
                    // }
                    // }while(1);
                    printf("File client want: %s\n",buffer);
                    char* path = "/home/phuongnam/transmit/";
                    size_t len = strlen(path);
                    char* path_buffer = malloc(len+strlen(buffer));
                    memset(path_buffer,'\0',sizeof(path_buffer));
                    strcpy(path_buffer,path);
                    strcpy(path_buffer+len,buffer);
                    char* siz = malloc(10*sizeof(char));
                    int sz = 0, ti = 0;
                    if (checkfile(path_buffer)==0){
                        printf("File dont exist");
                        memset(buffer,'\0',BUFFLEN);
                        strcpy(buffer,"Err");
                        if (send(sd,buffer,BUFFLEN,0)<0){
                            perror("Send error");
                            break;
                        }
                    }
                    else {
                        int op = open(path_buffer, O_RDONLY);
                        free(path_buffer);
                        lseek(op,0,SEEK_SET);
                        while (1){
                        memset(buffer,'\0',BUFFLEN);
                        sz = readn(op,buffer,BUFFLEN);
                        // printf("%s\n",buffer);
             
                        if (send(sd,buffer,sz,0)<0){
                            perror("Send error2");
                            exit(1);
                        }


                        if (sz < BUFFLEN){
                            memset(buffer,'\0',BUFFLEN);
                            printf("Client disconnect. Transmit: %ld\n",ti*BUFFLEN+sz);
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
                close(sd);
                clientSocketfd[i] = 0;
                }
                free(siz);
        }}}}}
    free(buffer);
    close(serverSocketfd);
}