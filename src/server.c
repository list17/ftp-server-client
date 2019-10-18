#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

#define EVENT_MAX 20
#define BUFFER_SIZE_MAX 4096
size_t epoll_size = 0;
char *response[BUFFER_SIZE_MAX];


static int socket_bind(const char* ipaddr, char* port);
static void event_add(int epoll_fd, int fd, int state);
static void event_delete(int epoll_fd, int fd, int state);
static void event_modify(int epoll_fd, int fd, int state);
static void handle_event(int epoll_fd,struct epoll_event *events, int ret, int listenfd, char *buf);
static void handle_epoll(int listenfd);
static int ftp_read(int epoll_fd, int fd, char *buf, int n);
static int ftp_write(int epoll_fd, int fd, char *buf);

int socket_bind(const char *ipaddr, char* port) {
    int listenfd;
    struct sockaddr_in addr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if(listenfd == -1){
        perror("socket bind error");
        exit(1);
    }

    memset(&addr,0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET,ipaddr,&addr.sin_addr);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = atoi(port);

    if(bind(listenfd,(struct sockaddr*)&addr,sizeof(addr)) == -1){
        perror("bind error");
        exit(1);
    }
    return listenfd;
}

void event_add(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    event.data.fd = fd;
    int flag = epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    if(flag == -1){
        perror("event add error");
    }else{
        epoll_size++;
    }
}

void event_delete(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    event.data.fd = fd;
    int flag = epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,&event);
    if (flag == 0) {
        perror("event delete error");
    } else {
        epoll_size--;
    }
}

void event_modify(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    event.data.fd = fd;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_MOD,fd,&event) == -1){
        perror("event modify error");
    }
}

void handle_epoll(int listenfd) {
    int epoll_fd;
    int ret;
    struct epoll_event events[EVENT_MAX];
    char buf[BUFFER_SIZE_MAX];
    memset(buf,0,BUFFER_SIZE_MAX);
    epoll_fd = epoll_create1(0);
    event_add(epoll_fd,listenfd,EPOLLIN);
    while(epoll_size){
        ret = epoll_wait(epoll_fd,events,EVENT_MAX,-1);
        handle_event(epoll_fd,events,ret,listenfd,buf);
    }
    close(listenfd);
}

void handle_event(int epoll_fd, struct epoll_event *events,int ret, int listenfd, char *buf) {
    for(int i = 0;i < ret;i++){
        if(events[i].data.fd == listenfd){
            struct sockaddr_in cliaddr;
            socklen_t socklen;
            int socketfd = accept(listenfd,(struct sockaddr*)&cliaddr,&socklen);
            if (socketfd == -1) {
                perror("accpet error:");
            } else {
                printf("accept a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                //添加一个客户描述符和事件
                event_add(epoll_fd, socketfd, EPOLLOUT);
            }
        } else if(events[i].events & EPOLLIN){
            //read
            ftp_read(epoll_fd,events[i].data.fd,buf,BUFFER_SIZE_MAX);
        } else if(events[i].events & EPOLLOUT){
            //write
            strcpy(response,"220 (MinimumFTP v0.1.0 alpha)\r\n");
            ftp_write(epoll_fd, events[i].data.fd, response);
        }
    }
}



int ftp_read(int epoll_fd, int fd, char *buf, int n) {
    int nread;
    nread = read(fd, buf, n);
    printf("%d\n",nread);
    if (nread == -1)
    {
        perror("read error:");
        close(fd);
        event_delete(epoll_fd,fd,EPOLLIN);
    }
    else if (nread == 0)
    {
        fprintf(stderr,"client close.\n");
        close(fd);
        event_delete(epoll_fd,fd,EPOLLIN);
    }
    else
    {
        printf("read message is : %s",buf);
        //修改描述符对应的事件，由读改为写
        event_modify(epoll_fd, fd,EPOLLOUT);
    }
    return nread;
}

int ftp_write(int epoll_fd, int fd, char *buf) {
    int nwrite;
    nwrite = write(fd,buf,strlen(buf));
    if (nwrite == -1)
    {
        perror("write error:");
        close(fd);
        event_delete(epoll_fd,fd,EPOLLOUT);
    }
    else
        event_modify(epoll_fd,fd,EPOLLIN);
    memset(buf,0,BUFFER_SIZE_MAX);
    printf("%d",nwrite);
    return nwrite;
}


int main(int argc, char *argv[]) {

    char *host = "127.0.0.1";
	char *pathname = "/tmp";
	char *port = "6789";
	int listenfd;

	//参数解析
	static struct option long_options[] = {
		{"root", required_argument,0,'r'},
        {"port", required_argument,0,'p'},
        {0, 0, 0, 0}
	};

	for(int i=0;i<argc;i++){
	    if(strcmp(argv[i],"-port") == 0){
            argv[i] = "--port";
	    }else if (strcmp(argv[i], "-root") == 0){
            argv[i] = "--root";
        }
	}
    int opt;
    while ((opt = getopt_long(argc, argv, "p:r:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'r':
                pathname = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: ./server [-port ] [-root ]\n");
                exit(EXIT_FAILURE);
        }
    }
    listenfd = socket_bind(host, port);
    if(listen(listenfd, 10) == -1){
        perror("listen error");
    }
    handle_epoll(listenfd);
    return 0;
}
