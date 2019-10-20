#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>

#define EVENT_MAX 20
#define BUFFER_SIZE_MAX 4096
#define COMMAND_SIZE 200
#define COMMAND_LENGTH 20
#define WORKINGDIRECTORY 100

typedef int (*command_callback)(int epoll_fd, struct epoll_event *event, char *command);

typedef struct ftp_command {
    char *command;
    size_t length;
    command_callback callback;
}ftpCommand;

enum buffer_status {
    COMMAND,
    DATA
};

typedef struct epoll_args{
    char buffer[1000];
    int buffer_length;
    char data[1000];
    int data_length;
    int fd;
    int data_fd;
    enum buffer_status stas[100];
    int stas_length;
    int complex;
    char *working_dir;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    struct sockaddr_storage pasv_addr;
    socklen_t pasv_addrlen;
    int pasv_fd;
}epollArgs;


size_t epoll_size = 0;
char *host = "127.0.0.1";
char *pathname = "/tmp";
char *port = "6789";
char workdir[WORKINGDIRECTORY];
int listenfd;

char command_buf[BUFFER_SIZE_MAX];
int buf_end = 0;

static int socket_bind(const char* ipaddr, char* port);
static void event_add(int epoll_fd, int fd, int state, char *buf);
static void event_delete(int epoll_fd, struct epoll_event *events, int state);
static void handle_event(int epoll_fd,struct epoll_event *events, int ret, int listenfd, char *buf);
static void handle_epoll(int listenfd);
static int ftp_read(int epoll_fd, struct epoll_event *event, char *buf, int n);
static int ftp_write(int epoll_fd, struct epoll_event *event);
static int ftp_write_string(int epoll_fd, struct epoll_event *event, char *buf);

//handle command
static int handle_command(int epoll_fd, struct epoll_event *event, char* command);
static int handle_user(int epoll_fd, struct epoll_event *event, char *command);
static int handle_pass(int epoll_fd, struct epoll_event *event, char *command);
static int handle_syst(int epoll_fd, struct epoll_event *event, char *command);
static int handle_list(int epoll_fd, struct epoll_event *event, char *command);
static int handle_port(int epoll_fd, struct epoll_event *event, char *command);
static int handle_pasv(int epoll_fd, struct epoll_event *event, char *command);
static int handle_cwd(int epoll_fd, struct epoll_event *event, char *command);
static int handle_pwd(int epoll_fd, struct epoll_event *event, char *command);

