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
#include <poll.h>

#define BUFFLEN 500
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
        return 0;
    }
    else if (access(buffer,R_OK) == -1){
        return 0;
    }
    else {
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

struct pollfd pollfds[MAX_CLIENTS+1];
pollfds[0].fd = serverSocketfd;
pollfds[0].events = POLLIN | POLLPRI;
int useClient = 0;
while (1){
    int pollResult = poll(pollfds,useClient+1,0);
    if (pollResult>0){
        if (pollfds[0].revents & POLLIN){
            int client_socket = accept(serverSocketfd,(struct sockaddr *)&clientadd, &clientlength);  
            printf("Client accepted: %s\n", inet_ntoa(clientadd.sin_addr));
            for (int i = 1; i<MAX_CLIENTS+1;i++){
                if (pollfds[i].fd == 0){
                    pollfds[i].fd = client_socket;
                    pollfds[i].events = POLLIN | POLLPRI;
                    useClient++;
                    break;
                }
            }
        }

    for (int i =1; i<MAX_CLIENTS+1;i++){

            if (pollfds[i].revents & POLLHUP){
                printf("Client disconnected: %s\n", inet_ntoa(clientadd.sin_addr));
            }

            else if (pollfds[i].revents & POLLIN){
                    int sd = pollfds[i].fd;
                    memset(buffer,'\0',BUFFLEN);
                    int val = recv(sd,buffer,BUFFLEN,0);
                    if (val==0){
                        printf("Client disconnected: %s\n", inet_ntoa(clientadd.sin_addr));
                        close(sd);
                        pollfds[i].fd = 0;
                        break;
                    }
                    else if (val<0){
                        perror("Recv fail");
                        exit(1);
                    }
                    else {
                    if (strcmp(buffer,"A")==0){
                        memset(buffer,'\0',BUFFLEN);
                        strcpy(buffer,"File");
                        if (send(sd,buffer,BUFFLEN,0)<0){
                            perror("Send error");
                            exit(1);
                        }
                    }
                    else {
                    printf("File client want: %s\n",buffer);
                    char* path = "/home/phuongnam/transmit/";
                    size_t len = strlen(path);
                    char* path_buffer = malloc(len+strlen(buffer));
                    memset(path_buffer,'\0',sizeof(path_buffer));
                    strcpy(path_buffer,path);
                    strcpy(path_buffer+len,buffer);
                    int sz = 0, ti = 0;
                    if (checkfile(path_buffer)==0){
                        printf("File dont exist\n");
                        memset(buffer,'\0',BUFFLEN);
                        strcpy(buffer,"Err");
                        if (send(sd,buffer,strlen(buffer),0)<0){
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
                        // printf("%d\n",sz);
                        if (send(sd,buffer,sz,0)<0){
                            perror("Send error2");
                            exit(1);
                        }

                        if (sz < BUFFLEN){
                            printf("Transmit: %ld\n",ti*BUFFLEN+sz);
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
            }}}
            else if (pollfds[i].revents & POLLERR){
                perror("Poll");
            }}
        }
    }                   
    free(buffer);
    close(serverSocketfd);
}