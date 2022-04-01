#include "lib/s3_connection_bak.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "lib/s3_packet_header.pb-c.h"

#define PORT 9000
#define IP "127.0.0.1"

#define EV_IO_LOOP_NUM 4
pthread_t io_threads[EV_IO_LOOP_NUM];
struct ev_loop *io_loops[EV_IO_LOOP_NUM] = {NULL};
int64_t accept_cnt = 0;
int pipefds[EV_IO_LOOP_NUM][2];

S3ConnectionBak *s3_connection_bak_construct() {
    S3ConnectionBak *conn = malloc(sizeof(S3ConnectionBak));
    if (conn != NULL) {
        *conn = (S3ConnectionBak)s3_connection_bak_null;
    }
    return conn;
}

void s3_connection_bak_desconstruct(S3ConnectionBak *conn) {
    if (conn != NULL) {
        s3_connection_bak_destroy(conn);
        free(conn);
    }
}

int s3_connection_bak_init(S3ConnectionBak *conn, struct ev_loop *loop, int fd) {
    conn->loop          = loop;
    conn->fd            = fd;

    conn->read_watcher.data = conn;
    conn->write_watcher.data = conn;
}

void s3_connection_bak_destroy(S3ConnectionBak *conn) {
    if (conn != NULL) {
        close(conn->fd);
        ev_io_stop(conn->loop, &conn->read_watcher);
        ev_io_stop(conn->loop, &conn->write_watcher);
    }
}


static void s3_connection_bak_create_multi_io_loop();
static void *s3_connection_bak_do_create_io_loop(void *arg);
static void s3_connection_bak_ev_loop_timer_hook(EV_P_ ev_timer *w, int revents);

static int s3_connection_bak_create_socket();
static void s3_connection_bak_accept_socket_cb(struct ev_loop *loop, ev_io *w, int revents);
static void s3_connection_bak_recv_socket_cb(struct ev_loop *loop, ev_io *w, int revents);
static void s3_connection_bak_write_socket_cb(struct ev_loop *loop, ev_io *w, int revents);

int s3_connection_bak_create_listen_and_io_loop(struct ev_loop *loop) {
    int listenfd = 0;
    ev_io lwatcher;

    s3_connection_bak_create_multi_io_loop();

    listenfd = s3_connection_bak_create_socket();
    if (listenfd < 0) {
        perror("create listenfd fail\n");
        return -1;
    }

    ev_io_init(&lwatcher, s3_connection_bak_accept_socket_cb, listenfd, EV_READ);
    ev_io_start(loop, &lwatcher);

    ev_run(loop, 0);

    return 0;
}

void s3_connection_bak_loop_run(struct ev_loop *loop) {
    ev_run(loop, 0);
}

static void s3_connection_bak_create_multi_io_loop() {
    int ret = 0;
    for (int i = 0; i < EV_IO_LOOP_NUM; ++i) {
        uint64_t j = i;
        ret = pthread_create(&io_threads[i], NULL, s3_connection_bak_do_create_io_loop, (void*)j);
        if (ret != 0) { // TODO: create fail
            printf("pthread create fail, ret=%d, errno=%d", ret, errno);
            break;
        }
    }
    usleep(500);
}

static void s3_connection_bak_pipe_read_cb(struct ev_loop *loop, ev_io *w, int revents) {
    int socket_fd;
    int ret = read(w->fd, &socket_fd, sizeof(int));
    printf("read socket fd=%d, ret=%d\n", socket_fd, ret);

    //struct ev_loop *io_loop = io_loops[accept_cnt % EV_IO_LOOP_NUM]; // FIXME: accept_cnt atomic
    //printf("accept_cnt=%ld, io_loop=%ld\n", accept_cnt, (uint64_t*)io_loop);

    S3ConnectionBak *conn = s3_connection_bak_construct();
    s3_connection_bak_init(conn, loop, socket_fd);
    conn->id = accept_cnt++;

    ev_io_init(&conn->read_watcher, s3_connection_bak_recv_socket_cb, socket_fd, EV_READ);
    ev_io_init(&conn->write_watcher, s3_connection_bak_write_socket_cb, socket_fd, EV_WRITE);

    ev_io_start(loop, &conn->read_watcher);

}

