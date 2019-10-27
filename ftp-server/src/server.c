#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <stdarg.h>

#define EVENT_MAX 21
#define BUFFER_SIZE_MAX 4096
#define COMMAND_SIZE 200
#define COMMAND_LENGTH 20
#define COMMAND_LENGTH_BEFORE_LOGIN 3
#define WORKINGDIRECTORY 100
#define DATA_SIZE_MAX 4096*4

enum WRITE_MODE {
    WRITE_FREE,
    FTP_DATA,
    FTP_FILE
};

enum TRANS_MODE {
    INIT,
    PORT,
    PASV
};

size_t epoll_size = 0;
char *host = "127.0.0.1";
char pathname[PATH_MAX] = "/tmp";
char *port = "21";
int listenfd;

char command_buf[BUFFER_SIZE_MAX];
size_t buf_end = 0;

typedef int (*command_callback)(int epoll_fd, struct epoll_event *event, char *command);

typedef struct ftp_command {
    char *command;
    size_t length;
    command_callback callback;
}ftpCommand;

typedef struct fd_collection{
    int command_fd;
    int data_fd;
    int file_fd;
    int pasv_fd;

    char data[DATA_SIZE_MAX];
    ssize_t data_length;

    int ready_to_write;

    enum TRANS_MODE trans_mode;
    enum WRITE_MODE write_mode;

}fd_collection;

typedef struct epoll_args{
    char buffer[BUFFER_SIZE_MAX];
    int client_exit;
    int client_login;
    size_t buffer_length;

    int fd;
    fd_collection *fds;

    char working_dir[PATH_MAX];
    size_t wd_len;

    char rename_from[PATH_MAX];
    int reto;

    off_t rest_offset;
    off_t appe_offset;

    struct sockaddr_storage addr;
    socklen_t addrlen;
    struct sockaddr_storage pasv_addr;
    socklen_t pasv_addrlen;

}epollArgs;

int str_cpy(epollArgs **args, char *format, ...);
int remove_connections(epollArgs **args);

int socket_bind(const char* ipaddr, char* port);
void epoll_args_init(epollArgs **args, int fd, char *buf);
void event_add(int epoll_fd, int fd, epollArgs *args, unsigned int state);
void event_delete(int epoll_fd, int fd, int state);
void event_modify(int epoll_fd, int fd, epollArgs **args, unsigned int state);
void handle_event(int epoll_fd,struct epoll_event *events, int ret, int listenfd, char *buf);
void handle_epoll(int listenfd);
int ftp_read(int epoll_fd, struct epoll_event *event, char *buf, int n);
int ftp_write(int epoll_fd, struct epoll_event *event);
int ftp_write_data(int epoll_fd, struct epoll_event *event, int mode);

//handle command
int handle_command(int epoll_fd, struct epoll_event *event, char* command);
int handle_abor(int epoll_fd, struct epoll_event *event, char *command);
int handle_pass(int epoll_fd, struct epoll_event *event, char *command);
int handle_syst(int epoll_fd, struct epoll_event *event, char *command);
int handle_list(int epoll_fd, struct epoll_event *event, char *command);
int handle_pasv(int epoll_fd, struct epoll_event *event, char *command);
int handle_user(int epoll_fd, struct epoll_event *event, char *command);
int handle_port(int epoll_fd, struct epoll_event *event, char *command);
int handle_cwd(int epoll_fd, struct epoll_event *event, char *command);
int handle_pwd(int epoll_fd, struct epoll_event *event, char *command);
int handle_type(int epoll_fd, struct epoll_event *event, char *command);
int handle_retr(int epoll_fd, struct epoll_event *event, char *command);
int handle_mkd(int epoll_fd, struct epoll_event *event, char *command);
int handle_quit(int epoll_fd, struct epoll_event *event, char *command);
int handle_stor(int epoll_fd, struct epoll_event *event, char *command);
int handle_rmd(int epoll_fd, struct epoll_event *event, char *command);
int handle_dele(int epoll_fd, struct epoll_event *event, char *command);
int handle_rnfr(int epoll_fd, struct epoll_event *event, char *command);
int handle_rnto(int epoll_fd, struct epoll_event *event, char *command);
int handle_size(int epoll_fd, struct epoll_event *event, char *command);
int handle_appe(int epoll_fd, struct epoll_event *event, char *command);
int handle_rest(int epoll_fd, struct epoll_event *event, char *command);

ftpCommand command_list_brfore_login[] = {
        {"USER", 4, handle_user},
        {"PASS", 4, handle_pass},
        {"QUIT", 4, handle_quit},
};

