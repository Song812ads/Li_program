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
    
}

int main(int argc, char **argv){
    // Thiết lập phương thức nhận dữ liêu và tạo kết nối đến server
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    int socketfd; 
    struct sockaddr_in serveradd;
    unsigned char *buffer = (unsigned char* )malloc(BUFFLEN * sizeof(unsigned char));
    
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

    memset(buffer,'\0',BUFFLEN);                   
    char filename[40];
    memset(filename,'\0',40); 
    while (1){

    
        printf("Hello User! Press A to see all file available or enter filename: ");
        scanf("%s",buffer);
        if (send(socketfd,buffer,BUFFLEN,0)<0){
            exit(1);
        }
        strcpy(filename,buffer);
        memset(buffer,'\0',BUFFLEN);
        if(recv(socketfd,buffer,BUFFLEN,0)<0)
        {
            perror("Buffer content read failed");
            exit(1);
        }
        if (strcmp(filename,"A")==0) {
            printf("File from server: %s\n",buffer);
            break;}
        
    

    else {
        if ((strcmp(buffer,"Error")==0)){
        printf("File don't exist. Check again");
    }
    else {
        printf("File exist on server ready to download\n");
        long size = atol(buffer);
        // printf("%s",buffer);
        if(recv(socketfd,buffer,BUFFLEN,0)<0)
        {
            perror("Buffer content read failed");
            exit(1);
        }
        file_transfer(filename ,buffer,size,0,FIRST);
        printf("File write complete part %ld \n",size);
        break;
    }}}
    free(buffer);
    close(socketfd);
    return 0;
}