static void *s3_connection_bak_do_create_io_loop(void *arg) {
    int64_t idx = (int64_t) arg;
    if (idx < 0 || idx >= EV_IO_LOOP_NUM) {
        printf("loop idx invalid, idx=%d\n", idx);
        return NULL;
    }

    struct ev_loop *loop = ev_loop_new(EVBACKEND_EPOLL);
    if (loop == NULL) {
        perror("new ev loop fail.");
        return NULL;
    }
    io_loops[idx] = loop;
    printf("create io ev loop, idx=%d, loop=%ld\n", idx, (uint64_t*)loop);

    //ev_timer wtimer;
    //ev_timer_init(&wtimer, s3_connection_bak_ev_loop_timer_hook, 0., 10.);
    //ev_timer_again(loop, &wtimer);

    int ret = pipe(pipefds[idx]);
    if (ret != 0) {
        printf("pipe fail. ret=%d, errno=%d\n", ret, errno);
        exit(1);
    }
    // TODO pipefds[idx][0] set non block

    ev_io pipe_read_watcher;
    ev_io_init(&pipe_read_watcher, s3_connection_bak_pipe_read_cb, pipefds[idx][0], EV_READ);
    ev_io_start(loop, &pipe_read_watcher);

     /* FIXME:
     *  before start run, at least regist one watcher.
     *  if regist a timer watcher, would WAIT the timer watcher callback,
     *  and then the io watcher would be work.
     */
    ret = ev_run(loop, 0);
    printf("io loop run over, loop idx=%d, ret=%d\n", idx, ret);
    //ev_timer_stop(loop, &wtimer);
    ev_io_stop(loop, &pipe_read_watcher);
    ev_loop_destroy(loop);
}

static void s3_connection_bak_ev_loop_timer_hook(EV_P_ ev_timer *w, int revents) {
    (void) w;
    (void) revents;
    (void) loop;
    printf("----------------------ev timer. revents=%d\n", revents); // what is revents?
}


static int s3_connection_bak_create_socket() {
    struct sockaddr_in addr;
    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);

    if(s == -1){
        perror("create socket error \n");
        return -1;
    }

    int so_reuseaddr = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));
    bzero(&addr, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);

    if(bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
        perror("bind socket error \n");
        return -1;
    }

    if(listen(s, 32) == -1){
        perror("listen socket error\n");
        return -1;
    }
    printf("bind %s, listen %d\n", IP, PORT);

    return s;
}

static void s3_connection_bak_accept_socket_cb(struct ev_loop *loop, ev_io *w, int revents) {
    int fd;
    int s = w->fd;
    struct sockaddr_in sin;
    socklen_t addrlen = sizeof(struct sockaddr);
    do {
        fd = accept(s, (struct sockaddr *)&sin, &addrlen);
        if(fd > 0) {
            break;
        }

        if(errno == EAGAIN || errno == EWOULDBLOCK){
            continue;
        }
    } while(1);

    int pipe_write_fd = pipefds[accept_cnt % EV_IO_LOOP_NUM][1]; // FIXME: accept_cnt atomic
    write(pipe_write_fd, &fd, sizeof(fd));
    usleep(100);
}

#define MAX_BUF_LEN 1024
static void s3_connection_bak_recv_socket_cb(struct ev_loop *loop, ev_io *w, int revents) {
    char buf[MAX_BUF_LEN] = {0};
    int ret = 0;
    S3ConnectionBak *conn = (S3ConnectionBak*)w->data;

    do {
        ret = recv(w->fd, buf, MAX_BUF_LEN - 1, 0);
        if (ret > 0) {
            /*
             * FIXME:
             *   解包，必须提供长度参数。但同一个protobuf结构，成员值不同时，其长度不一.
             *   替换为其它数据交换格式
             */
            S3PacketHeader *header = s3_packet_header__unpack(NULL, ret, buf);
            printf("recv len=%d message: header.pcode=%d, .session_id=%ld, .data_len=%ld, conn id=%d\n",
                    ret,
                    header->pcode,
                    header->session_id,
                    header->data_len,
                    conn->id);
            s3_packet_header__free_unpacked(header, NULL); // 释放空间

            ev_io_start(loop, &conn->write_watcher);
            return;
        }

        if (ret == 0) {
            printf("remote socket closed\n");
            break;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            continue;
        }

        break;
    } while(1);

    close(w->fd);
    ev_io_stop(loop, w);
    s3_connection_bak_desconstruct(conn);
}

static void s3_connection_bak_write_socket_cb(struct ev_loop *loop, ev_io *w, int revents) {
    char buf[MAX_BUF_LEN] = {0};

    snprintf(buf, MAX_BUF_LEN - 1, "this is test message from libev\n");
    send(w->fd, buf, strlen(buf), 0);
    ev_io_stop(loop, w);
}
