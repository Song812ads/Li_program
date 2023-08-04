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
typedef enum {FIRST, AFTER} file_mode;


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

// long file_transfer(char* buffer, int t){
//     int fp = open(buffer, O_RDONLY);
//     if (fp == -1){
//         perror("Error reading file\n");
//         exit(1);
//     }
//     long offset = 0;
//     while (offset < BUFFLEN){
//         ssize_t readnow = readn(fp, buffer+offset, 1);
//         if (readnow == 0){
//             break;
//         }
//         else offset = offset+readnow;
//     }
//     close(fp);
//     printf("File read complete \n");
//     return offset;
// }

 ssize_t                         /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
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

void file_transfer1(char* file, char* buffer, long size, int t, file_mode mode){
    int fp = open(file, O_RDWR | O_APPEND | O_CREAT | O_SYNC, 0644);
    if (fp == -1){
        perror("Error writing file\n");
        exit(1);
    }
    off_t offset = 0;
    for (int i=0; i < size; i++){
        ssize_t readnow = writen(fp, buffer + offset, 1);
        if (readnow < 0){
            printf("Read unsuccessful \n");
            free(buffer);
            close(fp);
            exit(1);
        }
        offset = offset+readnow;
    }
    close(fp);
    printf("File write complete \n");
}

// long file_transfer(char* buffer, int t){
//     FILE *fp = fopen(buffer, "rb" );
//     if (fp == NULL){
//         perror("Error reading file");
//         exit(1);
//     }
//     // struct stat st;
//     fseek(fp,(t)*BUFFLEN,SEEK_SET);
//     // long size = ftell(fp);
//     // printf("%ld \n",size);
//     // fseek(fp,0,SEEK_SET);
//     // memset(buffer,'\0',BUFFLEN);
//     long offset = 0;
//     while  (offset<BUFFLEN){
//         size_t readnow = readn(buffer+offset, fp,1);
//         if (readnow == 0) break;
//         else offset ++ ;
//     }
//     // printf("again: %s",buffer);
    
//     fclose(fp);
//     printf("Socket read complete ready to send \n");
//     return offset;
// }

// void file_transfer1(char* file, char* buffer, long size, int t, file_mode mode){
//     FILE *fp;
//     if (mode == AFTER) {fp = fopen(file, "ab+");}
//     else if (mode == FIRST) {fp = fopen(file, "wb+");}

//     if (fp == NULL){
//         perror("Error reading file\n");
//         exit(1);
//     }
//     long offset = 0;
//     fseek(fp,t*BUFFLEN,SEEK_SET);
//     // printf("Size of file: %ld\n",size);
//     // while (offset < size){
//     for (int i =0; i<size;i++){
//         size_t readnow = fwrite(buffer+offset, 1,1 , fp);
//         if (readnow < 0){
//             printf("Write unsuccessful \n");
//             free(buffer);
//             fclose(fp);
//             exit(1);}
//         offset = offset+readnow;
//     }
//     fclose(fp);
//     printf("File write complete \n");
// }
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
    // strcpy(buffer,path_buffer);
    int open1 = open(path_buffer, O_RDONLY);
    long size = readn(open1,buffer, BUFFLEN);
    file_mode mode = FIRST;
    printf("%s",buffer);
    int open2 = open("anh.jpeg", O_RDWR | O_APPEND | O_CREAT | O_SYNC, 0644);
    size = writen(open2,buffer,BUFFLEN);
    free(path_buffer);
    free(buffer);
    char msg[123]="12354";

    printf("%lld\n",atoll(msg));
    int a = 5000;
    if (a == BUFFLEN) printf("TRue"); else printf("False");
    // printf({a});
    return 0;

}