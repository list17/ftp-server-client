
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
#define COMMAND_LENGTH 22
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

    off_t rest_offset;
    off_t appe_offset;

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