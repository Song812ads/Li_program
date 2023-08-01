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
#define BUFFLEN 500
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
    printf("File write complete part %d \n",t+1);
}

int main(int argc, char **argv){
    // Thiết lập phương thức nhận dữ liêu và tạo kết nối đến server
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    int socketfd; 
    struct sockaddr_in serveradd;
    int serlen = sizeof(serveradd);
    unsigned char *buffer = (unsigned char* )malloc(BUFFLEN * sizeof(unsigned char));
    
    if (argc!=4){
        printf("Wrong type <server addresss> <server port>\n");
        exit(1);
    }

    // Socket create:
    if ((socketfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))<0){
        perror("Socket create fail\n");
        exit(1);
    }
    else printf("Socket: %d \n",socketfd);

    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    char* port = argv[2];
    serveradd.sin_port = htons ( atoi(port) );
    serveradd.sin_addr.s_addr = inet_addr("192.168.29.198");


    memset(buffer,'\0',BUFFLEN);                   
    strcpy(buffer,argv[3]);
    if(sendto(socketfd,buffer,BUFFLEN,0, (struct sockaddr* )&serveradd, serlen)<0)              
    {
        printf("sending the file name to the server side failed\n");
        perror("send failed");
        exit(1);
    }
    
    // Kiểm tra xem server đã sẵn sàng gửi dữ liệu chưa
    memset(buffer,'\0',BUFFLEN);                   
    if(recvfrom(socketfd,buffer,BUFFLEN,0,(struct sockaddr* )&serveradd, &serlen )<0)
    {
        perror("Checkin failed");
        exit(1);
    }

    if (strcmp(buffer,"Error")==0) {
        printf("%s download fail \n",argv[3]);
        exit(1);
    }

    else{
        printf("%s ready to download \n",argv[3]);
        memset(buffer,'\0',BUFFLEN);                   
        strcpy(buffer,"Ready");
        if (sendto(socketfd,buffer,BUFFLEN,0, (struct sockaddr* )&serveradd, serlen)<0){
            printf("Fail to send success read file signal");  
            free(buffer);
            exit(1);
        }
    int t =0; // số lần đã chuyển 
    file_mode mode = FIRST; // mode mở của file
    // here:
     while (1){
        if (t>0) mode = AFTER;
        // Nhận kích thuốc file từ server
        memset(buffer,'\0',BUFFLEN); 
        if(recvfrom(socketfd,buffer,BUFFLEN,0,(struct sockaddr* )&serveradd, &serlen )<0)
        {
            perror("Buffer size read failed");
            exit(1);
        }
            long size = atol(buffer);
        // gửi tin hiêu đã nhận kích thươc file
            memset(buffer,'\0',BUFFLEN);                   
            strcpy(buffer,"size");
            if (sendto(socketfd,buffer,BUFFLEN,0, (struct sockaddr* )&serveradd, serlen)<0){
                printf("Fail to send success read file signal");  
                free(buffer);
                exit(1);
            }
            // Nhận dữ liệu file từ server
            memset(buffer,'\0',BUFFLEN); 
            if(recvfrom(socketfd,buffer,BUFFLEN,0,(struct sockaddr* )&serveradd, &serlen )<0)
            {
                perror("Buffer content read failed");
                exit(1);
            }
            
            file_transfer(argv[3],buffer,size,t,mode);
            memset(buffer,'\0',BUFFLEN);  
              
            // Gửi tín hiệu đã hoàn tất đợi nhận từ server đã kết thúc nhận dữ liệu hay sẽ nhận tiếp
            strcpy(buffer,"FIN");
            
            if (sendto(socketfd,buffer,BUFFLEN,0, (struct sockaddr* )&serveradd, serlen)<0){
                printf("Fail to send success read file signal");  
                free(buffer);
                exit(1);
            }
            printf("%s",buffer);
            memset(buffer,'\0',BUFFLEN); 
            if(recvfrom(socketfd,buffer,BUFFLEN,0,(struct sockaddr* )&serveradd, &serlen )<0)
            {
                perror("Buffer content read failed");
                exit(1);
            }     
            
            if (strcmp(buffer,"Again")==0){
                t++;
            }
            else if (strcmp(buffer,"FIN")==0){
                    printf("Read total file size: %ld\n",size);
                    printf("Connection close\n");
                }
                break;
        }}
        


    free(buffer);
    close(socketfd);
    return 0;
}