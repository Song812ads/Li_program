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
#include <netdb.h>
#include <netinet/tcp.h>
#define BUFFLEN 500

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

    // Thiêt lập port và các phương thức cơ bản giao tiếp TCP/IP
    int serverSocketfd;
    struct sockaddr_in serveradd, clientadd;
    char *buffer = (char* )malloc(BUFFLEN * sizeof(char));
    int cli_ad_sz = sizeof(clientadd);
    char *hostaddr;
    struct hostent* host; 
     // noi store cac file

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    // Socket create:
    if ((serverSocketfd = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP))<0){
        perror("Socket create fail");
        exit(1);
    }   

    else printf("Socket created \n");


    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    serveradd.sin_port = htons ( 6365 );
    serveradd.sin_addr.s_addr = htonl(INADDR_ANY);
    //   if( setsockopt (serverSocketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0 )
    //     printf( "setsockopt fail\n" );
    //   if( setsockopt (serverSocketfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0 )
    //     printf( "setsockopt fail\n" ) ;
    if (bind (serverSocketfd, (struct sockaddr*) &serveradd, sizeof( serveradd))!=0){
        perror("Server bind fail");
        close(serverSocketfd);
        exit(1);
    }
    else printf("Binding...\n");
    while (1){
        bzero(buffer,BUFFLEN);
        if ((recvfrom(serverSocketfd, buffer, BUFFLEN, 0, (struct sockaddr *) &clientadd, &cli_ad_sz))<0){
            perror("Recv error");
        }
        host = gethostbyaddr((const char *)&clientadd.sin_addr.s_addr, 
              sizeof(clientadd.sin_addr.s_addr), AF_INET);
        if (host == NULL) perror("ERROR on gethostbyaddr");
        hostaddr = inet_ntoa(clientadd.sin_addr);
        if (hostaddr == NULL) perror("ERROR on inet_ntoa\n");
        printf("Server receive request from %s (%s)\n",host->h_name, hostaddr);

    }

    // if (bind (serverSocketfd, (struct sockaddr*) &serveradd, sizeof( serveradd))!=0){
    //     perror("Server bind fail");
    //     close(serverSocketfd);
    //     exit(1);
    // }
    // else printf("Binding...\n");


    //     memset(buffer,'\0', BUFFLEN);
    //     if(recvfrom(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr*)&clientadd, &cli_ad_sz)<0)
    //     {
    //         printf(" Knowing the status of the file on server side failed\n");
    //         perror("recv failed");
    //         exit(1);
    //     }
    //     char* path = "/home/phuongnam/transmit/";
    //     size_t len = strlen(path);
    //     char* path_buffer = malloc(len+strlen(buffer));
    //     memset(path_buffer,'\0',sizeof(path_buffer));
    //     strcpy(path_buffer,path);
    //     strcpy(path_buffer+len,buffer);
    //     strcpy(buffer,path_buffer);
        
    //     if (checkfile(buffer)==0){
    //         printf("Error access file\n");
    //         memset(buffer,'\0', BUFFLEN);
    //         strcpy(buffer,"Error");
    //         if (sendto(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr*)&clientadd,cli_ad_sz)<0){
    //             printf("Fail to send access error signal");
    //             free(buffer);
    //             exit(1); 
    //     }}

    //     else {
    //         memset(buffer,'\0', BUFFLEN);
    //         strcpy(buffer,"Success");
    //         if (sendto(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr*)&clientadd,cli_ad_sz)<0){
    //             printf("Fail to send access error signal");
    //             free(buffer);
    //             exit(1); }
        
    //     int t = 0; // ghi lại số lần dẵ chuyển
    // // Tạo vòng lặp gửi dữ liệu với từng buffer. VD 1 file 4k3 sẽ gửi bằng 9 lần với 8 lần max 500 byte và 1 lần 300 bytes. 
    //     while (1){
    //         memset(buffer,'\0',BUFFLEN);
    //         strcpy(buffer,path_buffer);
    //         printf("Continue sending from server part %d \n",t+1);
    //         long  size = file_transfer(buffer,t);
    //         sprintf(buffer,"%ld",size);
    // // Gửi kích thuốc khi kích thước =500 có thể hiểu là vẫn còn thêm thông tin. client sẽ chỉnh mode mở là append nếu có 1 lần kích thuốc =500.
    // // Dê mode mở bth nếu như k có lần nào 500 byte được chuyển
    //         if (sendto(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr*)&clientadd,sizeof(struct sockaddr))<0){
    //             printf("Fail to send file read");  
    //             free(buffer);
    //             close(serverSocketfd);
    //             exit(1);
    //         }

    //         memset(buffer,'\0',BUFFLEN);                   
    //         if(recvfrom(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr*)&clientadd, &cli_ad_sz)<0)
    //     {
    //         printf(" Knowing the status of the file on server side failed\n");
    //         perror("recv failed");
    //         exit(1);
    //     }
    //         // printf("%s\n",buffer);
    //         if (strcmp(buffer,"size") == 0){
    //         memset(buffer,'\0',BUFFLEN);
    //         strcpy(buffer,path_buffer);
    //         long  size = file_transfer(buffer,t);
         
    //         if (sendto(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr*)&clientadd, cli_ad_sz )<0){
    //             printf("Fail to send file read");  
    //             free(buffer);
    //             exit(1);
    //             }
    //         memset(buffer,'\0',BUFFLEN); 
    //         if(recvfrom(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr*)&clientadd, &cli_ad_sz)<0)
    //         {
    //             perror("Buffer content read failed");
    //             exit(1);
    //         }
          
    //         // Các bước ACK, FIN để kết thúc giao tiếp TCP/IP hoặc là Again để tiếp tục vòng nhận dữ liệu
    //         // printf("%ld\n",size);
          
    //         if (strcmp(buffer,"FINY") == 0){
    //             if (size == BUFFLEN) {t++;
    //                 memset(buffer,'\0',BUFFLEN);                   
    //                 strcpy(buffer,"Again");
    //                 // printf("%s\n",buffer);
    //                 if (sendto(serverSocketfd,buffer,BUFFLEN,0, (struct sockaddr*)&clientadd,cli_ad_sz)<0){
    //                     printf("Fail to send success read file signal");  
    //                     free(buffer);
    //                     exit(1);
    //                 }
    //             }
    //             else {  
                        
    //                         printf("Size from server: %ld \n",t*BUFFLEN+size);
    //                         printf("Server finish service. Ready to close\n"); 
    //                          break;
    //                     }
                   
    //             }  
    //         }
  
            
            
    //     }
    //         }


    free(buffer);
    close(serverSocketfd);

}