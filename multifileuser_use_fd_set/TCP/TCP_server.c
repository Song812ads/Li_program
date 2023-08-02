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
#define BUFFLEN 50000
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
    

    return offset;
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
    for (int i =0; i<MAX_CLIENTS+1; i++){
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
                    setsockopt(clientSocketfd[i], SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                    char* path = "/home/phuongnam/transmit/"; 
                    if (strcmp(buffer,"A")==0){
                        memset(buffer,'\0',BUFFLEN);
                        strcpy(buffer, path);
                        checkfolder(buffer);        
                        if (send(clientSocketfd[i],buffer,BUFFLEN,0)<0){
                            printf("Fail to send access error signal");
                            free(buffer);
                            close(serverSocketfd);
                            exit(1); 
                    }
                        memset(buffer,'\0',BUFFLEN);
                        if(recv(clientSocketfd[i],buffer,BUFFLEN,0)<0) exit(1);      
                         
                    }
                    else{
                    size_t len = strlen(path);
                    char* path_buffer = malloc(len+strlen(buffer));
                    memset(path_buffer,'\0',sizeof(path_buffer));
                    strcpy(path_buffer,path);
                    strcpy(path_buffer+len,buffer);
                    if (checkfile(path_buffer)==0){
                        memset(buffer,'\0', BUFFLEN);
                        strcpy(buffer, "Error");
                        if (send(clientSocketfd[i],buffer,BUFFLEN,0)<0){
                            printf("Fail to send access error signal");
                            free(buffer);
                            close(serverSocketfd);
                            exit(1); 
                    }}
                    else {
                            memset(buffer,'\0',BUFFLEN);
                            strcpy(buffer,path_buffer);
                            long  size = file_transfer(buffer,0);
                            sprintf(buffer,"%ld",size);
                            if (send(clientSocketfd[i],buffer,BUFFLEN,0)<0){
                                printf("Fail to send file read");  
                                free(buffer);
                                close(serverSocketfd);
                                exit(1);
                             }
                            memset(buffer,'\0',BUFFLEN);
                            strcpy(buffer,path_buffer);
                            size = file_transfer(buffer,0);
                            printf("Sending from server size %ld \n",size);
                            if (send(clientSocketfd[i],buffer,BUFFLEN,0)<0){
                                printf("Fail to send file read");  
                                free(buffer);
                                close(serverSocketfd);
                                exit(1);
                             }
                    }
                    free(path_buffer);
                }
                clientSocketfd[i] = 0;
                close(clientSocketfd[i]);
                
                }}}}
    free(buffer);
    close(serverSocketfd);
}