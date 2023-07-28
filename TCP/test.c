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

int checkfile(char* buffer){
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

long file_transfer(char* buffer){
    FILE *fp = fopen(buffer, "rb" );
    if (fp == NULL){
        perror("Error reading file");
        exit(1);
    }
    // struct stat st;
    fseek(fp,0,SEEK_END);
    long size = ftell(fp);
    printf("%ld \n",size);
    fseek(fp,0,SEEK_SET);
    memset(buffer,'\0',BUFFLEN);
    long offset = 0;
    while (offset!=size){
        size_t readnow = fread(buffer+offset, 1,1, fp);
        if (readnow == 0){
            printf("Read unsuccessful \n");
            free(buffer);
            fclose(fp);
            exit(1);
    }
            offset ++ ;
    }
    // printf("again: %s",buffer);
    
    fclose(fp);
    printf("Socket read complete ready to send \n");
    return size;
}

void file_transfer1(char* file, char* buffer, int size){
    FILE *fp = fopen(file, "wb+");
    if (fp == NULL){
        perror("Error reading file\n");
        exit(1);
    }
    long offset = 0;
    // printf("Size of file: %ld\n",size);
    // while (offset < size){
    for (int i =0; i<size;i++){
        size_t readnow = fwrite(buffer+offset, 1,1 , fp);
        if (readnow < 0){
            printf("Write unsuccessful \n");
            free(buffer);
            fclose(fp);
            exit(1);}
        offset = offset+readnow;
    }
    fclose(fp);
    printf("File write complete \n");
}

int main()
{
   char *buffer = (char* )malloc(BUFFLEN * sizeof(char));
    memset(buffer,'\0', BUFFLEN);
    strcpy(buffer,"anh.jpeg");
//    const char *buffer1 = "/home/phuongnam/text.txt";
//     if (access(buffer,F_OK)==0) printf("Exist");
//     else printf("No");

    char* path = "/home/phuongnam/transmit/";
    size_t len = strlen(path);
    char* path_buffer = malloc(len+strlen(buffer));
    strcpy(path_buffer,path);
    strcpy(path_buffer+len,buffer);
    memset(buffer,'\0',BUFFLEN);
    strcpy(buffer,path_buffer);

    long size = file_transfer(buffer);
    file_transfer1("anh.jpeg",buffer, size);
    free(path_buffer);
    free(buffer);
    char msg[123]="12354";

    printf("%lld",atoll(msg));
    return 0;

}