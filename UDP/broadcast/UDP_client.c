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
#define MAX_CLIENTS 1
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

char *sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
	char	portstr[8];
	static char str[128];

	switch (sa->sa_family) {
	case AF_INET: {
		      struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		      if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str))
				      == NULL)
			      return NULL;
		      if (ntohs(sin->sin_port) != 0) {
			      snprintf(portstr, sizeof(portstr), ":%d", 
					      ntohs(sin->sin_port));
			      strcat(str, portstr);
		      }
		      return str;
	      }
	}
	return NULL;
}

int main(int argc, char **argv){
    int socketfd; 
    struct sockaddr_in serveradd;
    socklen_t serlen = sizeof(serveradd);
    char buffer[BUFFLEN];
    fd_set readfds;
    // if (argc!=4){
    //     printf("Wrong type <server addresss> <server port>\n");
    //     exit(1);
    // }
    if ((socketfd = socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("Socket create fail\n");
        exit(1);
    }
    else printf("Socket: %d \n",socketfd);
    bzero (&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
    char* port = argv[2];
    serveradd.sin_port = htons ( 5375 );
    serveradd.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    const int on = 1;
    int ret = setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, 
                                    (char*)&on, sizeof(on));
    if (ret == -1) {
        perror("setsockopt error");
        return 0;
    }
    char filename[20];
    printf("Nhap file muon tai: ");
    gets(filename);
    if(sendto(socketfd,filename,strlen(filename),0, 
    (struct sockaddr *)&serveradd, sizeof(serveradd))<0) {
        perror("Send error");
        exit(1);
    }
        FD_ZERO(&readfds);
        FD_SET(socketfd,&readfds);
        while(1){
        select(socketfd+1,&readfds,NULL,NULL,NULL);

        printf("From server: %s\n", sock_ntop((struct sockaddr*)&serveradd,
                                                INET_ADDRSTRLEN));
            memset(buffer,'\0',BUFFLEN);           
            if (recvfrom(socketfd,buffer,BUFFLEN,0, 
            (struct sockaddr *)&serveradd, &serlen)<0){
                perror("Recv error");
                exit(1);
            }
            if (ret == 0){
                printf("Server not response \n");
                break;
            }
            else if (ret<0){
                perror("Recv fail");
                exit(1);
            }
            if (strcmp(buffer,"Err")==0){
                printf("From server %s: File not exist\n", sock_ntop((struct sockaddr*)&serveradd,
                                                INET_ADDRSTRLEN));
                break;
            }
        else  {
            ssize_t t = 0;
            long sz = 0;
            int op = open(filename, O_RDWR | O_CREAT , 0644); 
            lseek(op,0,SEEK_SET);
        while (1){
 
            writen(op,buffer,ret);
            if (ret==BUFFLEN){
            t++;
            sz = 0;
            lseek(op,t*BUFFLEN,SEEK_SET);
            memset(buffer,'\0',BUFFLEN);
            if ((ret = recvfrom(socketfd,buffer,BUFFLEN,0,(struct sockaddr *)&serveradd, &serlen))<0){
                perror("Recv error");
                exit(1);
            }
            }
            else {
            close(op);
            printf("Size from server %s: %ld\n",sock_ntop((struct sockaddr*)&serveradd,
                                                INET_ADDRSTRLEN),t*BUFFLEN+ret);
            break;
            }
        }
        }
        }
    
    free(buffer);
    return 0;
}