ftpCommand command_list[] = {
        {"ABOR", 4,handle_abor},
        {"APPE", 4, handle_appe},
        {"CWD",  3, handle_cwd},
        {"DELE", 4, handle_dele},
        {"LIST", 4, handle_list},
        {"MKD",  3, handle_mkd},
        {"PASS", 4, handle_pass},
        {"PASV", 4, handle_pasv},
        {"PORT", 4, handle_port},
        {"PWD",  3, handle_pwd},
        {"QUIT", 4, handle_quit},
        {"RETR", 4, handle_retr},
        {"RMD",  3, handle_rmd},
        {"RNFR,",4, handle_rnfr},
        {"RNTO,",4, handle_rnto},
        {"SIZE", 4, handle_size},
        {"STOR", 4, handle_stor},
        {"SYST", 4, handle_syst},
        {"TYPE", 4, handle_type},
        {"USER", 4, handle_user},
        {"REST", 4, handle_rest}
};

int socket_bind(const char *ipaddr, char* port_t) {
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
    addr.sin_port = htons(strtol(port_t,NULL,10));

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


void epoll_args_init(epollArgs **args, int fd, char *buf) {
    *args = (epollArgs*)malloc(sizeof(epollArgs));
    (*args)->fd = fd;

    (*args)->fds = (fd_collection*)malloc(sizeof(fd_collection));

    (*args)->fds->file_fd = -1;
    (*args)->fds->pasv_fd = -1;
    (*args)->fds->ready_to_write = 0;

    (*args)->fds->write_mode = WRITE_FREE;
    (*args)->fds->trans_mode = INIT;

    (*args)->fds->data_fd = -1;

    (*args)->fds->command_fd = -1;

    strcpy((*args)->working_dir, pathname);
    if((*args)->working_dir[strlen(pathname) - 1] != '/'){
        (*args)->working_dir[strlen(pathname)] = '/';
        (*args)->working_dir[strlen(pathname)+1] = '\0';
    }
    (*args)->wd_len = strlen((*args)->working_dir);
    (*args)->reto = 0;
    (*args)->rest_offset = 0;
    (*args)->appe_offset = 0;

    (*args)->buffer_length = 0;
    (*args)->fds->data_length = 0;
    (*args)->pasv_addrlen = sizeof((*args)->pasv_addr);
    (*args)->client_exit = 0;
    (*args)->client_login = 0;
    if(getsockname(fd, (struct sockaddr *)&(*args)->pasv_addr, &(*args)->pasv_addrlen) == -1){
        perror("getsockname error");
    }
    if(buf){
        strcpy((*args)->buffer+(*args)->buffer_length, buf);
        (*args)->buffer_length += strlen(buf);
    }
}

void event_add(int epoll_fd, int fd, epollArgs *args, unsigned int state) {
    args->fd = fd;
    struct epoll_event event;
    event.events = state;
    event.data.ptr = args;
    int flag = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if(flag == -1){
        fprintf(stderr, "%d:%s", errno, strerror(errno));
    }else{
        epoll_size++;
    }
}

void event_delete(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    int flag = epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd, &event);
    if (flag == 0) {
        perror("event delete error");
    } else {
        epoll_size--;
    }
}


void event_modify(int epoll_fd, int fd, epollArgs **args, unsigned int state) {
    (void) fd;
    struct epoll_event event;
    event.events = state;
    epollArgs *args1 = NULL;
    epoll_args_init(&args1, (*args)->fds->data_fd,NULL);
    args1->fds = (*args)->fds;
    event.data.ptr = args1;
    int flag = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, (*args)->fds->data_fd, &event);
    if(flag == -1){
        fprintf(stderr, "%d:%s", errno, strerror(errno));
    }
}

void handle_epoll(int fd) {
    int epoll_fd;
    int ret;
    struct epoll_event events[EVENT_MAX];
    memset(command_buf,0,BUFFER_SIZE_MAX);
    epoll_fd = epoll_create(10);
    epollArgs *args = NULL;
    epoll_args_init(&args,fd,NULL);
    event_add(epoll_fd, fd, args, EPOLLIN);
    while(epoll_size){
        ret = epoll_wait(epoll_fd,events,EVENT_MAX,-1);
        handle_event(epoll_fd,events,ret,fd,command_buf);
    }
    close(fd);
}

