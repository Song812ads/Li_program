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
#include <pthread.h>
#define BUFFLEN 1000
#define MAX_CLIENTS 10

pthread_cond_t  got_request   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t request_mutex = PTHREAD_MUTEX_INITIALIZER;
struct request {
    int socket;
    struct request* next;
    struct sockaddr_in clientadd;
    unsigned int  clientlength;
}request_t;
static int num_req = 0;
struct request *req = NULL;
struct request *last_req = NULL;

void handle(int socket, struct sockaddr_in clientadd, 
                            unsigned int clientlength);

void* handle_request(){
    pthread_mutex_lock(&request_mutex);
    while (1){
        if (num_req == 0) pthread_cond_wait(&got_request,
                                        &request_mutex);
        int socket = req->socket;
        struct sockaddr_in clientadd = req->clientadd;
        unsigned int clientlength = req->clientlength;
        struct request* head = last_req;
        if (head -> next == NULL){
            req = NULL;
            last_req = NULL;
        }
        else {
        while (head->next->next!=NULL)
            head = head->next;
        req = head;
        req->next = NULL;
        }
        num_req --;
        pthread_mutex_unlock(&request_mutex);
        handle(socket,clientadd,clientlength);
    }
}

void add_req(int socket, struct sockaddr_in clientadd, 
                            unsigned int clientlength){
    pthread_mutex_lock(&request_mutex);
    struct request* a_request = (struct request *)malloc(sizeof(struct request));
    a_request -> socket = socket;
    a_request -> clientadd = clientadd;
    a_request -> clientlength = clientlength;
    a_request -> next = last_req;
    if (num_req == 0){
        req = a_request;
    }
    last_req = a_request;
    num_req++;
    pthread_mutex_unlock(&request_mutex);
    pthread_cond_signal(&got_request);
}

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

void handle(int socket, struct sockaddr_in clientadd, unsigned int  clientlength){
    int sd = socket;
    char buffer[BUFFLEN];
    int ret;
start: 
while (1){
    memset(buffer,'\0',BUFFLEN);
    ret= read(sd,buffer,BUFFLEN);
    if (ret <0){  
        perror("Rcv error");
        exit(1);
    }
    else if (ret ==0){
        printf("Client disconnect\n");
        close(sd);
        break;
    }
    if (strcmp(buffer,"A")==0){
        memset(buffer,'\0',BUFFLEN);
        strcpy(buffer,"File");
        if (send(sd,buffer,BUFFLEN,0)<0){
            perror("Send error");
            exit(1);
        }
    }
    else break;}
    if (ret > 0) {
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
            if (send(sd,buffer,BUFFLEN,0)<0){
                perror("Send error");
                exit(1);
            }
            goto start;
        }
        else {
            int op = open(path_buffer, O_RDONLY);
            free(path_buffer);
            lseek(op,0,SEEK_SET);
            while (1){
            memset(buffer,'\0',BUFFLEN);
            sz = readn(op,buffer,BUFFLEN);
            if (send(sd,buffer,sz,0)<0){
                perror("Send error2");
                exit(1);
            }
            if (sz < BUFFLEN){
                printf("Client disconnect. Transmit: %ld\n",ti*BUFFLEN+sz);
                close(op);
                goto start;
                break;
            }
            else 
            {
                ti++;
                sz = 0;
                lseek(op,ti*BUFFLEN,SEEK_SET);
            }
        }
    }}}
            
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
    pthread_t thread;

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

    if (bind (serverSocketfd, (struct sockaddr*) &serveradd, sizeof( serveradd))!=0){
        perror("Server bind fail");
        close(serverSocketfd);
        exit(1);
    }
    else printf("Binding...\n");
    if (listen(serverSocketfd,20)!=0)
    {
        perror("Server listen error");
        close(serverSocketfd);
        exit(1);
    }
    else printf("Listening...\n");
    for (int i =0; i <MAX_CLIENTS; i++) {
        pthread_create(&thread, NULL, handle_request,NULL);
    }
while (1){
    FD_ZERO(&readfds);
    FD_SET(serverSocketfd,&readfds);
    int activity = select( serverSocketfd + 1 , &readfds , NULL , NULL , NULL);
    if ((activity < 0) && (errno!=EINTR)) 
    {
        printf("select error");
    }

    if (FD_ISSET(serverSocketfd, &readfds)) 
        {
            if (( new_socket = accept(serverSocketfd, 
            (struct sockaddr *)&clientadd, &clientlength))<0)
            {
                perror("accept");
                exit(1);
            }
            printf("New connection is : %s\n",sock_ntop((struct sockaddr*)&clientadd,
                                                INET_ADDRSTRLEN));
            add_req(new_socket,clientadd,clientlength);
        }
    }
    free(buffer);
    close(serverSocketfd);
}
