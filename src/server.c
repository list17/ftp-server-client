#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "hash_table.h"

static int socket_fd, epoll_fd;
size_t epoll_size = 0;

static void event_add(int epoll_fd, int fd, int state);
static void event_delete(int epoll_fd, int fd, int state);
static void event_modify(int epoll_fd, int fd, int state);
static void handle_event(int epoll_fd,struct epoll_event *events, int listenfd, char *buf);

void event_add(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    event.data.fd = fd;
    int flag = epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
    if(flag == 0){
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
    if(flag == 0){
        perror("event delete error");
    }else{
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

void handle_event(int epoll_fd, struct epoll_event *events, int listenfd, char *buf) {
    int fd;
    for(int i = 0;i < epoll_size;i++){
        fd = events
    }
}

static void socket_create_bind_local(port){

    struct sockaddr_in server_addr;
    int opt = 1;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket");
        exit(1);
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1){
        perror("Setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = INADDR_ANY;


    if (bind(socket_fd,(struct sockaddr *)&server_addr, sizeof(struct sockaddr))){
        perror("Unable to bind");
        exit(1);
    }
}

static int make_socket_non_blocking(int sfd){
    int flags;

    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1){
        perror("fcntl error");
        return -1;
    }

    flags |= O_NONBLOCK; // a |= b means a = a & b

    if (fcntl(sfd, F_GETFL, flags) == -1){
        perror("fcntl error 2");
        return -1;
    }
    return 0;
}

void accept_and_add_new(){
    struct epoll_event event;
    struct sockaddr in_addr;
    socklen_t in_len  = sizeof(in_addr);
    int infd;

    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    while ((infd = accept(socket_fd, &in_addr, &in_len)) != -1){
        if(getnameinfo(&in_addr,in_len,
                hbuf, sizeof(hbuf),
                sbuf, sizeof(sbuf),
                NI_NUMERICHOST) == 0){
            printf("Accepted connection on descriptor %d (host=%s, port=%s)\n",
                   infd, hbuf, sbuf);
        }

        event.data.fd = infd;
        event.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, infd, &event) == -1){
            perror("epoll_ctl");
            abort();
        }
        in_len = sizeof(in_addr);
    }
    if (errno != EAGAIN && errno != EWOULDBLOCK){
        perror("accept");
    }
}

void process_new_data(int fd)
{
    ssize_t count;
    char buf[512];

    while ((count = read(fd, buf, sizeof(buf) - 1))) {
        if (count == -1) {
            /* EAGAIN, read all data */
            if (errno == EAGAIN)
                return;

            perror("read");
            break;
        }

        /* Write buffer to stdout */
        buf[count] = '\0';
        printf("%s \n", buf);
    }

    printf("Close connection on descriptor: %d\n", fd);
    close(fd);
}


int main(int argc, char *argv[]) {

    char *host = "127.0.0.1";
	char *pathname = "/tmp";
	char *port = "21";
	int listenfd, connfd;		//监听socket和连接socket不一样，后者用于数据传输
	struct sockaddr_in addr;
	char sentence[8192];
	int p;
	int len;

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
}