ftpCommand command_list[] = {
        {"PASS", 4, handle_pass},
        {"QUIT", 4, NULL},
        {"USER", 4, handle_user},

        {"APPE", 4, NULL},
        {"CWD",  3, handle_cwd},
        {"DELE", 4, NULL},
        {"LIST", 4, handle_list},
        {"MKD",  3, NULL},
        {"PASS", 4, NULL},
        {"PASV", 4, handle_pasv},
        {"PORT", 4, handle_port},
        {"PWD",  3, handle_pwd},
        {"QUIT", 4, NULL},
        {"RETR", 4, NULL},
        {"RMD",  3, NULL},
        {"SIZE", 4, NULL},
        {"STOR", 4, NULL},
        {"SYST", 4, handle_syst},
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
    addr.sin_port = strtol(port,NULL,10);

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

void event_add(int epoll_fd, int fd, int state, char* buf) {
    struct epoll_event event;
    event.events = state;
    epollArgs *args = (epollArgs*)malloc(sizeof(epollArgs));
    args->fd = fd;
    args->data_fd = -1;
    args->working_dir = pathname;
    args->buffer_length = 0;
    args->data_length = 0;
    args->complex = 0;
    args->stas_length = 0;
    args->pasv_addrlen = sizeof(args->pasv_addr);
    if(getsockname(fd, (struct sockaddr *)&args->pasv_addr, &args->pasv_addrlen) == -1){
        perror("getsockname error");
    }
//    close(fd);
    if(buf){
        strcpy(args->buffer+args->buffer_length, buf);
        args->buffer_length += strlen(buf);
    }
    event.data.ptr = args;
    int flag = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if(flag == -1){
        perror("event add error");
    }else{
        epoll_size++;
    }
}

void event_delete(int epoll_fd, struct epoll_event *events, int state) {
    events->events = state;
    epollArgs *args = events->data.ptr;
    int flag = epoll_ctl(epoll_fd,EPOLL_CTL_DEL,args->fd,events);
    if (flag == 0) {
        perror("event delete error");
    } else {
        epoll_size--;
    }
}

void handle_epoll(int listenfd) {
    int epoll_fd;
    int ret;
    struct epoll_event events[EVENT_MAX];
    memset(command_buf,0,BUFFER_SIZE_MAX);
    epoll_fd = epoll_create(10);
    event_add(epoll_fd, listenfd,EPOLLIN, NULL);
    while(epoll_size){
        ret = epoll_wait(epoll_fd,events,EVENT_MAX,-1);
        handle_event(epoll_fd,events,ret,listenfd,command_buf);
    }
    close(listenfd);
}

void handle_event(int epoll_fd, struct epoll_event *events, int ret, int listenfd, char *buf) {
    for (int i = 0; i < ret; i++) {
        epollArgs *args = events[i].data.ptr;
        if (args->fd == listenfd) {
            struct sockaddr_in cliaddr;
            socklen_t socklen = sizeof(struct sockaddr_in);
            int socketfd = accept(listenfd, (struct sockaddr *) &cliaddr, &socklen);
            if (socketfd == -1) {
                perror("accpet error:");
            } else {
                //set non blocking
                int flags = fcntl(socketfd, F_GETFL, 0);
                if (flags == -1 || fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    perror("fcntl error");
                }
                event_add(epoll_fd, socketfd, EPOLLIN | EPOLLOUT | EPOLLET,"220 list FTP server ready.\r\n");
            }
        } else {
            if (events[i].events & EPOLLIN) {
                ftp_read(epoll_fd, &events[i], buf, BUFFER_SIZE_MAX);
            }
            if (events[i].events & EPOLLOUT) {
                ftp_write(epoll_fd, &events[i]);
            }
        }
    }
}

int ftp_read(int epoll_fd, struct epoll_event *event, char *buf, int n) {
    int temp;
    epollArgs *args = event->data.ptr;
    while (1){
        temp = read(args->fd, buf + buf_end, n);
        if(temp == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }else{
                perror("read error:");
                close(args->fd);
                event_delete(epoll_fd,event,EPOLLIN);
            }
        }else{
            buf_end = buf_end + temp;
        }
    }

    char command[COMMAND_SIZE];
    while (strchr(buf,'\r')!=NULL){
        size_t command_begin = 0;
        while (buf[command_begin] != '\r'){
            command[command_begin] = buf[command_begin];
            command_begin++;
        }
        command[command_begin] = '\0';
        memmove(buf,buf+command_begin+2,BUFFER_SIZE_MAX-command_begin);
        buf_end = buf_end - command_begin - 2;
        printf("read message is : %s",command);
        handle_command(epoll_fd, event, command);
    }
    return temp;
}

int ftp_write(int epoll_fd, struct epoll_event *event) {
    int n;
    epollArgs *epollArgs1 = event->data.ptr;
    if(epollArgs1->complex){
        for(int i=0;i<epollArgs1->stas_length;i++){
            switch (epollArgs1->stas[i]){
                case COMMAND:{
                    char *position_char = strchr(epollArgs1->buffer,'\n');
                    int position = position_char == NULL ? -1 : position_char - epollArgs1->buffer;
                    if(position != -1){
                        n = write(epollArgs1->fd, epollArgs1->buffer , position + 1);
                        if(n<=epollArgs1->buffer_length){
                            epollArgs1->buffer_length -= position;
                            memmove(epollArgs1->buffer, epollArgs1->buffer+position + 1, epollArgs1->buffer_length);
                        }
                    }
                }
                    break;
                case DATA:{
                    n = write(epollArgs1->data_fd,epollArgs1->data, epollArgs1->data_length);
                    if(n<=epollArgs1->data_length){
                        if(n == epollArgs1->data_length){
                            close(epollArgs1->data_fd);
                        }
                        epollArgs1->data_length -= n;
                        memmove(epollArgs1->data, epollArgs1->data+n, epollArgs1->data_length);
                    }
                }
                    break;
            }
        }
        epollArgs1->complex = 0;
        epollArgs1->stas_length = 0;
    }else{
        n = write(epollArgs1->fd, epollArgs1->buffer,epollArgs1->buffer_length);
        if (n == -1){
            perror("write error:");
            close(epollArgs1->fd);
            event_delete(epoll_fd,event,EPOLLOUT);
        }else if(n<=epollArgs1->buffer_length){
            epollArgs1->buffer_length -= n;
            memmove(epollArgs1->buffer, epollArgs1->buffer+n, epollArgs1->buffer_length);
        }
        return n;
    }
}