void handle_event(int epoll_fd, struct epoll_event *events, int ret, int listenfd_t, char *buf) {
    for (int i = 0; i < ret; i++) {
        epollArgs *args = events[i].data.ptr;
        if (args->fd == listenfd_t) {
            struct sockaddr_in cliaddr;
            socklen_t socklen = sizeof(struct sockaddr_in);
            int socketfd = accept(listenfd_t, (struct sockaddr *) &cliaddr, &socklen);
            if (socketfd == -1) {
                perror("accpet error:");
            } else {
                //set non blocking
                int flags = fcntl(socketfd, F_GETFL, 0);
                if (flags == -1 || fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    perror("fcntl error");
                }
                epollArgs *args1 = NULL;
                epoll_args_init(&args1, socketfd,"220 list FTP server ready.\r\n");
                args1->fds->command_fd = socketfd;
                event_add(epoll_fd, socketfd, args1, EPOLLIN | EPOLLOUT | EPOLLET);
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
    int temp = -1;
    epollArgs *args = event->data.ptr;

    if(args->fds->write_mode == FTP_DATA || args->fds->write_mode == FTP_FILE){
        ftp_write_data(epoll_fd, event, 1);
        return 0;
    }

    if(args->client_exit && args->fd != -1){
        close(args->fd);
        args->fd = -1;
        args->client_exit = 0;
        return 1;
    }
    if(args->fd == args->fds->command_fd){
        //读到缓冲区
        while (1){
            temp = read(args->fd, buf + buf_end, n);
            if(temp == -1){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    break;
                }else{
                    perror("read error:");
                    close(args->fd);
                    event_delete(epoll_fd,args->fd,EPOLLIN);
                }
            } else {
                if(temp == 0){
                    break;
                }
                buf_end = buf_end + temp;
            }
        }

        //分解命令
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
    return temp;
}

int ftp_write(int epoll_fd, struct epoll_event *event) {
    (void) epoll_fd;

    size_t n = 0;
    epollArgs *epollArgs1 = event->data.ptr;

    if(epollArgs1->client_exit && epollArgs1->fd != -1 && epollArgs1->buffer_length != 0){
        while (1){
            char *position_char = strchr(epollArgs1->buffer,'\n');
            ssize_t position = (position_char == NULL) ? -1 : position_char - epollArgs1->buffer;
            if(position != -1){
                n = write(epollArgs1->fd, epollArgs1->buffer , position + 1);
                if(n <= epollArgs1->buffer_length){
                    epollArgs1->buffer_length -= (position + 1);
                    memmove(epollArgs1->buffer, epollArgs1->buffer+position + 1, BUFFER_SIZE_MAX - (position + 2));
                }
            }else
                break;
        }
        close(epollArgs1->fd);
        epollArgs1->fd = -1;
        epollArgs1->client_exit = 0;
        return 1;
    }

    if(epollArgs1->fd == epollArgs1->fds->command_fd){
        while (1){
            char *position_char = strchr(epollArgs1->buffer,'\n');
            ssize_t position = (position_char == NULL) ? -1 : position_char - epollArgs1->buffer;
            if(position != -1){
                n = write(epollArgs1->fd, epollArgs1->buffer , position + 1);
                if(n <= epollArgs1->buffer_length){
                    epollArgs1->buffer_length -= (position + 1);
                    memmove(epollArgs1->buffer, epollArgs1->buffer+position + 1, BUFFER_SIZE_MAX - (position + 2));
                    if(n == epollArgs1->buffer_length && epollArgs1->client_exit){
                        close(epollArgs1->fd);
                        epollArgs1->fd = -1;
                        epollArgs1->client_exit = 0;
                    }
                }
            }else
                break;
        }
    }else if(epollArgs1->fd == epollArgs1->fds->data_fd){
        ftp_write_data(epoll_fd, event, 2);
    }
    return n;
}


int ftp_write_data(int epoll_fd, struct epoll_event *event, int mode) {
    (void) epoll_fd;
    ssize_t n = 0;
    epollArgs *epollArgs1 = event->data.ptr;

    if (mode == 1) {
        ssize_t ret = read(epollArgs1->fds->data_fd, epollArgs1->fds->data + epollArgs1->fds->data_length,
                           DATA_SIZE_MAX - epollArgs1->fds->data_length);
        if (ret == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                return 0;
            else {
                perror("ftp read error:");
                return 0;
            }
        }
        if (ret == 0) {
            close(epollArgs1->fds->data_fd);
            epollArgs1->fds->data_fd = -1;
            epollArgs1->fds->write_mode = WRITE_FREE;
            event_delete(epoll_fd, epollArgs1->fds->data_fd,EPOLLOUT | EPOLLIN);
            if (epollArgs1->fds->trans_mode == PASV) {
                epollArgs1->fds->pasv_fd = -1;
                epollArgs1->fds->trans_mode = INIT;
            }
            write(epollArgs1->fds->command_fd, "226 File transfered completely\r\n",
                  strlen("226 File transfered completely\r\n"));
            return 0;
        }
        epollArgs1->fds->data_length += ret;

        while (epollArgs1->fds->data_length) {
            n = write(epollArgs1->fds->file_fd, epollArgs1->fds->data, epollArgs1->fds->data_length);
            if (n == -1) {
                perror("write file");
            }
            memmove(epollArgs1->fds->data, epollArgs1->fds->data + n, DATA_SIZE_MAX - n);
            epollArgs1->fds->data_length -= n;
        }
        if(epollArgs1->fds->data_fd == -1){
            close(epollArgs1->fds->file_fd);
            epollArgs1->fds->file_fd = -1;
        }
        return 0;
    } else if (mode == 2 && epollArgs1->fds->ready_to_write) {
        if (epollArgs1->fds->write_mode == FTP_FILE) {
            ssize_t ret = read(epollArgs1->fds->file_fd, epollArgs1->fds->data + epollArgs1->fds->data_length,
                               DATA_SIZE_MAX - epollArgs1->fds->data_length);
            if (ret == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    return 0;
                else {
                    perror("ftp read error:");
                    return 0;
                }
            }
            if (ret == 0) {
                close(epollArgs1->fds->data_fd);
                close(epollArgs1->fds->file_fd);
                epollArgs1->fds->file_fd = -1;
                epollArgs1->fds->data_fd = -1;
                epollArgs1->fds->write_mode = WRITE_FREE;
                event_delete(epoll_fd, epollArgs1->fds->data_fd, EPOLLIN|EPOLLOUT);
                if (epollArgs1->fds->trans_mode == PASV) {
                    epollArgs1->fds->pasv_fd = -1;
                    epollArgs1->fds->trans_mode = INIT;
                }
                epollArgs1->fds->ready_to_write = 0;
                write(epollArgs1->fds->command_fd, "226 File transfered completely\r\n",
                      strlen("226 File transfered completely\r\n"));
                return 0;
            }
            epollArgs1->fds->data_length += ret;

            while (epollArgs1->fds->data_length) {
                n = write(epollArgs1->fds->data_fd, epollArgs1->fds->data, epollArgs1->fds->data_length);
                if (n == -1) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN)
                        return 0;
                    perror("write file");
                }
                memmove(epollArgs1->fds->data, epollArgs1->fds->data + n, DATA_SIZE_MAX - n);
                epollArgs1->fds->data_length -= n;
            }
        } else if (epollArgs1->fds->write_mode == FTP_DATA) {
                epollArgs1->fds->write_mode = WRITE_FREE;
                if (epollArgs1->fds->trans_mode == PASV) {
                    epollArgs1->fds->pasv_fd = -1;
                    epollArgs1->fds->trans_mode = INIT;
                }
            while (epollArgs1->fds->data_length) {
                n = write(epollArgs1->fds->data_fd, epollArgs1->fds->data, epollArgs1->fds->data_length);
                if (n == -1) {
                    perror("write file");
                }
                memmove(epollArgs1->fds->data, epollArgs1->fds->data + n, DATA_SIZE_MAX - n);
                epollArgs1->fds->data_length -= n;
            }
            close(epollArgs1->fds->data_fd);
            epollArgs1->fds->data_fd = -1;
            epollArgs1->fds->ready_to_write = 0;
        }
        return 0;
    }
    return 0;
}

int handle_command(int epoll_fd, struct epoll_event *event,char *command) {

    epollArgs *args = event->data.ptr;
    if(args->client_login)
    {
        for(size_t i = 0;i< COMMAND_LENGTH;i++){
            if(strncmp(command_list[i].command,command,command_list[i].length) == 0){
                return (*command_list[i].callback)(epoll_fd, event, command[command_list[i].length] == ' ' ? command+command_list[i].length+1 : command + command_list[i].length);
            }
        }
    } else {
        for(size_t i = 0;i< COMMAND_LENGTH_BEFORE_LOGIN; i++){
            if(strncmp(command_list_brfore_login[i].command,command,command_list_brfore_login[i].length) == 0){
                return (*command_list_brfore_login[i].callback)(epoll_fd, event, command[command_list_brfore_login[i].length] == ' ' ? command+command_list_brfore_login[i].length+1 : command + command_list_brfore_login[i].length);
            }
        }
    }
    return 0;
}

int handle_user(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    size_t len = strlen(command);
    if(strncmp("anonymous",command,len-2) == 0){
        epollArgs *args = event->data.ptr;
        strcpy(args->buffer + args->buffer_length,"331 Guest login ok, send your complete e-mail address as password.\r\n");
        args->buffer_length += strlen("331 Guest login ok, send your complete e-mail address as password.\r\n");
    }else{
        epollArgs *args = event->data.ptr;
        strcpy(args->buffer + args->buffer_length,"331 Please enter your password.\r\n");
        args->buffer_length += strlen("331 Please enter your password.\r\n");
        return -1;
    }
    return 0;
}

int handle_pass(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    (void) command;
    epollArgs *args = event->data.ptr;
    args->client_login = 1;
    strcpy(args->buffer + args->buffer_length, "230 Guest login ok, access restrictions apply.\r\n");
    args->buffer_length += strlen("230 Guest login ok, access restrictions apply.\r\n");
    return 0;
}

int handle_syst(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    (void) command;
    epollArgs *args = event->data.ptr;
    strcpy(args->buffer +args->buffer_length, "215 UNIX Type: L8\r\n");
    args->buffer_length += strlen("215 UNIX Type: L8\r\n");
    return 0;
}

int handle_list(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    (void) command;
    char lscommand[255];
    epollArgs *args = event->data.ptr;
    if(args->fds->data_fd == -1 && args->fds->pasv_fd == -1){
        strcpy(args->buffer + args->buffer_length, "425 Use PORT or PASV first.\r\n");
        args->buffer_length += strlen("425 Use PORT or PASV first.\r\n");
        return -1;
    }
    strcpy(args->buffer + args->buffer_length,"150 directory list start\r\n");
    args->buffer_length += strlen("150 directory list start\r\n");
    FILE *stream;
    strcpy(lscommand,"ls -Nn ");
    strcpy(lscommand + 7, args->working_dir);
    stream = popen( lscommand, "r" );
    int ret = fread( args->fds->data+args->fds->data_length, sizeof(char), DATA_SIZE_MAX - args->fds->data_length, stream);
    args->fds->data_length += ret;
    pclose( stream );
    strcpy(args->buffer+args->buffer_length,"226 directory list end\r\n");
    args->buffer_length += strlen("226 directory list end\r\n");
    args->fds->write_mode = FTP_DATA;
    args->fds->ready_to_write = 1;
    if (args->fds->trans_mode == PASV && args->fds->pasv_fd != -1) {
        int fd_t = accept(args->fds->pasv_fd, NULL, NULL);
        if (fd_t != -1) {
            args->fds->data_fd = fd_t;
            int flags = fcntl(args->fds->data_fd, F_GETFL, 0);
            if (flags == -1 || fcntl(args->fds->data_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                perror("fcntl error");
            }
            epollArgs *args1 = NULL;
            epoll_args_init(&args1, args->fds->data_fd, NULL);
            args1->fds = args->fds;
            event_add(epoll_fd,  args->fds->data_fd, args1, EPOLLOUT);
            return 0;
        } else {
            if (errno != EAGAIN)
                perror("PASV accept");
        }
    } else {
        event_modify(epoll_fd, args->fds->data_fd, &args, EPOLLOUT);
    }
    return 0;
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
    if(args->fds->data_fd != -1){
        close(args->fds->data_fd);
    }
    args->fds->data_fd = socket(args->addr.ss_family,SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(args->fds->data_fd == -1){
        strcpy(args->buffer+args->buffer_length, "425 connection error");
        args->buffer_length += strlen("425 connection error");
    } else {
        int flags = fcntl(args->fds->data_fd, F_GETFL, 0);
        if (flags == -1 || fcntl(args->fds->data_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl error");
        }

        if(connect(args->fds->data_fd, (struct sockaddr *)&args->addr, args->addrlen) == -1){
            if(errno == EINPROGRESS){
                epollArgs *args1;
                epoll_args_init(&args1, args->fds->data_fd,NULL);
                args1->fds = args->fds;
                event_add(epoll_fd, args->fds->data_fd,args1,EPOLLIN | EPOLLOUT);
                args1->fds->trans_mode = PORT;
                strcpy(args->buffer+args->buffer_length,"200 PORT command successful.\r\n");
                args->buffer_length += strlen("200 PORT command successful.\r\n");
            } else {
                perror("connect error");
                strcpy(args->buffer+args->buffer_length,"425 connection error\r\n");
                args->buffer_length += strlen("425 connection error\r\n");
            }
        } else {
            epollArgs *args1 = NULL;
            epoll_args_init(&args1, args->fds->data_fd,NULL);
            args1->fds = args->fds;
            args->fds->trans_mode = PORT;
            event_add(epoll_fd, args->fds->data_fd,args1, EPOLLIN | EPOLLOUT);
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
    args->fds->pasv_fd = socket(args->pasv_addr.ss_family, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (args->fds->pasv_fd == -1) {
        perror("pasv handle error");
    } else {
        struct sockaddr_in addr = *(struct sockaddr_in *) &args->pasv_addr;
        addr.sin_port = 0;
        socklen_t addrlen = sizeof(addr);
        if (bind(args->fds->pasv_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
            close(args->fds->pasv_fd);
            args->fds->pasv_fd = -1;
        } else if (getsockname(args->fds->pasv_fd, (struct sockaddr *) &addr, &addrlen) == -1) {
            close(args->fds->pasv_fd);
            args->fds->pasv_fd = -1;
        } else if (listen(args->fds->pasv_fd, SOMAXCONN) == -1) {
            close(args->fds->pasv_fd);
            args->fds->pasv_fd = -1;
        } else {
            epollArgs *args1 = NULL;
            epoll_args_init(&args1,args->fds->pasv_fd,NULL);
            args1->fds = args->fds;
            args->fds->trans_mode = PASV;
            event_add(epoll_fd, args->fds->pasv_fd,args1,EPOLLIN | EPOLLOUT);
            unsigned char *host = (unsigned char *) &addr.sin_addr, *port = (unsigned char *) &addr.sin_port;
            size_t ret_t = str_cpy(&args, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n",host[0], host[1], host[2], host[3], port[0], port[1]);
            args->buffer_length += ret_t;
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
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    if(!strcmp(".",command)||!strcmp("./",command)){
        strcpy(args->buffer+args->buffer_length,"250 Directory changed successfully.\r\n");
        args->buffer_length += strlen("250 Directory changed successfully.\r\n");
    } else if(!strcmp("../",command)||!strcmp("..",command)){
        if(strlen(args->working_dir)==args->wd_len){
            strcpy(args->buffer+args->buffer_length,"250 Directory changed successfully.\r\n");
            args->buffer_length += strlen("250 Directory changed successfully.\r\n");
        } else {
            for (size_t i= strlen(args->working_dir) - 2; i >0 ; --i) {
                if(args->working_dir[i] == '/'){
                    args->working_dir[i+1] = '\0';
                    strcpy(args->buffer+args->buffer_length,"250 Directory changed successfully.\r\n");
                    args->buffer_length += strlen("250 Directory changed successfully.\r\n");
                    break;
                }
            }
        }
    } else if(!strcmp("/",command)){
        args->working_dir[args->wd_len] = '\0';
        strcpy(args->buffer+args->buffer_length,"250 Directory changed successfully.\r\n");
        args->buffer_length += strlen("250 Directory changed successfully.\r\n");
    } else {
        char temp[WORKINGDIRECTORY];
        strcpy(temp, args->working_dir);
        strcpy(temp+strlen(args->working_dir), command);

        DIR *dir = opendir(temp);
        if(dir){
            strcpy(args->working_dir+strlen(args->working_dir),command);
            strcpy(args->buffer+args->buffer_length,"250 Directory changed successfully.\r\n");
            args->buffer_length += strlen("250 Directory changed successfully.\r\n");
        } else {
            strcpy(args->buffer+args->buffer_length,"550 Failed to change directory.\r\n");
            args->buffer_length += strlen("550 Failed to change directory.\r\n");
            if(ENOENT != errno) {
                perror("cwd opendir error");
                return -1;
            }
        }
    }
    if(args->working_dir[strlen(args->working_dir)-1]!='/'){
        args->working_dir[strlen(args->working_dir)]='/';
        args->working_dir[strlen(args->working_dir) +1]='\0';
    }
    return 0;
}

int handle_pwd(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    (void) command;
    epollArgs *args = event->data.ptr;
    const char *root = args->working_dir + args->wd_len - 1;
    if (!*root)
        root = "/";

    int ret = str_cpy(&args, "257 \"%s\" is the current directory.\r\n", root);
    args->buffer_length += ret;
    return 0;
}

int handle_type(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    if(strcmp(command, "I") == 0){
        epollArgs *args = event->data.ptr;
        strcpy(args->buffer+args->buffer_length, "200 Type set to I.\r\n");
        args->buffer_length += strlen("200 Type set to I.\r\n");
    } else if (strcmp(command, "A") == 0){
        epollArgs *args = event->data.ptr;
        strcpy(args->buffer+args->buffer_length, "200 Switching to ASCII mode.\r\n");
        args->buffer_length += strlen("200 Switching to ASCII mode.\r\n");
    } else {
        epollArgs *args = event->data.ptr;
        strcpy(args->buffer+args->buffer_length, "500 Unrecognised TYPE command.\r\n");
        args->buffer_length += strlen("500 Unrecognised TYPE command.\r\n");
    }
    return 0;
}

int handle_retr(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    if(args->fds->data_fd == -1 && args->fds->pasv_fd == -1){
        strcpy(args->buffer + args->buffer_length, "425 Use PORT or PASV first.\r\n");
        args->buffer_length += strlen("425 Use PORT or PASV first.\r\n");
        return -1;
    }
    char filepath[1000];
    strcpy(filepath,args->working_dir);
    strcpy(filepath+strlen(args->working_dir),command);
    struct stat stat_t;
    int temp = stat(filepath, &stat_t);
    if(((args->fds->file_fd = open(filepath, O_RDONLY | O_NONBLOCK)) == -1)
    || (temp == -1) ||(!S_ISREG(stat_t.st_mode)) || (args->rest_offset > stat_t.st_size)){
        strcpy(args->buffer+args->buffer_length, "425 Failed to open file.\r\n");
        args->buffer_length += strlen("425 Failed to open file.\r\n");
        if(args->fds->data_fd != -1){
            close(args->fds->data_fd);
            args->fds->data_fd = -1;
        }
        return -1;
    } else {
        lseek(args->fds->file_fd, args->rest_offset, SEEK_SET);
        size_t ret = str_cpy(&args, "150 Opening BINARY mode data connection (%ld bytes).\r\n", stat_t.st_size);
        if(ret < (BUFFER_SIZE_MAX-args->buffer_length)){
            args->buffer_length += ret;
        } else {
            //to do
        }
        args->fds->write_mode = FTP_FILE;
        args->fds->ready_to_write = 1;
        if (args->fds->trans_mode == PASV && args->fds->pasv_fd != -1) {
            int fd_t = accept(args->fds->pasv_fd, NULL, NULL);
            if (fd_t != -1) {
                args->fds->data_fd = fd_t;
                int flags = fcntl(args->fds->data_fd, F_GETFL, 0);
                if (flags == -1 || fcntl(args->fds->data_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    perror("fcntl error");
                }
                epollArgs *args1 = NULL;
                epoll_args_init(&args1, args->fds->data_fd, NULL);
                args1->fds = args->fds;
                event_add(epoll_fd,  args->fds->data_fd, args1, EPOLLOUT);
                return 0;
            } else {
                if (errno != EAGAIN)
                    perror("PASV accept");
            }
        } else {
            event_modify(epoll_fd, args->fds->data_fd, &args, EPOLLOUT);
        }
        return 0;
    }
}

int handle_mkd(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    struct stat st = {0};
    char path_t[PATH_MAX];
    epollArgs *args = event->data.ptr;
    strcpy(path_t, args->working_dir);
    strcpy(path_t+strlen(args->working_dir),command);
    if (stat(path_t, &st) == -1) {
        if(!mkdir(path_t, 0755)){
            int ret = str_cpy(&args, "257 %s created\r\n", path_t);
            args->buffer_length += ret;
        }
    }
    return 0;
}

int handle_quit(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    (void) command;
    epollArgs *args = event->data.ptr;
    strcpy(args->buffer+args->buffer_length,"221 Goodbye.\r\n");
    args->buffer_length += strlen("221 Goodbye.\r\n");
    remove_connections(&args);
    args->client_exit = 1;
    return 0;
}

int handle_stor(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    if(args->fds->data_fd == -1 && args->fds->pasv_fd == -1){
        strcpy(args->buffer + args->buffer_length, "425 Use PORT or PASV first.\r\n");
        args->buffer_length += strlen("425 Use PORT or PASV first.\r\n");
        return -1;
    }
    char filepath[1000];
    strcpy(filepath,args->working_dir);
    strcpy(filepath+strlen(args->working_dir),command);
    if(((args->fds->file_fd = open(filepath, O_WRONLY | O_CREAT | O_NONBLOCK | O_TRUNC, 0644)) == -1)){
        strcpy(args->buffer+args->buffer_length, "425 Failed to open file.\r\n");
        args->buffer_length += strlen("425 Failed to open file.\r\n");
        if(args->fds->data_fd != -1){
            close(args->fds->data_fd);
            args->fds->data_fd = -1;
        }
        return -1;
    } else {
        lseek(args->fds->file_fd,args->appe_offset,SEEK_SET);
        size_t ret = str_cpy(&args, "150 The server is ready.\r\n");
        if(ret<(BUFFER_SIZE_MAX-args->buffer_length)){
            args->buffer_length += ret;
        } else {
            //to do
        }
        args->fds->write_mode = FTP_FILE;
        if (args->fds->trans_mode == PASV && args->fds->pasv_fd != -1) {
            int fd_t = accept(args->fds->pasv_fd, NULL, NULL);
            if (fd_t != -1) {
                args->fds->data_fd = fd_t;
                int flags = fcntl(args->fds->data_fd, F_GETFL, 0);
                if (flags == -1 || fcntl(args->fds->data_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    perror("fcntl error");
                }
                epollArgs *args1 = NULL;
                epoll_args_init(&args1, args->fds->data_fd, NULL);
                args1->fds = args->fds;
                event_add(epoll_fd,  args->fds->data_fd, args1, EPOLLIN);
                return 0;
            } else {
                if (errno != EAGAIN)
                    perror("PASV accept");
            }
        } else {
            event_modify(epoll_fd, args->fds->data_fd, &args, EPOLLIN);
        }
        return 0;
    }
}

int handle_rnfr(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    strcpy(args->rename_from, args->working_dir);
    strcpy(args->rename_from, command);
    if(!access(args->rename_from,X_OK)){
        int ret = str_cpy(&args, "350 Ready for RNTO.\r\n");
        args->buffer_length += ret;
        args->reto = 1;
        return 0;
    } else {
        args->rename_from[0] = '\0';
        int ret = str_cpy(&args, "550 RNFR command failed.\r\n");
        args->buffer_length += ret;
        return -1;
    }
}

int handle_rnto(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    if(args->reto){
        char rename_to[PATH_MAX];
        strcpy(rename_to, args->working_dir);
        strcpy(rename_to + strlen(args->working_dir),command);
        if (rename(args->rename_from, rename_to) == 0) {
            int ret = str_cpy(&args, "250 Rename successful.\r\n");
            args->buffer_length += ret;
            args->reto = 0;
            return 0;
        } else {
           perror("rnto error:");
            return -1;
        }
    }
    return -1;
}

int handle_size(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    char filepath[1000];
    strcpy(filepath,args->working_dir);
    strcpy(filepath+strlen(args->working_dir),command);
    struct stat stat_t;
    int temp = stat(filepath, &stat_t);
    if(temp && !S_ISREG(stat_t.st_mode)){
        int ret = str_cpy(&args, "550 Failed to get the size of the file.\r\n");
        args->buffer_length += ret;
        return -1;
    } else {
        size_t ret = str_cpy(&args, "213 the file size is (%ld bytes).\r\n", stat_t.st_size);
        if(ret < (BUFFER_SIZE_MAX-args->buffer_length)){
            args->buffer_length += ret;
        }
        return 0;
    }
}

int handle_rmd(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    char dir_name[PATH_MAX];
    strcpy(dir_name, args->working_dir);
    strcpy(dir_name+strlen(args->working_dir), command);
    if(rmdir(dir_name) == 0){
        int ret = str_cpy(&args, "250 remove directory operation successful\r\n");
        args->buffer_length += ret;
    } else if(rmdir(dir_name) == -1){
        int ret = str_cpy(&args, "550 remove directory operation failed\r\n");
        args->buffer_length += ret;
        return -1;
    }
    return 0;
}

int handle_dele(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    char filename[PATH_MAX];
    strcpy(filename, args->working_dir);
    strcpy(filename+strlen(args->working_dir), command);
    if(remove(filename) == 0){
        int ret = str_cpy(&args, "250 remove file operation successful\r\n");
        args->buffer_length += ret;
    } else if(remove(filename) == -1){
        int ret = str_cpy(&args, "550 remove file operation failed\r\n");
        args->buffer_length += ret;
        return -1;
    }
    return 0;
}

int handle_appe(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    char *str_part;
    args->appe_offset = strtoul(command, &str_part, 10);
    return 0;
}

int handle_rest(int epoll_fd, struct epoll_event *event, char *command) {
    (void) epoll_fd;
    epollArgs *args = event->data.ptr;
    char *strpart;
    args->rest_offset = strtoul(command, &strpart, 10);
    size_t ret = str_cpy(&args, "350 Restart position accepted (%ld).\r\n",args->rest_offset);
    args->buffer_length += ret;
    return 0;
}

int str_cpy(epollArgs **args, char *format, ...) {
    va_list list;
    va_start(list, format);
    size_t ret = vsnprintf((*args)->buffer+(*args)->buffer_length,BUFFER_SIZE_MAX-(*args)->buffer_length, format, list);
    va_end(list);
    return ret;
}

int remove_connections(epollArgs **args) {
    if((*args)->fds->file_fd!=-1){
        close((*args)->fds->file_fd);
    }
    if((*args)->fds->data_fd!=-1){
        close((*args)->fds->data_fd);
    }
    if((*args)->fds->pasv_fd!=-1){
        close((*args)->fds->pasv_fd);
    }
    return 0;
}

int handle_abor(int epoll_fd, struct epoll_event *event, char *command) {
    (void) command;
    epollArgs *args = event->data.ptr;
    if(args->fds->data_fd != -1){
        close(args->fds->data_fd);
        args->fds->data_fd = -1;
        event_delete(epoll_fd, args->fds->data_fd, EPOLLOUT | EPOLLIN);
    }
    if(args->fds->file_fd != -1){
        close(args->fds->file_fd);
        args->fds->file_fd = -1;
    }
    if(args->fds->pasv_fd != -1){
        close(args->fds->pasv_fd);
        args->fds->pasv_fd = -1;
        event_delete(epoll_fd, args->fds->pasv_fd, EPOLLOUT | EPOLLIN);
    }
    if(args->fds->ready_to_write == 1){
        args->fds->ready_to_write = 0;
    }
    args->fds->write_mode = WRITE_FREE;
    args->fds->trans_mode = INIT;
    size_t ret = str_cpy(&args, "225 No transfer to ABOR.\r\n");
    args->buffer_length += ret;
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
                strcpy(pathname, optarg);
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
