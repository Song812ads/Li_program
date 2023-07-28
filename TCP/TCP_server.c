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
#define BUFFLEN 5000

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
    FILE *fp = fopen(buffer, "rb" );
    if (fp == NULL){
        perror("Error reading file");
        exit(1);
    }
    // struct stat st;
    fseek(fp,(t)*BUFFLEN,SEEK_SET);
    // long size = ftell(fp);
    // printf("%ld \n",size);
    // fseek(fp,0,SEEK_SET);
    // memset(buffer,'\0',BUFFLEN);
    long offset = 0;
    while  (offset<BUFFLEN){
        size_t readnow = fread(buffer+offset, 1,1, fp);
        if (readnow == 0) break;
        else offset ++ ;
    }
    // printf("again: %s",buffer);
    
    fclose(fp);
    printf("Socket read complete part %d ready to send \n",t);
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

int main(int argc, char **argv){
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    int serverSocketfd, clientSocketfd, valread;
    struct sockaddr_in serveradd, clientadd;
    char *buffer = (char* )malloc(BUFFLEN * sizeof(char));
    int clientlength = sizeof(clientadd);

    // Socket create:
    if ((serverSocketfd = socket(AF_INET, SOCK_STREAM,0))<0){
        perror("Socket create fail");
        exit(1);
    }
    else printf("Socket: %d \n",serverSocketfd);
    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    serveradd.sin_port = htons ( 5585 );
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
    if ((clientSocketfd = accept(serverSocketfd, (struct sockaddr*) &clientadd, &clientlength))==-1){
        printf("Server accept fail");
        exit(1);
    }
    else printf("Server Accepted\n");

    memset(buffer,'\0', BUFFLEN);
    if (valread=recv(clientSocketfd,buffer,BUFFLEN,0)<0){
        printf("Receive error");
        memset(buffer,'\0', BUFFLEN);
        strcpy(buffer,"Error");
        if (send(clientSocketfd,buffer,BUFFLEN,0)<0){
            printf("Fail to send receive signal");
            free(buffer);
            exit(1);
        }}

    char* path = "/home/phuongnam/transmit/";
    size_t len = strlen(path);
    char* path_buffer = malloc(len+strlen(buffer));
    memset(path_buffer,'\0',sizeof(path_buffer));
    strcpy(path_buffer,path);
    strcpy(path_buffer+len,buffer);

    if (checkfile(path_buffer)==0){
        printf("Error access file\n");
        memset(buffer,'\0', BUFFLEN);
        strcpy(buffer,"Error");
        if (send(clientSocketfd,buffer,BUFFLEN,0)<0){
            printf("Fail to send access error signal");
            free(buffer);
            exit(1); 
    }}

    else {
        memset(buffer,'\0',BUFFLEN);                   
        strcpy(buffer,"Success");
        if (send(clientSocketfd,buffer,BUFFLEN,0)<0){
            printf("Fail to send success read file signal");  
            free(buffer);
            exit(1);
        }

        memset(buffer,'\0',BUFFLEN);                   
        if(recv(clientSocketfd,buffer,BUFFLEN,0)<0)
    {
        printf(" Knowing the status of the file on server side failed\n");
        perror("recv failed");
        exit(1);
    }
        if (strcmp(buffer,"Ready")==0){
        int t = 0;
        while(1){
            memset(buffer,'\0',BUFFLEN);
            strcpy(buffer,path_buffer);
            long  size = file_transfer(buffer,t);
            
            // if (size == BUFFLEN) t++;
            char *msg = malloc(30*sizeof(char));
            sprintf(msg,"%ld",size);
            printf("Size from server: %ld \n",size);
            if (send(clientSocketfd,msg,strlen(msg),0)<0){
                printf("Fail to send file read");  
                free(buffer);
                exit(1);
            }
            memset(msg,'\0',30);                   
            if(recv(clientSocketfd,msg,30,0)<0)
        {
            printf(" Knowing the status of the file on server side failed\n");
            perror("recv failed");
            exit(1);
        }

            if (strcmp(msg,"size") == 0){
            if (send(clientSocketfd,buffer,BUFFLEN,0)<0){
                printf("Fail to send file read");  
                free(buffer);
                exit(1);
                }
            memset(buffer,'\0',BUFFLEN); 
            if(recv(clientSocketfd,buffer,BUFFLEN,0)<0)
            {
                perror("Buffer content read failed");
                exit(1);
            }
            if (strcmp(buffer,"Complete") == 0){
                if (size == BUFFLEN) t++;
                else {
                    printf("server send all file content");
                    break;
                }
            }
            
            }}
            }}
    free(buffer);
    free(path_buffer);
    close(clientSocketfd);
    close(serverSocketfd);

}