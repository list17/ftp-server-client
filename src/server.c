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

typedef int (*command_callback)(int epoll_fd, struct epoll_event *event, char *command);

typedef struct ftp_command {
    char *command;
    size_t length;
    command_callback callback;
}ftpCommand;



enum client_status {
    BEFORE_LOGIN,
    LOGIN,
    LOGIN_SUCCESS
};

typedef struct epoll_args{
    char buffer[100];
    int fd;
    enum client_status status;
}epollArgs;


size_t epoll_size = 0;
char *response[BUFFER_SIZE_MAX];



static int socket_bind(const char* ipaddr, char* port);
static void event_add(int epoll_fd, int fd, int state);
static void event_delete(int epoll_fd, int fd, int state);
static void event_modify(int epoll_fd, int fd, int state);
static void handle_event(int epoll_fd,struct epoll_event *events, int ret, int listenfd, char *buf);
static void handle_epoll(int listenfd);
static int ftp_read(int epoll_fd, struct epoll_event *event, char *buf, int n);
static int ftp_write(int epoll_fd, struct epoll_event *event);
static int ftp_write_string(int epoll_fd, int fd, char *buf);

//handle command
static int handle_command(int epoll_fd, struct epoll_event *event, char* full_command);
static int handle_user(int epoll_fd, struct epoll_event *event, char *command);

ftpCommand command_list[] = {
        {"PASS", 4, NULL},
        {"QUIT", 4, NULL},
        {"USER", 4, handle_user},

        {"APPE", 4, NULL},
        {"CWD",  3, NULL},
        {"DELE", 4, NULL},
        {"EPRT", 4, NULL},
        {"EPSV", 4, NULL},
        {"LIST", 4, NULL},
        {"MKD",  3, NULL},
        {"PASS", 4, NULL},
        {"PASV", 4, NULL},
        {"PORT", 4, NULL},
        {"PWD",  3, NULL},
        {"QUIT", 4, NULL},
        {"RETR", 4, NULL},
        {"RMD",  3, NULL},
        {"SIZE", 4, NULL},
        {"STOR", 4, NULL},
        {"SYST", 4, NULL},
        {"TYPE", 4, NULL},
        {"USER", 4, NULL}
};

int socket_bind(const char *ipaddr, char* port) {
    int listenfd;
    struct sockaddr_in addr;
    listenfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(listenfd == -1){
        perror("socket bind error");
        exit(1);
    }

    memset(&addr,0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET,ipaddr,&addr.sin_addr);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = atoi(port);

    int temp = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(temp)) == -1){
        perror("setsockopt error");
        exit(1);
    }
    if(bind(listenfd,(struct sockaddr*)&addr,sizeof(addr)) == -1){
        perror("bind error");
        exit(1);
    }
    return listenfd;
}

void event_add(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    epollArgs *args = (epollArgs*)malloc(sizeof(epollArgs));
    args->status = BEFORE_LOGIN;
    args->fd = fd;
    strcpy(args->buffer,"220 FTP server ready\r\n");
    event.data.ptr = args;
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
    epoll_fd = epoll_create(10);
    event_add(epoll_fd,listenfd,EPOLLIN | EPOLLOUT );
    while(epoll_size){
        ret = epoll_wait(epoll_fd,events,EVENT_MAX,-1);
        handle_event(epoll_fd,events,ret,listenfd,buf);
    }
    close(listenfd);
}

void handle_event(int epoll_fd, struct epoll_event *events,int ret, int listenfd, char *buf) {
    for(int i = 0;i < ret;i++){
        epollArgs  *args = events[i].data.ptr;
        if(args->fd == listenfd){
            struct sockaddr_in cliaddr;
            socklen_t socklen;
            int socketfd = accept(listenfd,(struct sockaddr*)&cliaddr,&socklen);
            if (socketfd == -1) {
                perror("accpet error:");
            } else {
                event_add(epoll_fd, socketfd, EPOLLIN|EPOLLOUT|EPOLLET);
            }
        }
        else {
            if(events[i].events & EPOLLIN){
                ftp_read(epoll_fd, &events[i],buf,BUFFER_SIZE_MAX);
            }
            if(events[i].events & EPOLLOUT){
                ftp_write(epoll_fd, &events[i]);
            }
        }
    }
}



int ftp_read(int epoll_fd, struct epoll_event *event, char *buf, int n) {
    int temp;
    epollArgs *args = event->data.ptr;
    temp = read(args->fd, buf, n);
    if (temp == -1){
        perror("read error:");
        close(args->fd);
        event_delete(epoll_fd,args->fd,EPOLLIN);
    }else if (temp == 0){
        fprintf(stderr,"client close.\n");
        close(args->fd);
        event_delete(epoll_fd,args->fd,EPOLLIN);
    }else{
        printf("read message is : %s",buf);
        handle_command(epoll_fd, event, buf);
    }
    return temp;
}

int ftp_write(int epoll_fd, struct epoll_event *event) {
    int n;
    epollArgs *epollArgs1 = event->data.ptr;
    if(epollArgs1->buffer){
        int length = strlen(epollArgs1->buffer);
        n = write(epollArgs1->fd, epollArgs1->buffer,strlen(epollArgs1->buffer));
        if (n == -1){
            perror("write error:");
            close(epollArgs1->fd);
            event_delete(epoll_fd,epollArgs1->fd,EPOLLOUT);
        }
        return n;
    }
}

int ftp_write_string(int epoll_fd, int fd, char *buf) {
    int n;
    if(buf){
        int length = strlen(buf);
        n = write(fd, buf,strlen(buf));
        if (n == -1){
            perror("write error:");
            close(fd);
            event_delete(epoll_fd,fd,EPOLLOUT);
        }
        return n;
    }
}

int handle_command(int epoll_fd, struct epoll_event *event,char *full_command) {
    if(strlen(full_command)<=3){
        epollArgs *args = event->data.ptr;
        ftp_write_string(epoll_fd,args->fd,"command too short");
    }
    for(int i=0;i< sizeof(command_list);i++){
        if(strncmp(command_list[i].command,full_command,command_list->length) == 0){
            return (*command_list[i].callback)(epoll_fd, event, full_command[command_list[i].length] == ' ' ? full_command+command_list[i].length+1 : full_command + command_list[i].length);
        }
    }

    return 0;
}

int handle_user(int epoll_fd, struct epoll_event *event, char *command) {
    printf("handle user command %s \n", command);
    int len = strlen(command);

    epollArgs *args = event->data.ptr;
    if(strncmp("anonymous",command,len-2) == 0){
        ftp_write_string(epoll_fd, args->fd, "331 Guest login ok, send your complete e-mail address as password.\r\n");
    }else{
        ftp_write_string(epoll_fd, args->fd, "error");
    }
    return 0;
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