int ftp_write_string(int epoll_fd, struct epoll_event *event, char *buf) {
    int n;
    if(buf){
        epollArgs *args = event->data.ptr;
        n = write(args->fd, buf,strlen(buf));
        if (n == -1){
            perror("write error:");
            close(args->fd);
            event_delete(epoll_fd,event,EPOLLOUT);
        }
        return n;
    }
}

int handle_command(int epoll_fd, struct epoll_event *event,char *command) {

    if(strlen(command)<=3){
//        to do
    }
    for(size_t i = 0;i< COMMAND_LENGTH;i++){
        if(strncmp(command_list[i].command,command,command_list[i].length) == 0){
            return (*command_list[i].callback)(epoll_fd, event, command[command_list[i].length] == ' ' ? command+command_list[i].length+1 : command + command_list[i].length);
        }
    }
    return 0;
}

int handle_user(int epoll_fd, struct epoll_event *event, char *command) {
    size_t len = strlen(command);
    if(strncmp("anonymous",command,len-2) == 0){
        epollArgs *args = event->data.ptr;
        strcpy(args->buffer + args->buffer_length,"331 Guest login ok, send your complete e-mail address as password.\r\n");
        args->buffer_length += strlen("331 Guest login ok, send your complete e-mail address as password.\r\n");
    }else{
        perror("handle_user error");
        return -1;
    }
    return 0;
}

int handle_pass(int epoll_fd, struct epoll_event *event, char *command) {
    if(strchr(command,'@')!=NULL){
        epollArgs *args = event->data.ptr;
        strcpy(args->buffer + args->buffer_length, "230 Guest login ok, access restrictions apply.\r\n");
        args->buffer_length += strlen("230 Guest login ok, access restrictions apply.\r\n");
    }
    return 0;
}

int handle_syst(int epoll_fd, struct epoll_event *event, char *command) {
    (void) command;
    epollArgs *args = event->data.ptr;
    strcpy(args->buffer +args->buffer_length, "215 UNIX Type: L8\r\n");
    args->buffer_length += strlen("215 UNIX Type: L8\r\n");
    return 0;
}

int handle_list(int epoll_fd, struct epoll_event *event, char *command) {
    char filename[255];
    epollArgs *args = event->data.ptr;
    args->complex = 1;
    strcpy(args->buffer + args->buffer_length,"150 directory list start\r\n");
    args->stas[args->stas_length++] = COMMAND;
    args->buffer_length += strlen("150 directory list start\r\n");
    DIR *dir = NULL;
    struct dirent *myitem = NULL;
    dir = opendir(args->working_dir);
    if(dir == NULL){
        perror("handle_list error");
        return -1;
    }else{
        while((myitem=readdir(dir)) != NULL){
            size_t s = 0;
            while(myitem->d_name[s]!='\0'){
                filename[s] = myitem->d_name[s];
                s++;
            }
            filename[s] = '\n';
            filename[s+1] = '\0';
            strcpy(args->data+args->data_length,filename);
            args->data_length += strlen(filename);
        }
    }
    args->stas[args->stas_length++] = DATA;
    strcpy(args->buffer+args->buffer_length,"226 directory list end\r\n");
    args->buffer_length += strlen("226 directory list end\r\n");
    args->stas[args->stas_length++] = COMMAND;
}

int handle_port(int epoll_fd, struct epoll_event *event, char *command) {

    epollArgs *args = event->data.ptr;
    struct sockaddr_in *addr = (struct sockaddr_in *)&args->addr;
    unsigned char *host = (unsigned char *) &addr->sin_addr, *port = (unsigned char *) &addr->sin_port;
    if (sscanf(command, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", host, host + 1, host + 2, host + 3, port, port + 1) != 6) {
        strcpy(args->buffer+args->buffer_length,"500 Illegal.\r\n");
        args->buffer_length += strlen("500 Illegal.\r\n");
        return -1;
    }
    addr->sin_family = AF_INET;
    args->addrlen = sizeof(struct sockaddr_in);
    if(args->data_fd != -1){
        close(args->data_fd);
    }
    args->data_fd = socket(args->addr.ss_family,SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(args->data_fd == -1){
        ftp_write_string(epoll_fd,event,"425 connection error");
    }else{

        int flags = fcntl(args->data_fd, F_GETFL, 0);
        if (flags == -1 || fcntl(args->data_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl error");
        }

        if(connect(args->data_fd, (struct sockaddr *)&args->addr, args->addrlen) == -1){
            if(errno == EINPROGRESS){
                struct epoll_event *event1;
                event_add(epoll_fd, args->data_fd,EPOLLET|EPOLLIN|EPOLLOUT,NULL);
                strcpy(args->buffer+args->buffer_length,"200 PORT command successful.\r\n");
                args->buffer_length += strlen("200 PORT command successful.\r\n");
            }else{
                perror("connect error");
                ftp_write_string(epoll_fd, event,"425 connection error");
            }
        }else{
            struct epoll_event *event1;
            event_add(epoll_fd, args->data_fd,EPOLLET|EPOLLIN|EPOLLOUT, NULL);
            strcpy(args->buffer+args->buffer_length,"200 PORT command successful.\r\n");
            args->buffer_length += strlen("200 PORT command successful.\r\n");
        }
    }
    return 0;
}

int handle_pasv(int epoll_fd, struct epoll_event *event, char *command) {
    (void) command;
    int ret = -1;
    epollArgs *args = event->data.ptr;
    if (args->pasv_addr.ss_family != AF_INET) {
        strcpy(args->buffer+args->buffer_length, "522: No address available for PASV\r\n");
        args->buffer_length += strlen("522: No address available for PASV\r\n");
        return -1;
    }
    args->pasv_fd = socket(args->pasv_addr.ss_family, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (args->pasv_fd == -1) {
        perror("pasv handle error");
    } else {
        struct sockaddr_in addr = *(struct sockaddr_in *) &args->pasv_addr;
        addr.sin_port = 0;
        socklen_t addrlen = sizeof(addr);
        if (bind(args->pasv_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
            close(args->pasv_fd);
            args->pasv_fd = -1;
        } else if (getsockname(args->pasv_fd, (struct sockaddr *) &addr, &addrlen) == -1) {
            close(args->pasv_fd);
            args->pasv_fd = -1;
        } else if (listen(args->pasv_fd, SOMAXCONN) == -1) {
            close(args->pasv_fd);
            args->pasv_fd = -1;
        } else {
            event_add(epoll_fd, args->pasv_fd,EPOLLIN | EPOLLET, NULL);
            strcpy(args->buffer+args->buffer_length,"227 Passive Mode\r\n");
            args->buffer_length += strlen("227 Passive Mode\r\n");
            ret = 0;
            }
        }
    if(ret == -1){
        strcpy(args->buffer+args->buffer_length, "451 Server temporary unavailable.\r\n");
        args->buffer_length += strlen("451 Server temporary unavailable.\r\n");
    }
    return 0;
}

int handle_cwd(int epoll_fd, struct epoll_event *event, char *command) {
    chdir(command);
    return 0;
}

int handle_pwd(int epoll_fd, struct epoll_event *event, char *command) {
    getcwd(workdir,WORKINGDIRECTORY);
    epollArgs * args = event->data.ptr;
    strcpy(args->buffer+args->buffer_length,"")
    return 0;
}

int main(int argc, char *argv[]) {

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
            default:
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
